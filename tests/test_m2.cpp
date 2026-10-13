#include "doctest.hpp"
#include "helpers.hpp"
#include "m2.hpp"

TEST_CASE("complement_mutants makes one mutant per position in the input alphabet") {
    std::vector<FastaEntry> mutants = complement_mutants({"seq", "ACGU"});
    REQUIRE(mutants.size() == 4);
    CHECK(mutants[0].name == "seq_mm_0_A_U");
    CHECK(mutants[0].sequence == "UCGU");
    CHECK(mutants[3].sequence == "ACGA");
    for (const FastaEntry& m : mutants) CHECK(m.sequence.find('T') == std::string::npos);
}

TEST_CASE("complement_mutants and all_mutants accept lowercase input") {
    std::vector<FastaEntry> mutants = complement_mutants({"seq", "acgu"});
    REQUIRE(mutants.size() == 4);
    CHECK(mutants[0].name == "seq_mm_0_A_U");
    CHECK(mutants[0].sequence == "UCGU");
    CHECK(all_mutants({"seq", "acgt"}).size() == 12);
}

TEST_CASE("mutant suffixes follow the first word of the header") {
    std::vector<FastaEntry> mutants = complement_mutants({"seq some description", "AC"});
    REQUIRE(mutants.size() == 2);
    CHECK(mutants[0].name == "seq_mm_0_A_T some description");
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"seq some description", "AC"}});
    run_m2(input, output, false, false);
    CHECK(read_fasta(output)[0].name == "seq_wt some description");
}

TEST_CASE("complement_mutants skips ambiguous positions") {
    CHECK(complement_mutants({"seq", "ANGT"}).size() == 3);
}

TEST_CASE("all_mutants makes three mutants per position") {
    std::vector<FastaEntry> mutants = all_mutants({"seq", "ACGT"});
    CHECK(mutants.size() == 12);
    for (const FastaEntry& m : mutants) CHECK(m.sequence.find('U') == std::string::npos);
}

TEST_CASE("run_m2 writes the wild type and the mutants") {
    TempDir dir;
    std::string input = dir.file("in.fasta");
    std::string output = dir.file("out.fasta");
    write_fasta_file(input, {{"a", "acg"}, {"b", "TT"}});
    run_m2(input, output, false, false);
    std::vector<FastaEntry> entries = read_fasta(output);
    REQUIRE(entries.size() == 2 + 3 + 2);
    CHECK(entries[0].name == "a_wt");
    CHECK(entries[0].sequence == "ACG");
    CHECK(entries[4].name == "b_wt");
    CHECK_THROWS(run_m2(input, output, false, false));
    CHECK_NOTHROW(run_m2(input, output, true, true));
    CHECK(read_fasta(output).size() == 2 + 9 + 6);
}
