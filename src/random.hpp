#ifndef FLD_RANDOM_HPP
#define FLD_RANDOM_HPP

#include <cstddef>
#include <optional>
#include <random>
#include <vector>

using Rng = std::mt19937_64;

// Creates a generator seeded from the operating system.
Rng make_rng();

// Creates a generator with the given seed, or from the operating system when there is none.
Rng make_rng(std::optional<size_t> seed);

// Samples a uniform integer in the closed range [low, high].
size_t sample_range(size_t low, size_t high, Rng& rng);

// Samples a uniform element of a non-empty vector.
template <typename T>
const T& sample_element(const std::vector<T>& values, Rng& rng) {
    return values[sample_range(0, values.size() - 1, rng)];
}

#endif
