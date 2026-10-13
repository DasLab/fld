#include "commands.hpp"

#include <optional>

#include "barcode.hpp"
#include "cli.hpp"
#include "files.hpp"
#include "m2.hpp"
#include "pipeline.hpp"
#include "stem.hpp"
#include "text.hpp"

static const std::string OUTPUT_OPTION = "--output";
static const std::string OUTPUT_SHORT_OPTION = "-o";
static const std::string OVERWRITE_OPTION = "--overwrite";
static const std::string SEED_OPTION = "--seed";
static const std::string CONSTRUCT_TEMPLATE_OPTION = "--construct-template";
static const std::string CONSTRUCT_LENGTH_OPTION = "--construct-length";
static const std::string M2_OPTION = "--m2";
static const std::string NO_PREDICT_OPTION = "--no-predict";
static const std::string SORT_OPTION = "--sort";
static const std::string COUNT_OPTION = "--count";
static const std::string LENGTH_OPTION = "--length";
static const std::string ALL_OPTION = "--all";
static const std::string MIN_STEM_OPTION = "--min-stem";
static const std::string MAX_STEM_OPTION = "--max-stem";
static const std::string SPACER_OPTION = "--spacer";
static const std::string MAX_AU_OPTION = "--max-au";
static const std::string MAX_GC_OPTION = "--max-gc";
static const std::string MAX_GU_OPTION = "--max-gu";
static const std::string TERMINAL_GC_OPTION = "--terminal-gc";

static void add_output_option(Command& command, const std::string& help) {
    command.add_option(OUTPUT_OPTION, help);
    command.add_short_name(OUTPUT_OPTION, OUTPUT_SHORT_OPTION);
}

static void add_pair_options(Command& command) {
    StemLimits defaults;
    command.add_option(MAX_AU_OPTION, "Maximum AU pairs per stem", count_text(defaults.max_au));
    command.add_option(MAX_GC_OPTION, "Maximum GC pairs per stem", count_text(defaults.max_gc));
    command.add_option(MAX_GU_OPTION, "Maximum GU pairs per stem", count_text(defaults.max_gu));
    command.add_option(TERMINAL_GC_OPTION, "GC pairs at the open end of each stem", count_text(defaults.terminal_gc));
}

static void add_padding_options(Command& command) {
    StemLimits defaults;
    command.add_option(MIN_STEM_OPTION, "Minimum padding stem length", count_text(defaults.min_length));
    command.add_option(MAX_STEM_OPTION, "Maximum padding stem length", count_text(defaults.max_length));
    command.add_option(SPACER_OPTION, "PolyA spacer length between padding hairpins", count_text(defaults.spacer));
}

static void add_seed_option(Command& command) {
    command.add_optional(SEED_OPTION, "Seed for the random generator");
}

static StemLimits pair_limits_from(const Command& command) {
    StemLimits limits;
    limits.max_au = command.count_value(MAX_AU_OPTION);
    limits.max_gc = command.count_value(MAX_GC_OPTION);
    limits.max_gu = command.count_value(MAX_GU_OPTION);
    limits.terminal_gc = command.size_value(TERMINAL_GC_OPTION);
    return limits;
}

static StemLimits stem_limits_from(const Command& command) {
    StemLimits limits = pair_limits_from(command);
    limits.min_length = command.size_value(MIN_STEM_OPTION);
    limits.max_length = command.size_value(MAX_STEM_OPTION);
    limits.spacer = command.size_value(SPACER_OPTION);
    return limits;
}

static std::optional<size_t> seed_from(const Command& command) {
    if (!command.has(SEED_OPTION)) {
        return std::nullopt;
    }
    return command.size_value(SEED_OPTION);
}

static Command pipeline_command() {
    Command command(PIPELINE_COMMAND, "Designs a library from FASTA files of design sequences.");
    command.add_positionals("fasta", "Input FASTA files. Each file becomes one sublibrary.");
    add_output_option(command, "Output directory");
    command.add_option(CONSTRUCT_LENGTH_OPTION, "Total length of every construct");
    command.add_option(CONSTRUCT_TEMPLATE_OPTION, "Regions of the construct from 5' to 3'");
    command.add_flag(OVERWRITE_OPTION, "Overwrite the files in an existing output directory");
    command.add_flag(M2_OPTION, "Add the complement mutants of each design as a sublibrary");
    command.add_flag(NO_PREDICT_OPTION, "Skip read prediction and assign padding and barcodes at random");
    command.add_option(SORT_OPTION, "Order of the output: " + sort_order_names(), SORT_NONE);
    add_seed_option(command);
    add_padding_options(command);
    add_pair_options(command);
    return command;
}

static PipelineOptions pipeline_options_from(const Command& command) {
    PipelineOptions options;
    options.inputs = command.positionals();
    options.output_dir = command.value(OUTPUT_OPTION);
    options.overwrite = command.flag(OVERWRITE_OPTION);
    options.layout = parse_layout(command.value(CONSTRUCT_TEMPLATE_OPTION));
    options.construct_length = command.size_value(CONSTRUCT_LENGTH_OPTION);
    options.stem = stem_limits_from(command);
    options.m2 = command.flag(M2_OPTION);
    options.predict = !command.flag(NO_PREDICT_OPTION);
    options.sort = parse_sort_order(command.value(SORT_OPTION));
    options.seed = seed_from(command);
    return options;
}

int run_pipeline_command(const std::vector<std::string>& args) {
    Command command = pipeline_command();
    command.parse(args);
    run_pipeline(pipeline_options_from(command));
    return 0;
}

static Command barcodes_command() {
    Command command(BARCODES_COMMAND, "Writes hairpin barcodes, one per line, at pairwise Hamming distance two or more.");
    command.add_option(COUNT_OPTION, "Number of barcodes");
    command.add_option(LENGTH_OPTION, "Stem length of each barcode");
    add_output_option(command, "Output file");
    command.add_flag(OVERWRITE_OPTION, "Overwrite an existing output file");
    add_seed_option(command);
    add_pair_options(command);
    return command;
}

static void write_random_barcodes(const Command& command) {
    std::string output = command.value(OUTPUT_OPTION);
    require_writable(output, command.flag(OVERWRITE_OPTION));
    Rng rng = make_rng(seed_from(command));
    std::vector<std::string> barcodes = random_barcodes(
        command.size_value(COUNT_OPTION), command.size_value(LENGTH_OPTION), pair_limits_from(command), rng);
    write_lines(output, barcodes);
}

int run_barcodes_command(const std::vector<std::string>& args) {
    Command command = barcodes_command();
    command.parse(args);
    write_random_barcodes(command);
    return 0;
}

static Command m2_command() {
    Command command(M2_COMMAND, "Writes the wild type and the single mutants of every sequence in a FASTA file.");
    command.add_positional("fasta", "Input FASTA file");
    add_output_option(command, "Output FASTA file");
    command.add_flag(ALL_OPTION, "Write all three mutants per position instead of the complement only");
    command.add_flag(OVERWRITE_OPTION, "Overwrite an existing output file");
    add_seed_option(command);
    return command;
}

int run_m2_command(const std::vector<std::string>& args) {
    Command command = m2_command();
    command.parse(args);
    Rng rng = make_rng(seed_from(command));
    run_m2(command.positional(), command.value(OUTPUT_OPTION),
           command.flag(ALL_OPTION), command.flag(OVERWRITE_OPTION), rng);
    return 0;
}
