#include "sequence.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

static const std::vector<char> DNA_BASES = {BASE_A, BASE_C, BASE_G, BASE_T};

// Returns the DNA bases that an IUPAC code denotes in alphabetical order, or null for any
// other character.
static const char* polybase_options(char code) {
    switch (code) {
        case 'A': return "A";
        case 'C': return "C";
        case 'G': return "G";
        case 'T': return "T";
        case 'R': return "AG";
        case 'Y': return "CT";
        case 'K': return "GT";
        case 'M': return "AC";
        case 'S': return "CG";
        case 'W': return "AT";
        case 'V': return "ACG";
        case 'D': return "AGT";
        case 'H': return "ACT";
        case 'B': return "CGT";
        case 'N': return "ACGT";
        default:  return nullptr;
    }
}

bool is_dna_base(char base) {
    return base == BASE_A || base == BASE_C || base == BASE_G || base == BASE_T;
}

const std::vector<char>& dna_bases() {
    return DNA_BASES;
}

std::string uppercase(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string normalize_sequence(const std::string& raw) {
    std::string seq = uppercase(raw);
    for (size_t i = 0; i < seq.size(); i++) {
        if (seq[i] == BASE_U) {
            seq[i] = BASE_T;
        }
        if (polybase_options(seq[i]) == nullptr) {
            throw std::runtime_error(
                "Invalid character '" + std::string(1, raw[i]) + "' at position " +
                std::to_string(i + 1));
        }
    }
    return seq;
}

std::string resolve_polybases(const std::string& seq, Rng& rng) {
    std::string result = seq;
    for (char& c : result) {
        const char* options = polybase_options(c);
        if (options == nullptr) {
            throw std::runtime_error("Invalid character '" + std::string(1, c) + "' in sequence");
        }
        c = options[sample_range(0, std::strlen(options) - 1, rng)];
    }
    return result;
}

std::string random_sequence(size_t length, Rng& rng) {
    std::string seq(length, BASE_A);
    for (char& c : seq) {
        c = sample_element(DNA_BASES, rng);
    }
    return seq;
}

static const std::string IUPAC_CODES = "ACGTRYKMSWVDHBN";

static std::string complement_options(const char* options) {
    std::string complements;
    for (const char* c = options; *c != '\0'; c++) {
        complements += complement(*c);
    }
    std::sort(complements.begin(), complements.end());
    return complements;
}

bool code_allows(char code, char base) {
    const char* options = polybase_options(code);
    return options != nullptr && std::strchr(options, base) != nullptr;
}

char complement_code(char code) {
    const char* options = polybase_options(code);
    if (options == nullptr) {
        throw std::runtime_error("Cannot complement '" + std::string(1, code) + "'");
    }
    std::string complements = complement_options(options);
    for (char candidate : IUPAC_CODES) {
        if (polybase_options(candidate) == complements) return candidate;
    }
    throw std::logic_error("No IUPAC code denotes " + complements);
}

char complement(char base) {
    switch (base) {
        case BASE_A: return BASE_T;
        case BASE_C: return BASE_G;
        case BASE_G: return BASE_C;
        case BASE_T: return BASE_A;
        default:
            throw std::runtime_error("Cannot complement '" + std::string(1, base) + "'");
    }
}
