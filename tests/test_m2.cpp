#include "doctest.hpp"
#include "helpers.hpp"
#include "m2.hpp"

static Rng rng = make_rng(1);

TEST_CASE("complement_mutants makes one mutant per position") {
    std::vector<FastaEntry> mutants = complement_mutants({"seq", "ACGT"});
    REQUIRE(mutants.size() == 4);
    CHECK(mutants[0].name == "seq_mm_0_A_T");
    CHECK(mutants[0].sequence == "TCGT");
    CHECK(mutants[3].sequence == "ACGA");
}

TEST_CASE("run_m2 normalizes RNA, lowercase, and mixed input to DNA") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"a", "acgu"}, {"b", "ACGTu"}});
    run_m2(input, output, false, false, rng);
    std::vector<FastaEntry> entries = read_fasta(output);
    REQUIRE(entries.size() == 2 + 4 + 5);
    CHECK(entries[0].sequence == "ACGT");
    CHECK(entries[1].name == "a_mm_0_A_T");
    CHECK(entries[1].sequence == "TCGT");
    CHECK(entries[5].sequence == "ACGTT");
    for (const FastaEntry& entry : entries) CHECK(entry.sequence.find('U') == std::string::npos);
}

TEST_CASE("run_m2 replaces IUPAC codes once per entry") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"a", "NNNNACGT"}});
    run_m2(input, output, false, false, rng);
    std::vector<FastaEntry> entries = read_fasta(output);
    REQUIRE(entries.size() == 1 + 8);
    const std::string& wild_type = entries[0].sequence;
    CHECK(wild_type.find('N') == std::string::npos);
    for (size_t i = 1; i < entries.size(); i++) {
        size_t mismatches = 0;
        for (size_t j = 0; j < wild_type.size(); j++) mismatches += entries[i].sequence[j] != wild_type[j];
        CHECK(mismatches == 1);
    }
}

TEST_CASE("run_m2 rejects invalid characters with the entry name") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    write_fasta_file(input, {{"bad", "ACGX"}});
    CHECK_THROWS_WITH(run_m2(input, dir.file("out.fasta"), false, false, rng), doctest::Contains("bad"));
}

TEST_CASE("mutant suffixes follow the first word of the header") {
    std::vector<FastaEntry> mutants = complement_mutants({"seq some description", "AC"});
    REQUIRE(mutants.size() == 2);
    CHECK(mutants[0].name == "seq_mm_0_A_T some description");
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"seq some description", "AC"}});
    run_m2(input, output, false, false, rng);
    CHECK(read_fasta(output)[0].name == "seq_wt some description");
}

TEST_CASE("all_mutants makes three mutants per position") {
    CHECK(all_mutants({"seq", "ACGT"}).size() == 12);
}

TEST_CASE("mutant functions reject bases outside DNA") {
    CHECK_THROWS(complement_mutants({"seq", "ANGT"}));
    CHECK_THROWS(all_mutants({"seq", "ACGU"}));
}

TEST_CASE("run_m2 writes the wild type and the mutants") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"a", "acg"}, {"b", "TT"}});
    run_m2(input, output, false, false, rng);
    std::vector<FastaEntry> entries = read_fasta(output);
    REQUIRE(entries.size() == 2 + 3 + 2);
    CHECK(entries[0].name == "a_wt");
    CHECK(entries[0].sequence == "ACG");
    CHECK(entries[4].name == "b_wt");
    CHECK_THROWS(run_m2(input, output, false, false, rng));
    CHECK_NOTHROW(run_m2(input, output, true, true, rng));
    CHECK(read_fasta(output).size() == 2 + 9 + 6);
}

TEST_CASE("run_m2 rejects an empty name and a duplicate name") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"", "ACGT"}});
    CHECK_THROWS_WITH(run_m2(input, output, false, false, rng), doctest::Contains("empty name"));
    write_fasta_file(input, {{"a x", "ACGT"}, {"a y", "ACGT"}});
    CHECK_THROWS_WITH(run_m2(input, output, false, false, rng), doctest::Contains("'a'"));
}

TEST_CASE("complement_mutant_patterns mirrors complement_mutants") {
    FastaEntry entry{"seq", "ACGT"};
    std::vector<std::string> patterns = complement_mutant_patterns("ACGT");
    std::vector<FastaEntry> mutants = complement_mutants(entry);
    REQUIRE(patterns.size() == mutants.size());
    for (size_t i = 0; i < patterns.size(); i++) CHECK(patterns[i] == mutants[i].sequence);
    CHECK(complement_mutant_patterns("AN") == std::vector<std::string>{"TN", "AN"});
}
