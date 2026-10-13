#include "doctest.hpp"
#include "sequence.hpp"

TEST_CASE("normalize_sequence uppercases and converts U to T") {
    CHECK(normalize_sequence("acguACGU") == "ACGTACGT");
}

TEST_CASE("normalize_sequence keeps IUPAC codes") {
    CHECK(normalize_sequence("NRYKMSWVDHB") == "NRYKMSWVDHB");
}

TEST_CASE("normalize_sequence rejects other characters") {
    CHECK_THROWS(normalize_sequence("ACG\rT"));
    CHECK_THROWS(normalize_sequence("ACGX"));
    CHECK_THROWS(normalize_sequence("AC GT"));
}

TEST_CASE("resolve_polybases replaces codes with bases they denote") {
    Rng rng = make_rng(1);
    for (int i = 0; i < 50; i++) {
        std::string seq = resolve_polybases("ANRY", rng);
        CHECK(seq[0] == 'A');
        CHECK(is_dna_base(seq[1]));
        CHECK((seq[2] == 'A' || seq[2] == 'G'));
        CHECK((seq[3] == 'C' || seq[3] == 'T'));
    }
}

TEST_CASE("code_allows follows the IUPAC codes") {
    CHECK(code_allows('A', 'A'));
    CHECK_FALSE(code_allows('A', 'C'));
    CHECK(code_allows('R', 'G'));
    CHECK_FALSE(code_allows('R', 'C'));
    CHECK(code_allows('N', 'T'));
    CHECK_FALSE(code_allows('X', 'A'));
}

TEST_CASE("complement_code complements the bases a code denotes") {
    CHECK(complement_code('A') == 'T');
    CHECK(complement_code('R') == 'Y');
    CHECK(complement_code('K') == 'M');
    CHECK(complement_code('S') == 'S');
    CHECK(complement_code('W') == 'W');
    CHECK(complement_code('V') == 'B');
    CHECK(complement_code('D') == 'H');
    CHECK(complement_code('N') == 'N');
    CHECK_THROWS(complement_code('X'));
}

TEST_CASE("complement pairs the DNA bases") {
    CHECK(complement('A') == 'T');
    CHECK(complement('T') == 'A');
    CHECK(complement('G') == 'C');
    CHECK(complement('C') == 'G');
    CHECK_THROWS(complement('N'));
    CHECK_THROWS(complement('U'));
}

TEST_CASE("random_sequence has the requested length and only DNA bases") {
    Rng rng = make_rng(2);
    std::string seq = random_sequence(37, rng);
    CHECK(seq.size() == 37);
    for (char c : seq) CHECK(is_dna_base(c));
}
