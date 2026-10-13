#include "padding.hpp"

#include <algorithm>
#include <vector>

#include "sequence.hpp"

struct PaddingPieces {
    std::vector<std::string> hairpins;
    std::string fill;
};

static size_t unit_length(size_t stem_length, const StemLimits& limits) {
    return hairpin_length(stem_length) + limits.spacer;
}

static bool fits_hairpin(size_t remaining, const StemLimits& limits) {
    return remaining >= unit_length(limits.min_length, limits);
}

static size_t random_stem_length(size_t remaining, const StemLimits& limits, Rng& rng) {
    size_t longest = (remaining - limits.spacer - HAIRPIN_LOOP_LENGTH) / 2;
    return sample_range(limits.min_length, std::min(longest, limits.max_length), rng);
}

static PaddingPieces random_pieces(size_t length, const StemLimits& limits, Rng& rng) {
    PaddingPieces pieces;
    size_t remaining = length;
    while (fits_hairpin(remaining, limits)) {
        size_t stem_length = random_stem_length(remaining, limits, rng);
        pieces.hairpins.push_back(random_hairpin(stem_length, limits, rng));
        remaining -= unit_length(stem_length, limits);
    }
    pieces.fill = random_sequence(remaining, rng);
    return pieces;
}

static std::string join_five_prime(const PaddingPieces& pieces, const std::string& spacer) {
    std::string padding = pieces.fill;
    for (const std::string& hairpin : pieces.hairpins) {
        padding += hairpin + spacer;
    }
    return padding;
}

static std::string join_three_prime(const PaddingPieces& pieces, const std::string& spacer) {
    std::string padding;
    for (const std::string& hairpin : pieces.hairpins) {
        padding += spacer + hairpin;
    }
    return padding + pieces.fill;
}

std::string random_padding(size_t length, PaddingSide side, const StemLimits& limits, Rng& rng) {
    PaddingPieces pieces = random_pieces(length, limits, rng);
    std::string spacer(limits.spacer, BASE_A);
    if (side == PaddingSide::FivePrime) {
        return join_five_prime(pieces, spacer);
    }
    return join_three_prime(pieces, spacer);
}
