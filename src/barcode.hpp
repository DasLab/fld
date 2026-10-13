#ifndef FLD_BARCODE_HPP
#define FLD_BARCODE_HPP

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

#include "random.hpp"
#include "stem.hpp"

// Returns the sequence and every sequence at Hamming distance one from it.
std::vector<std::string> hamming_ball(const std::string& seq);

bool has_hamming_neighbor(const std::string& seq, const std::unordered_set<std::string>& existing);

// Counts the hairpins that satisfy the pair limits, saturating at UNLIMITED.
size_t count_hairpins(size_t stem_length, const StemLimits& limits);

// Throws unless the barcodes are unique and pairwise at Hamming distance two or more.
void require_hamming_separated(const std::vector<std::string>& barcodes);

// Generates hairpin barcodes that are pairwise at Hamming distance two or more, and checks the
// result before returning it.
std::vector<std::string> random_barcodes(
    size_t count,
    size_t stem_length,
    const StemLimits& limits,
    Rng& rng
);

#endif
