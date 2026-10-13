#ifndef FLD_SEQUENCE_HPP
#define FLD_SEQUENCE_HPP

#include <string>
#include <vector>

#include "random.hpp"

constexpr char BASE_A = 'A';
constexpr char BASE_C = 'C';
constexpr char BASE_G = 'G';
constexpr char BASE_T = 'T';
constexpr char BASE_U = 'U';

// Returns true for A, C, G, and T.
bool is_dna_base(char base);

const std::vector<char>& dna_bases();

std::string uppercase(const std::string& text);

// Uppercases the sequence, converts U to T, and rejects characters that are not IUPAC codes.
std::string normalize_sequence(const std::string& raw);

// Replaces each IUPAC code with a random base that the code denotes. The input must be normalized.
std::string resolve_polybases(const std::string& seq, Rng& rng);

std::string random_sequence(size_t length, Rng& rng);

// Returns the Watson-Crick complement of a DNA base.
char complement(char base);

// Returns true if the IUPAC code denotes the base.
bool code_allows(char code, char base);

// Returns the IUPAC code that denotes the complements of the bases the code denotes.
char complement_code(char code);

#endif
