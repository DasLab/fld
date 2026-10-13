#ifndef FLD_M2_HPP
#define FLD_M2_HPP

#include <string>
#include <vector>

#include "fasta.hpp"
#include "random.hpp"

// Returns one mutant per position, each with the base replaced by its complement. Throws on a
// base outside A, C, G, and T. The mutant suffix follows the first word of the header.
std::vector<FastaEntry> complement_mutants(const FastaEntry& entry);

// Returns three mutants per position, one for each other DNA base. Throws on a base outside
// A, C, G, and T.
std::vector<FastaEntry> all_mutants(const FastaEntry& entry);

// Resolves every entry of a FASTA file and writes the wild type and the mutants.
void run_m2(const std::string& input, const std::string& output, bool all, bool overwrite, Rng& rng);

#endif
