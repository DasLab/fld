#include "doctest.hpp"
#include "helpers.hpp"
#include "sequence.hpp"
#include "stem.hpp"

static bool is_pair(char five, char three) {
    std::string pair = {five, three};
    return pair == "AT" || pair == "TA" || pair == "GC" || pair == "CG" || pair == "GT" || pair == "TG";
}

static bool is_gc_pair(char five, char three) {
    std::string pair = {five, three};
    return pair == "GC" || pair == "CG";
}

TEST_CASE("random_hairpin has stem, loop, and antisense stem") {
    Rng rng = make_rng(3);
    StemLimits limits = test_limits();
    for (size_t stem = 4; stem <= 13; stem++) {
        std::string hairpin = random_hairpin(stem, limits, rng);
        REQUIRE(hairpin.size() == hairpin_length(stem));
        for (size_t i = 0; i < stem; i++) {
            CHECK(is_pair(hairpin[i], hairpin[hairpin.size() - 1 - i]));
        }
    }
}

TEST_CASE("random_hairpin closes the stem with GC pairs") {
    Rng rng = make_rng(4);
    StemLimits limits = test_limits();
    limits.terminal_gc = 2;
    for (int i = 0; i < 20; i++) {
        std::string hairpin = random_hairpin(8, limits, rng);
        CHECK(is_gc_pair(hairpin[0], hairpin.back()));
        CHECK(is_gc_pair(hairpin[1], hairpin[hairpin.size() - 2]));
    }
}

TEST_CASE("random_hairpin respects the GC and GU limits") {
    Rng rng = make_rng(5);
    StemLimits limits = test_limits();
    limits.max_gc = 2;
    limits.max_gu = 0;
    for (int i = 0; i < 50; i++) {
        std::string hairpin = random_hairpin(10, limits, rng);
        size_t gc = 0;
        for (size_t j = 0; j < 10; j++) {
            char five = hairpin[j];
            char three = hairpin[hairpin.size() - 1 - j];
            if (is_gc_pair(five, three)) gc++;
            CHECK(!(five == 'G' && three == 'T'));
            CHECK(!(five == 'T' && three == 'G'));
        }
        CHECK(gc <= 2);
    }
}

TEST_CASE("validate_stem_limits rejects inconsistent limits") {
    StemLimits limits = test_limits();
    CHECK_NOTHROW(validate_stem_limits(limits));
    limits.max_gc = 0;
    CHECK_THROWS(validate_stem_limits(limits));
    limits = test_limits();
    limits.terminal_gc = 8;
    CHECK_THROWS(validate_stem_limits(limits));
    limits = test_limits();
    limits.max_length = 6;
    CHECK_THROWS(validate_stem_limits(limits));
    limits = test_limits();
    limits.max_au = 3;
    limits.max_gc = 3;
    CHECK_THROWS(validate_stem_limits(limits));
}

TEST_CASE("validate_stem_length rejects short stems") {
    CHECK_THROWS(validate_stem_length(2, test_limits()));
    CHECK_NOTHROW(validate_stem_length(10, test_limits()));
}

TEST_CASE("saturating arithmetic stops at UNLIMITED") {
    CHECK(saturating_add(UNLIMITED, 1) == UNLIMITED);
    CHECK(saturating_add(1, 2) == 3);
    CHECK(saturating_mul(UNLIMITED, 2) == UNLIMITED);
    CHECK(saturating_mul(3, 4) == 12);
    CHECK(saturating_mul(0, UNLIMITED) == 0);
}
