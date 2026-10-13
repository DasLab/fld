#include "fasta.hpp"

#include <stdexcept>

#include "files.hpp"

static bool is_header(const std::string& line) {
    return !line.empty() && line[0] == '>';
}

static void require_sequence(const FastaEntry& entry, const std::string& path) {
    if (entry.sequence.empty()) {
        throw std::runtime_error("Entry '" + entry.name + "' in '" + path + "' has no sequence");
    }
}

static void add_sequence_line(std::vector<FastaEntry>& entries, const std::string& line, const std::string& path) {
    if (entries.empty()) {
        throw std::runtime_error("'" + path + "' has sequence data before the first header");
    }
    entries.back().sequence += line;
}

std::vector<FastaEntry> read_fasta(const std::string& path) {
    require_file(path);
    std::vector<FastaEntry> entries;
    for (const std::string& line : read_lines(path)) {
        if (line.empty()) continue;
        if (is_header(line)) {
            entries.push_back(FastaEntry{line.substr(1), ""});
        } else {
            add_sequence_line(entries, line, path);
        }
    }
    for (const FastaEntry& entry : entries) {
        require_sequence(entry, path);
    }
    return entries;
}

void write_fasta_entry(std::ostream& out, const FastaEntry& entry) {
    out << ">" << entry.name << "\n" << entry.sequence << "\n";
}
