#include "design.hpp"

#include <stdexcept>

#include "sequence.hpp"

static FastaEntry resolve_design(const FastaEntry& entry, const std::string& source, Rng& rng) {
    try {
        return FastaEntry{entry.name, resolve_polybases(normalize_sequence(entry.sequence), rng)};
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Entry '" + entry.name + "' in " + source + ": " + e.what());
    }
}

std::vector<FastaEntry> resolve_designs(
    const std::vector<FastaEntry>& entries,
    const std::string& source,
    Rng& rng
) {
    std::vector<FastaEntry> resolved;
    for (const FastaEntry& entry : entries) {
        resolved.push_back(resolve_design(entry, source, rng));
    }
    return resolved;
}
