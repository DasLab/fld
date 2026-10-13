#include "doctest.hpp"
#include "helpers.hpp"
#include "pipeline.hpp"
#include "fasta.hpp"
#include "library.hpp"
#include "predict.hpp"

#include <cmath>
#include <filesystem>
#include <set>

static PipelineOptions base_options(const TempDir& dir, const std::string& layout) {
    PipelineOptions options;
    options.inputs = {dir.file("in.fasta")};
    options.output_dir = dir.file("out");
    options.layout = parse_layout(layout);
    options.construct_length = 200;
    options.seed = 42;
    options.predict = false;
    return options;
}

static void check_outputs(const TempDir& dir, const PipelineOptions& options, size_t count) {
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    std::vector<FastaEntry> fasta = read_fasta(dir.file("out/" + LIBRARY_FASTA));
    REQUIRE(rows.size() == count + 1);
    REQUIRE(fasta.size() == count);
    const std::vector<std::string>& header = rows[0];
    size_t design_col = column_index(header, "design");
    size_t slot_count = options.layout.slots.size();
    for (size_t r = 1; r < rows.size(); r++) {
        const std::vector<std::string>& row = rows[r];
        std::string seq;
        for (size_t c = 3; c < 3 + slot_count; c++) seq += row[c];
        CHECK(seq == fasta[r - 1].sequence);
        CHECK(seq.size() == options.construct_length);
        std::string expected_name = row[1] + " sublibrary=" + row[2] + " begin=";
        CHECK(fasta[r - 1].name.substr(0, expected_name.size()) == expected_name);
        size_t begin = header_value(fasta[r - 1].name, "begin");
        size_t end = header_value(fasta[r - 1].name, "end");
        CHECK(seq.substr(begin, end - begin) == row[design_col]);
    }
}

TEST_CASE("pipeline fills every slot and keeps the construct length") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {
        {"one", "ACGTACGTACGTACGTACGT"},
        {"two", "acguacguacguacguacguacgu"},
        {"three,quoted", "NNNNACGTACGTACGTACGTACGTACGTACGT"},
    });
    PipelineOptions options = base_options(dir,
        "const:ACTCGAGTAGAGTCGAAAA,barcode:10,design,padding,barcode:8,const:AAAAGAAACAACAACAACAAC");
    run_pipeline(options);
    check_outputs(dir, options, 3);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    CHECK(rows[0] == std::vector<std::string>{
        "index", "name", "sublibrary", "const_1", "barcode_1", "design", "padding", "barcode_2", "const_2"});
    CHECK(rows[3][1] == "three,quoted");
    CHECK(rows[3][2] == "in");
    CHECK(rows[2][5] == "ACGTACGTACGTACGTACGTACGT");
    std::set<std::string> barcodes;
    for (size_t r = 1; r < rows.size(); r++) {
        barcodes.insert(rows[r][4]);
        barcodes.insert(rows[r][7]);
        CHECK(rows[r][4].size() == hairpin_length(10));
        CHECK(rows[r][7].size() == hairpin_length(8));
    }
    CHECK(barcodes.size() == 6);
}

TEST_CASE("pipeline is reproducible with a seed") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGTACGTACGTACGTACGT"}});
    PipelineOptions options = base_options(dir, "padding,design,barcode:6");
    run_pipeline(options);
    std::vector<FastaEntry> first = read_fasta(dir.file("out/" + LIBRARY_FASTA));
    options.overwrite = true;
    run_pipeline(options);
    std::vector<FastaEntry> second = read_fasta(dir.file("out/" + LIBRARY_FASTA));
    CHECK(first[0].sequence == second[0].sequence);
    options.seed = 43;
    run_pipeline(options);
    std::vector<FastaEntry> third = read_fasta(dir.file("out/" + LIBRARY_FASTA));
    CHECK(first[0].sequence != third[0].sequence);
}

TEST_CASE("pipeline without padding checks the construct length") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGTACGTAC"}});
    PipelineOptions options = base_options(dir, "const:GGGG,design,const:CCCC");
    options.construct_length = 18;
    CHECK_NOTHROW(run_pipeline(options));
    options.construct_length = 19;
    options.overwrite = true;
    CHECK_THROWS(run_pipeline(options));
}

TEST_CASE("pipeline rejects designs that do not fit") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"long", std::string(201, 'A')}});
    PipelineOptions options = base_options(dir, "padding,design");
    CHECK_THROWS(run_pipeline(options));
}

TEST_CASE("pipeline refuses to touch an existing output directory") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    run_pipeline(options);
    CHECK_THROWS(run_pipeline(options));
    options.overwrite = true;
    CHECK_NOTHROW(run_pipeline(options));
}

TEST_CASE("pipeline rejects invalid characters with the entry name") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"bad", "ACGTX"}});
    PipelineOptions options = base_options(dir, "padding,design");
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("bad"));
}

TEST_CASE("pipeline adds complement mutants as a separate sublibrary") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGT"}, {"two", "GG"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.m2 = true;
    run_pipeline(options);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    REQUIRE(rows.size() == 1 + 2 + 6);
    CHECK(rows[1][2] == "in");
    CHECK(rows[3][2] == "in_m2");
    CHECK(rows[3][0] == "0");
    CHECK(rows[3][1] == "one_mm_0_A_T");
    check_outputs(dir, options, 8);
}

static size_t mismatches(const std::string& a, const std::string& b) {
    REQUIRE(a.size() == b.size());
    size_t count = 0;
    for (size_t i = 0; i < a.size(); i++) count += a[i] != b[i];
    return count;
}

TEST_CASE("pipeline builds the mutants from the resolved wild type") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "NNNNNNNNacgu"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.m2 = true;
    run_pipeline(options);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    REQUIRE(rows.size() == 1 + 1 + 12);
    size_t design_col = column_index(rows[0], "design");
    const std::string& wild_type = rows[1][design_col];
    for (size_t r = 2; r < rows.size(); r++) {
        CHECK(mismatches(wild_type, rows[r][design_col]) == 1);
    }
}

TEST_CASE("pipeline leaves no output directory when the input is invalid") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"bad", "ACGTX"}});
    PipelineOptions options = base_options(dir, "padding,design");
    CHECK_THROWS(run_pipeline(options));
    CHECK(!std::filesystem::exists(options.output_dir));
    options.overwrite = false;
    write_fasta_file(dir.file("in.fasta"), {{"good", "ACGT"}});
    CHECK_NOTHROW(run_pipeline(options));
}

TEST_CASE("pipeline without padding rejects a wrong length before assembly") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"short", "ACGTACGTA"}});
    PipelineOptions options = base_options(dir, "const:GGGG,design,const:CCCC");
    options.construct_length = 18;
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("construct length is 18"));
}

TEST_CASE("pipeline without padding ignores the padding stem limits") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGTACGTAC"}});
    PipelineOptions options = base_options(dir, "design,barcode:5");
    options.construct_length = 10 + hairpin_length(5);
    options.stem.max_au = 0;
    CHECK_THROWS(validate_stem_limits(options.stem));
    CHECK_NOTHROW(run_pipeline(options));
}

TEST_CASE("pipeline removes the intermediate files of an earlier run") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    run_pipeline(options);
    std::filesystem::create_directories(dir.file("out/tmp"));
    write_fasta_file(dir.file("out/tmp/stale.fasta"), {{"old", "ACGT"}});
    options.overwrite = true;
    run_pipeline(options);
    CHECK(!std::filesystem::exists(dir.file("out/tmp/stale.fasta")));
}

TEST_CASE("pipeline rejects a mutant sublibrary that collides with an input") {
    TempDir dir;
    write_fasta_file(dir.file("a.fasta"), {{"one", "ACGT"}});
    write_fasta_file(dir.file("a_m2.fasta"), {{"two", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.inputs = {dir.file("a.fasta"), dir.file("a_m2.fasta")};
    CHECK_NOTHROW(run_pipeline(options));
    options.m2 = true;
    options.overwrite = true;
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("a_m2"));
}

TEST_CASE("pipeline rejects input file names with whitespace") {
    TempDir dir;
    write_fasta_file(dir.file("my lib.fasta"), {{"one", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.inputs = {dir.file("my lib.fasta")};
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("whitespace"));
}

TEST_CASE("pipeline sorts by design length") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"long", "ACGTACGT"}, {"short", "ACGT"}, {"medium", "ACGTAC"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.sort = SortOrder::Length;
    run_pipeline(options);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    REQUIRE(rows.size() == 4);
    CHECK(rows[1][1] == "short");
    CHECK(rows[2][1] == "medium");
    CHECK(rows[3][1] == "long");
    CHECK(rows[1][0] == "1");
}

TEST_CASE("pipeline rejects sorting by reads without prediction") {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {{"one", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.sort = SortOrder::Reads;
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("--sort reads"));
}

TEST_CASE("parse_sort_order reads the three orders and rejects others") {
    CHECK(parse_sort_order("none") == SortOrder::None);
    CHECK(parse_sort_order("reads") == SortOrder::Reads);
    CHECK(parse_sort_order("length") == SortOrder::Length);
    CHECK_THROWS(parse_sort_order("name"));
}

TEST_CASE("pipeline rejects two input files with the same name") {
    TempDir dir;
    std::filesystem::create_directories(dir.file("a"));
    std::filesystem::create_directories(dir.file("b"));
    write_fasta_file(dir.file("a/lib.fasta"), {{"one", "ACGT"}});
    write_fasta_file(dir.file("b/lib.fasta"), {{"two", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.inputs = {dir.file("a/lib.fasta"), dir.file("b/lib.fasta")};
    CHECK_THROWS_WITH(run_pipeline(options), doctest::Contains("lib"));
}

TEST_CASE("pipeline numbers each sublibrary from zero") {
    TempDir dir;
    write_fasta_file(dir.file("a.fasta"), {{"a1", "ACGT"}, {"a2", "ACGT"}});
    write_fasta_file(dir.file("b.fasta"), {{"b1", "ACGT"}});
    PipelineOptions options = base_options(dir, "padding,design");
    options.inputs = {dir.file("a.fasta"), dir.file("b.fasta")};
    run_pipeline(options);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    REQUIRE(rows.size() == 4);
    CHECK(rows[1][0] == "0");
    CHECK(rows[2][0] == "1");
    CHECK(rows[3][0] == "0");
    CHECK(rows[3][2] == "b");
}

TEST_CASE("pipeline predicts and assigns with rn-coverage" * doctest::skip(!rn_coverage_on_path())) {
    TempDir dir;
    write_fasta_file(dir.file("in.fasta"), {
        {"one", "ACGTACGTACGTACGTACGT"},
        {"two", "TGCATGCATGCATGCATGCA"},
        {"three", "AAAACCCCGGGGTTTTAAAA"},
    });
    PipelineOptions options = base_options(dir, "const:GGGG,padding,design,barcode:6,const:CCCC");
    options.construct_length = 80;
    options.predict = true;
    options.sort = SortOrder::Reads;
    run_pipeline(options);
    check_outputs(dir, options, 3);
    std::vector<std::vector<std::string>> rows = read_csv_file(dir.file("out/" + LIBRARY_CSV));
    const std::vector<std::string>& header = rows[0];
    CHECK(header.size() == 3 + 5 + 4);
    size_t reads_col = column_index(header, "reads");
    CHECK(column_index(header, "reads_design") < reads_col);
    CHECK(column_index(header, "reads_padding") < reads_col);
    CHECK(column_index(header, "reads_barcode_1") < reads_col);
    double previous = -1e300;
    for (size_t r = 1; r < rows.size(); r++) {
        double reads = std::stod(rows[r][reads_col]);
        CHECK(std::isfinite(reads));
        CHECK(reads >= previous);
        previous = reads;
        CHECK(std::isfinite(std::stod(rows[r][column_index(header, "reads_design")])));
    }
    CHECK(std::filesystem::exists(dir.file("out/tmp/library_80_reads.txt")));
}
