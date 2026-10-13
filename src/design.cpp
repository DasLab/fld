#include "design.hpp"

#include <set>
#include <stdexcept>

#include "sequence.hpp"

std::string identifier(const FastaEntry& entry) {
    return entry.name.substr(0, entry.name.find_first_of(" \t"));
}

static void require_identifier(const FastaEntry& entry, const std::string& source) {
    if (identifier(entry).empty()) {
        throw std::runtime_error("An entry in " + source + " has an empty name");
    }
}

void require_distinct_identifiers(const std::vector<FastaEntry>& entries, const std::string& source) {
    std::set<std::string> seen;
    for (const FastaEntry& entry : entries) {
        if (!seen.insert(identifier(entry)).second) {
            throw std::runtime_error("Two entries in " + source + " are both named '" + identifier(entry) + "'");
        }
    }
}

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
        require_identifier(entry, source);
        resolved.push_back(resolve_design(entry, source, rng));
    }
    require_distinct_identifiers(resolved, source);
    return resolved;
}
