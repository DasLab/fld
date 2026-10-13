#ifndef FLD_FASTA_HPP
#define FLD_FASTA_HPP

#include <ostream>
#include <string>
#include <vector>

struct FastaEntry {
    std::string name;
    std::string sequence;
};

// Reads every entry of a FASTA file. Throws on a missing file, a sequence before the first
// header, or an entry with an empty sequence.
std::vector<FastaEntry> read_fasta(const std::string& path);

void write_fasta_entry(std::ostream& out, const FastaEntry& entry);

#endif
