#ifndef FLD_DESIGN_HPP
#define FLD_DESIGN_HPP

#include <string>
#include <vector>

#include "fasta.hpp"
#include "random.hpp"

// Returns the first word of the header, which names the entry.
std::string identifier(const FastaEntry& entry);

// Throws if two entries have the same identifier. The source names the input in the message.
void require_distinct_identifiers(const std::vector<FastaEntry>& entries, const std::string& source);

// Normalizes each entry to uppercase DNA and replaces its IUPAC codes with random bases.
// Throws on an empty identifier or two entries with the same identifier. The source names
// the input in error messages.
std::vector<FastaEntry> resolve_designs(
    const std::vector<FastaEntry>& entries,
    const std::string& source,
    Rng& rng
);

#endif
