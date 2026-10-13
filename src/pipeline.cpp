#include "pipeline.hpp"

#include <algorithm>
#include <filesystem>
#include <numeric>
#include <set>
#include <stdexcept>

#include "assign.hpp"
#include "barcode.hpp"
#include "design.hpp"
#include "fasta.hpp"
#include "files.hpp"
#include "library.hpp"
#include "m2.hpp"
#include "padding.hpp"
#include "predict.hpp"
#include "progress.hpp"
#include "sequence.hpp"
#include "verify.hpp"

static const std::string TMP_DIR = "tmp";
static const std::string M2_SUFFIX = "_m2";
static const std::string LIBRARY_LABEL = "library";
static const std::string READS_PREFIX = "reads_";
static const std::string READS_COLUMN = "reads";
const std::string SORT_NONE = "none";
const std::string SORT_READS = "reads";
const std::string SORT_LENGTH = "length";

std::string sort_order_names() {
    return SORT_NONE + ", " + SORT_READS + ", or " + SORT_LENGTH;
}

SortOrder parse_sort_order(const std::string& text) {
    if (text == SORT_NONE) return SortOrder::None;
    if (text == SORT_READS) return SortOrder::Reads;
    if (text == SORT_LENGTH) return SortOrder::Length;
    throw std::runtime_error("Unknown sort order '" + text + "'. Expected " + sort_order_names());
}

static std::string design_label(const Layout& layout) {
    return slot_names(layout)[design_slot(layout)];
}

// A pool of generated sequences for one variable slot, with one entry per construct.
struct ComponentSet {
    std::string label;
    size_t slot = 0;
    std::vector<std::string> sequences;
    Groups groups;
    std::vector<double> reads;
    std::vector<size_t> assignment;
};

struct Sublibrary {
    std::string name;
    std::vector<FastaEntry> entries;
};

// The sublibrary name goes into the FASTA header as a key=value field, so it cannot contain whitespace.
static std::string sublibrary_name(const std::string& input) {
    std::string name = file_stem(input);
    if (name.find_first_of(" \t") != std::string::npos) {
        throw std::runtime_error(
            "The input file name '" + name + "' contains whitespace. Rename the file, because " +
            "its name becomes the sublibrary name in the FASTA header.");
    }
    return name;
}

static std::string m2_sublibrary_name(const std::string& name) {
    return name + M2_SUFFIX;
}

// Returns the names of the sublibraries that the run writes, in output order.
static std::vector<std::string> sublibrary_names(const PipelineOptions& options) {
    std::vector<std::string> names;
    for (const std::string& input : options.inputs) {
        names.push_back(sublibrary_name(input));
        if (options.m2) {
            names.push_back(m2_sublibrary_name(names.back()));
        }
    }
    return names;
}

static void require_distinct_sublibraries(const PipelineOptions& options) {
    std::set<std::string> seen;
    for (const std::string& name : sublibrary_names(options)) {
        if (!seen.insert(name).second) {
            throw std::runtime_error("Two sublibraries would both be named '" + name + "'");
        }
    }
}

static void validate_inputs(const PipelineOptions& options) {
    if (options.inputs.empty()) {
        throw std::runtime_error("At least one input FASTA file is required");
    }
    for (const std::string& input : options.inputs) {
        require_file(input);
    }
    require_distinct_sublibraries(options);
}

static void validate_stem_options(const PipelineOptions& options) {
    if (has_padding_slot(options.layout)) {
        validate_stem_limits(options.stem);
    }
    for (size_t slot : barcode_slots(options.layout)) {
        validate_stem_length(options.layout.slots[slot].stem_length, options.stem);
    }
}

static void validate_prediction_options(const PipelineOptions& options) {
    if (options.sort == SortOrder::Reads && !options.predict) {
        throw std::runtime_error("--sort reads needs predicted reads, which --no-predict skips");
    }
    if (options.predict && !rn_coverage_on_path()) {
        throw std::runtime_error("rn-coverage is not on PATH. Install it, or pass --no-predict.");
    }
}

static void validate_options(const PipelineOptions& options) {
    validate_inputs(options);
    validate_stem_options(options);
    validate_prediction_options(options);
    require_writable(options.output_dir, options.overwrite);
}

static std::string tmp_dir(const PipelineOptions& options) {
    return join_path(options.output_dir, TMP_DIR);
}

// Removes the intermediate files of an earlier run, because rn-coverage appends to existing
// prediction files.
static void create_output_dirs(const PipelineOptions& options) {
    std::filesystem::create_directories(options.output_dir);
    std::filesystem::remove_all(tmp_dir(options));
    if (options.predict) {
        std::filesystem::create_directories(tmp_dir(options));
    }
}

static std::vector<FastaEntry> mutants_of(const std::vector<FastaEntry>& entries) {
    std::vector<FastaEntry> mutants;
    for (const FastaEntry& entry : entries) {
        for (const FastaEntry& m : complement_mutants(entry)) {
            mutants.push_back(m);
        }
    }
    return mutants;
}

// Reads each input file and resolves its entries. The mutants are built from the resolved
// entries, so each mutant differs from its wild type at exactly one position.
static std::vector<Sublibrary> load_sublibraries(const PipelineOptions& options, Rng& rng) {
    std::vector<Sublibrary> sublibraries;
    for (const std::string& input : options.inputs) {
        std::string name = sublibrary_name(input);
        std::vector<FastaEntry> entries = resolve_designs(read_fasta(input), name, rng);
        progress() << "Read " << entries.size() << " sequences from " << input << "\n";
        sublibraries.push_back(Sublibrary{name, entries});
        if (options.m2) {
            sublibraries.push_back(Sublibrary{m2_sublibrary_name(name), mutants_of(entries)});
        }
    }
    return sublibraries;
}

static Construct make_construct(
    const Layout& layout,
    size_t index,
    const FastaEntry& entry,
    const std::string& sublibrary
) {
    Construct construct;
    construct.index = index;
    construct.name = entry.name;
    construct.sublibrary = sublibrary;
    construct.parts.resize(layout.slots.size());
    for (size_t i = 0; i < layout.slots.size(); i++) {
        construct.parts[i] = layout.slots[i].constant;
    }
    construct.parts[design_slot(layout)] = entry.sequence;
    return construct;
}

static Library load_library(const PipelineOptions& options, Rng& rng) {
    Library library;
    library.layout = options.layout;
    for (const Sublibrary& sublibrary : load_sublibraries(options, rng)) {
        for (size_t i = 0; i < sublibrary.entries.size(); i++) {
            library.constructs.push_back(
                make_construct(options.layout, i, sublibrary.entries[i], sublibrary.name));
        }
    }
    if (library.constructs.empty()) {
        throw std::runtime_error("The input files contain no sequences");
    }
    return library;
}

// Without a padding slot, nothing adjusts the length of a construct, so every design must
// already bring its construct to the construct length.
static void require_construct_lengths(const Library& library, size_t construct_length) {
    if (has_padding_slot(library.layout)) return;
    for (const Construct& construct : library.constructs) {
        size_t length = fixed_length(library.layout) + design_length(library.layout, construct);
        if (length != construct_length) {
            throw std::runtime_error(
                "Construct '" + construct.name + "' in " + construct.sublibrary + " has " +
                std::to_string(length) + " bases, but the construct length is " +
                std::to_string(construct_length));
        }
    }
}

static std::vector<size_t> padding_lengths(const Library& library, size_t construct_length) {
    std::vector<size_t> lengths;
    for (const Construct& construct : library.constructs) {
        lengths.push_back(padding_length(library.layout, construct, construct_length));
    }
    return lengths;
}

static ComponentSet padding_components(const Library& library, const PipelineOptions& options, Rng& rng) {
    ComponentSet set;
    set.slot = padding_slot(library.layout);
    set.label = slot_names(library.layout)[set.slot];
    std::vector<size_t> lengths = padding_lengths(library, options.construct_length);
    PaddingSide side = padding_side(library.layout);
    for (size_t length : lengths) {
        set.sequences.push_back(random_padding(length, side, options.stem, rng));
    }
    set.groups = groups_by_key(lengths);
    return set;
}

static ComponentSet barcode_components(const Library& library, size_t slot, const StemLimits& stem, Rng& rng) {
    ComponentSet set;
    set.slot = slot;
    set.label = slot_names(library.layout)[slot];
    size_t count = library.constructs.size();
    set.sequences = random_barcodes(count, library.layout.slots[slot].stem_length, stem, rng);
    set.groups = single_group(count);
    return set;
}

static std::vector<ComponentSet> generate_components(const Library& library, const PipelineOptions& options, Rng& rng) {
    std::vector<ComponentSet> sets;
    if (has_padding_slot(library.layout)) {
        progress() << "Generating padding\n";
        sets.push_back(padding_components(library, options, rng));
    }
    for (size_t slot : barcode_slots(library.layout)) {
        progress() << "Generating " << slot_names(library.layout)[slot] << "\n";
        sets.push_back(barcode_components(library, slot, options.stem, rng));
    }
    return sets;
}

static void assign_identity(std::vector<ComponentSet>& sets) {
    for (ComponentSet& set : sets) {
        set.assignment.resize(set.sequences.size());
        std::iota(set.assignment.begin(), set.assignment.end(), 0);
    }
}

static std::vector<std::string> design_sequences(const Library& library) {
    std::vector<std::string> designs;
    for (const Construct& construct : library.constructs) {
        designs.push_back(construct.parts[design_slot(library.layout)]);
    }
    return designs;
}

static std::vector<std::string> full_sequences(const Library& library) {
    std::vector<std::string> sequences;
    for (const Construct& construct : library.constructs) {
        sequences.push_back(full_sequence(construct));
    }
    return sequences;
}

static std::vector<ComponentPool> pools_of(const std::vector<ComponentSet>& sets) {
    std::vector<ComponentPool> pools;
    for (const ComponentSet& set : sets) {
        pools.push_back(ComponentPool{set.reads, set.groups});
    }
    return pools;
}

// Predicts the design and component reads and assigns the components to minimize the
// variance of the summed reads. Returns the design reads.
static std::vector<double> assign_by_prediction(
    const Library& library,
    std::vector<ComponentSet>& sets,
    const std::string& work_dir
) {
    std::vector<double> design_reads = predict_reads(design_sequences(library), work_dir, design_label(library.layout));
    for (ComponentSet& set : sets) {
        set.reads = predict_reads(set.sequences, work_dir, set.label);
    }
    std::vector<std::vector<size_t>> assignments = minimize_variance(design_reads, pools_of(sets));
    for (size_t k = 0; k < sets.size(); k++) {
        sets[k].assignment = assignments[k];
    }
    return design_reads;
}

static void fill_parts(Library& library, const std::vector<ComponentSet>& sets) {
    for (const ComponentSet& set : sets) {
        for (size_t i = 0; i < library.constructs.size(); i++) {
            library.constructs[i].parts[set.slot] = set.sequences[set.assignment[i]];
        }
    }
}

static void verify_assembly(
    const Library& library,
    const std::vector<ComponentSet>& sets,
    const std::vector<std::string>& designs,
    size_t construct_length
) {
    for (const ComponentSet& set : sets) {
        require_permutation(set.assignment, set.groups);
    }
    verify_library(library, designs, construct_length);
    progress() << "Verified " << library.constructs.size() << " constructs\n";
}

static void attach_reads(
    Library& library,
    const std::vector<double>& design_reads,
    const std::vector<ComponentSet>& sets,
    const std::vector<double>& final_reads
) {
    library.read_columns = {READS_PREFIX + design_label(library.layout)};
    for (const ComponentSet& set : sets) {
        library.read_columns.push_back(READS_PREFIX + set.label);
    }
    library.read_columns.push_back(READS_COLUMN);
    for (size_t i = 0; i < library.constructs.size(); i++) {
        std::vector<double>& reads = library.constructs[i].reads;
        reads = {design_reads[i]};
        for (const ComponentSet& set : sets) {
            reads.push_back(set.reads[set.assignment[i]]);
        }
        reads.push_back(final_reads[i]);
    }
}

static void predict_final_reads(Library& library, const std::vector<double>& design_reads,
                                const std::vector<ComponentSet>& sets, const PipelineOptions& options) {
    std::vector<double> final_reads = predict_reads(full_sequences(library), tmp_dir(options), LIBRARY_LABEL);
    attach_reads(library, design_reads, sets, final_reads);
    verify_reads(library);
}

// Sorts the constructs by the final predicted reads, lowest first.
static void sort_by_reads(Library& library) {
    std::stable_sort(library.constructs.begin(), library.constructs.end(),
        [](const Construct& a, const Construct& b) { return a.reads.back() < b.reads.back(); });
}

// Sorts the constructs by design length, shortest first.
static void sort_by_design_length(Library& library) {
    const Layout& layout = library.layout;
    std::stable_sort(library.constructs.begin(), library.constructs.end(),
        [&layout](const Construct& a, const Construct& b) {
            return design_length(layout, a) < design_length(layout, b);
        });
}

static void sort_library(Library& library, SortOrder order) {
    switch (order) {
        case SortOrder::None: break;
        case SortOrder::Reads: sort_by_reads(library); break;
        case SortOrder::Length: sort_by_design_length(library); break;
    }
}

static void write_outputs(const Library& library, const std::string& output_dir) {
    write_library_csv(library, join_path(output_dir, LIBRARY_CSV));
    write_library_fasta(library, join_path(output_dir, LIBRARY_FASTA));
    progress() << "Wrote " << library.constructs.size() << " constructs to " << output_dir << "\n";
}

void run_pipeline(const PipelineOptions& options) {
    validate_options(options);
    Rng rng = make_rng(options.seed);
    Library library = load_library(options, rng);
    require_construct_lengths(library, options.construct_length);
    std::vector<std::string> designs = design_sequences(library);
    std::vector<ComponentSet> sets = generate_components(library, options, rng);
    // Created after loading and generation, so a failed run leaves no output directory.
    create_output_dirs(options);
    std::vector<double> design_reads;
    if (options.predict) {
        design_reads = assign_by_prediction(library, sets, tmp_dir(options));
    } else {
        assign_identity(sets);
    }
    fill_parts(library, sets);
    verify_assembly(library, sets, designs, options.construct_length);
    if (options.predict) {
        predict_final_reads(library, design_reads, sets, options);
    }
    sort_library(library, options.sort);
    write_outputs(library, options.output_dir);
}
