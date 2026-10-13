#include "random.hpp"

Rng make_rng() {
    std::random_device device;
    std::seed_seq seed{device(), device(), device(), device()};
    return Rng(seed);
}

Rng make_rng(std::optional<size_t> seed) {
    if (seed) {
        return Rng(*seed);
    }
    return make_rng();
}

size_t sample_range(size_t low, size_t high, Rng& rng) {
    std::uniform_int_distribution<size_t> dist(low, high);
    return dist(rng);
}
