#include "m2.hpp"

#include <stdexcept>

#include "design.hpp"
#include "files.hpp"
#include "sequence.hpp"

static const std::string WILD_TYPE_SUFFIX = "_wt";
static const std::string MUTANT_INFIX = "_mm_";

// Inserts the suffix after the identifier, which is the first word of the header.
static std::string with_suffix(const std::string& header, const std::string& suffix) {
    size_t space = header.find_first_of(" \t");
    if (space == std::string::npos) return header + suffix;
    return header.substr(0, space) + suffix + header.substr(space);
}

static FastaEntry mutant(const FastaEntry& entry, size_t position, char base) {
    FastaEntry result = entry;
    result.sequence[position] = base;
    result.name = with_suffix(entry.name, MUTANT_INFIX + std::to_string(position) + "_" +
                              entry.sequence[position] + "_" + base);
    return result;
}

static void require_dna_base(const FastaEntry& entry, char base) {
    if (!is_dna_base(base)) {
        throw std::logic_error("Entry '" + entry.name + "' contains '" + std::string(1, base) + "', which is not a DNA base");
    }
}

std::vector<FastaEntry> complement_mutants(const FastaEntry& entry) {
    std::vector<FastaEntry> mutants;
    for (size_t i = 0; i < entry.sequence.size(); i++) {
        char original = entry.sequence[i];
        require_dna_base(entry, original);
        mutants.push_back(mutant(entry, i, complement(original)));
    }
    return mutants;
}

std::vector<FastaEntry> all_mutants(const FastaEntry& entry) {
    std::vector<FastaEntry> mutants;
    for (size_t i = 0; i < entry.sequence.size(); i++) {
        char original = entry.sequence[i];
        require_dna_base(entry, original);
        for (char base : dna_bases()) {
            if (base != original) mutants.push_back(mutant(entry, i, base));
        }
    }
    return mutants;
}

static void write_wild_type(std::ostream& out, const FastaEntry& entry) {
    write_fasta_entry(out, FastaEntry{with_suffix(entry.name, WILD_TYPE_SUFFIX), entry.sequence});
}

static void write_mutants(std::ostream& out, const FastaEntry& entry, bool all) {
    std::vector<FastaEntry> mutants = all ? all_mutants(entry) : complement_mutants(entry);
    for (const FastaEntry& m : mutants) {
        write_fasta_entry(out, m);
    }
}

void run_m2(const std::string& input, const std::string& output, bool all, bool overwrite, Rng& rng) {
    std::vector<FastaEntry> entries = resolve_designs(read_fasta(input), input, rng);
    require_writable(output, overwrite);
    std::ofstream out = open_output(output);
    for (const FastaEntry& entry : entries) {
        write_wild_type(out, entry);
        write_mutants(out, entry, all);
    }
    require_written(out, output);
}
