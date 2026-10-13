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

TEST_CASE("complement uses the alphabet of the sequence") {
    CHECK(complement('A', Alphabet::DNA) == 'T');
    CHECK(complement('A', Alphabet::RNA) == 'U');
    CHECK(complement('U', Alphabet::RNA) == 'A');
    CHECK(complement('G', Alphabet::DNA) == 'C');
    CHECK_THROWS(complement('N', Alphabet::DNA));
}

TEST_CASE("detect_alphabet finds U") {
    CHECK(detect_alphabet("ACGU") == Alphabet::RNA);
    CHECK(detect_alphabet("ACGT") == Alphabet::DNA);
}

TEST_CASE("random_sequence has the requested length and only DNA bases") {
    Rng rng = make_rng(2);
    std::string seq = random_sequence(37, rng);
    CHECK(seq.size() == 37);
    for (char c : seq) CHECK(is_dna_base(c));
}
