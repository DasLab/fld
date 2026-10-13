#include "doctest.hpp"
#include "helpers.hpp"
#include "barcode.hpp"

static size_t hamming_distance(const std::string& a, const std::string& b) {
    size_t distance = 0;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) distance++;
    }
    return distance;
}

TEST_CASE("has_hamming_neighbor detects the sequence itself and every single substitution") {
    std::unordered_set<std::string> existing = {"ACGT"};
    CHECK(has_hamming_neighbor("ACGT", existing) == true);
    for (size_t i = 0; i < 4; i++) {
        for (char base : {'A', 'C', 'G', 'T'}) {
            std::string variant = "ACGT";
            variant[i] = base;
            CHECK(has_hamming_neighbor(variant, existing) == true);
        }
    }
    CHECK(has_hamming_neighbor("AAGG", existing) == false);
    CHECK(has_hamming_neighbor("ACGTA", existing) == false);
}

TEST_CASE("has_hamming_neighbor detects transversions") {
    std::unordered_set<std::string> existing = {"GCTTCGAGC"};
    CHECK(has_hamming_neighbor("GCATCGAGC", existing) == true);
    CHECK(has_hamming_neighbor("GCTTCGAGC", existing) == true);
    CHECK(has_hamming_neighbor("GCAACGAGC", existing) == false);
}

TEST_CASE("random_barcodes are pairwise at Hamming distance two or more") {
    Rng rng = make_rng(10);
    StemLimits limits = test_limits();
    std::vector<std::string> barcodes = random_barcodes(300, 6, limits, rng);
    REQUIRE(barcodes.size() == 300);
    for (size_t i = 0; i < barcodes.size(); i++) {
        CHECK(barcodes[i].size() == hairpin_length(6));
        for (size_t j = i + 1; j < barcodes.size(); j++) {
            CHECK(hamming_distance(barcodes[i], barcodes[j]) >= 2);
        }
    }
}

TEST_CASE("require_hamming_separated rejects neighbors and duplicates") {
    CHECK_NOTHROW(require_hamming_separated({"ACGTTTCGACGT", "TGCATTCGTGCA"}));
    CHECK_THROWS(require_hamming_separated({"ACGTTTCGACGT", "ACGTTTCGACGA"}));
    CHECK_THROWS(require_hamming_separated({"ACGTTTCGACGT", "ACGTTTCGACGT"}));
}

TEST_CASE("random_barcodes fails when the capacity is too small") {
    Rng rng = make_rng(11);
    StemLimits limits = test_limits();
    CHECK_THROWS(random_barcodes(count_hairpins(4, limits) + 1, 4, limits, rng));
}

TEST_CASE("count_hairpins counts stems and loops") {
    StemLimits limits = test_limits();
    limits.max_au = 0;
    limits.max_gc = 4;
    limits.max_gu = 0;
    limits.terminal_gc = 1;
    // Every pair is GC, so the only freedom is the orientation of each pair and the loop.
    CHECK(count_hairpins(4, limits) == 16 * tetraloop_count());
}

TEST_CASE("count_hairpins saturates for long stems") {
    StemLimits limits = test_limits();
    limits.max_gc = UNLIMITED;
    CHECK(count_hairpins(70, limits) == UNLIMITED);
}
