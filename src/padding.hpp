#ifndef FLD_PADDING_HPP
#define FLD_PADDING_HPP

#include <cstddef>
#include <string>

#include "random.hpp"
#include "stem.hpp"

// The side of the design region on which the padding sits.
enum class PaddingSide { FivePrime, ThreePrime };

// Builds padding of exactly the given length from hairpins. A polyA spacer separates adjacent
// hairpins and separates the padding from the design. Length that cannot hold a hairpin is
// filled with random sequence at the end away from the design.
std::string random_padding(size_t length, PaddingSide side, const StemLimits& limits, Rng& rng);

#endif
