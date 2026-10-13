#ifndef FLD_M2_HPP
#define FLD_M2_HPP

#include <string>
#include <vector>

#include "fasta.hpp"

// Returns one mutant per position, each with the base replaced by its complement. The mutants
// are written in uppercase, in the alphabet of the input.
std::vector<FastaEntry> complement_mutants(const FastaEntry& entry);

// Returns three mutants per position, one for each other base of the alphabet. The mutants are
// written in uppercase, in the alphabet of the input.
std::vector<FastaEntry> all_mutants(const FastaEntry& entry);

// Writes the wild type and the mutants of every entry of a FASTA file, in uppercase.
void run_m2(const std::string& input, const std::string& output, bool all, bool overwrite);

#endif
