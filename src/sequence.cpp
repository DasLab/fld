#include "sequence.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>

static const std::vector<char> DNA_BASES = {BASE_A, BASE_C, BASE_G, BASE_T};

// Returns the DNA bases that an IUPAC code denotes, or null for any other character.
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

std::string to_dna(const std::string& seq) {
    std::string result = seq;
    for (char& c : result) {
        if (c == 'U') c = 'T';
        if (c == 'u') c = 't';
    }
    return result;
}

std::string to_rna(const std::string& seq) {
    std::string result = seq;
    for (char& c : result) {
        if (c == 'T') c = 'U';
        if (c == 't') c = 'u';
    }
    return result;
}
