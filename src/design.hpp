#ifndef FLD_DESIGN_HPP
#define FLD_DESIGN_HPP

#include <string>
#include <vector>

#include "fasta.hpp"
#include "random.hpp"

// Normalizes each entry to uppercase DNA and replaces its IUPAC codes with random bases. The
// source names the input in error messages.
std::vector<FastaEntry> resolve_designs(
    const std::vector<FastaEntry>& entries,
    const std::string& source,
    Rng& rng
);

#endif
