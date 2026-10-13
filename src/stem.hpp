#ifndef FLD_STEM_HPP
#define FLD_STEM_HPP

#include <cstddef>
#include <string>

#include "random.hpp"
#include "text.hpp"

constexpr size_t HAIRPIN_LOOP_LENGTH = 4;
constexpr size_t SHORTEST_STEM = 4;

// Limits on the hairpin stems used for padding and barcodes. The member defaults are the
// program defaults.
struct StemLimits {
    size_t min_length = 7;
    size_t max_length = 13;
    size_t max_au = UNLIMITED;
    size_t max_gc = 5;
    size_t max_gu = 0;
    size_t terminal_gc = 1;
    size_t spacer = 2;
};

size_t saturating_add(size_t a, size_t b);
size_t saturating_mul(size_t a, size_t b);

// Throws if a stem of this length cannot satisfy the pair limits.
void validate_stem_length(size_t stem_length, const StemLimits& limits);

// Throws if the limits are inconsistent with each other.
void validate_stem_limits(const StemLimits& limits);

size_t hairpin_length(size_t stem_length);

size_t tetraloop_count();

// Returns stem, tetraloop, and antisense stem as one string.
std::string random_hairpin(size_t stem_length, const StemLimits& limits, Rng& rng);

#endif
