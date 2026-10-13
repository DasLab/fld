#include "doctest.hpp"
#include "padding.hpp"
#include "sequence.hpp"

TEST_CASE("random_padding has exactly the requested length") {
    Rng rng = make_rng(6);
    StemLimits limits;
    for (size_t length = 0; length < 120; length++) {
        CHECK(random_padding(length, PaddingSide::FivePrime, limits, rng).size() == length);
        CHECK(random_padding(length, PaddingSide::ThreePrime, limits, rng).size() == length);
    }
}

TEST_CASE("five prime padding ends with a spacer next to the design") {
    Rng rng = make_rng(7);
    StemLimits limits;
    std::string padding = random_padding(100, PaddingSide::FivePrime, limits, rng);
    CHECK(padding.substr(padding.size() - limits.spacer) == std::string(limits.spacer, BASE_A));
}

TEST_CASE("three prime padding starts with a spacer next to the design") {
    Rng rng = make_rng(8);
    StemLimits limits;
    std::string padding = random_padding(100, PaddingSide::ThreePrime, limits, rng);
    CHECK(padding.substr(0, limits.spacer) == std::string(limits.spacer, BASE_A));
}

TEST_CASE("padding with one hairpin places the fill away from the design") {
    Rng rng = make_rng(9);
    StemLimits limits;
    limits.min_length = 7;
    limits.max_length = 7;
    // One hairpin of 18 bases plus a 2-base spacer, then 3 bases of fill.
    size_t length = hairpin_length(7) + limits.spacer + 3;
    std::string five = random_padding(length, PaddingSide::FivePrime, limits, rng);
    CHECK(five.substr(five.size() - 2) == "AA");
    std::string three = random_padding(length, PaddingSide::ThreePrime, limits, rng);
    CHECK(three.substr(0, 2) == "AA");
}
