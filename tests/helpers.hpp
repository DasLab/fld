#ifndef FLD_TEST_HELPERS_HPP
#define FLD_TEST_HELPERS_HPP

#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "csv.hpp"
#include "files.hpp"
#include "stem.hpp"

// Stem limits with every field set, so that no test depends on the program defaults.
inline StemLimits test_limits() {
    StemLimits limits;
    limits.min_length = 7;
    limits.max_length = 13;
    limits.max_au = UNLIMITED;
    limits.max_gc = 5;
    limits.max_gu = 0;
    limits.terminal_gc = 1;
    limits.spacer = 2;
    return limits;
}

// A temporary directory that is removed on destruction.
class TempDir {
public:
    TempDir() {
        std::random_device device;
        _path = std::filesystem::temp_directory_path() / ("fld_test_" + std::to_string(device()));
        std::filesystem::create_directories(_path);
    }
    ~TempDir() { std::filesystem::remove_all(_path); }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    std::string file(const std::string& name) const { return (_path / name).string(); }
private:
    std::filesystem::path _path;
};

inline void write_fasta_file(const std::string& path, const std::vector<std::pair<std::string, std::string>>& entries) {
    std::ofstream file(path);
    for (const auto& [name, seq] : entries) {
        file << ">" << name << "\n" << seq << "\n";
    }
}

// Consumes one character inside a quoted field and reports whether the field is still quoted.
inline bool consume_quoted(const std::string& line, size_t& i, std::string& current) {
    if (line[i] != '"') {
        current += line[i];
        return true;
    }
    if (i + 1 < line.size() && line[i + 1] == '"') {
        current += '"';
        i++;
        return true;
    }
    return false;
}

inline std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); i++) {
        if (quoted) {
            quoted = consume_quoted(line, i, current);
        } else if (line[i] == '"') {
            quoted = true;
        } else if (line[i] == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current += line[i];
        }
    }
    fields.push_back(current);
    return fields;
}

inline std::vector<std::vector<std::string>> read_csv_file(const std::string& path) {
    std::vector<std::vector<std::string>> rows;
    for (const std::string& line : read_lines(path)) {
        rows.push_back(parse_csv_line(line));
    }
    return rows;
}

inline size_t column_index(const std::vector<std::string>& header, const std::string& name) {
    for (size_t i = 0; i < header.size(); i++) {
        if (header[i] == name) return i;
    }
    throw std::runtime_error("No column " + name);
}

// Reads an integer key=value field from a FASTA header.
inline size_t header_value(const std::string& header, const std::string& key) {
    size_t start = header.find(" " + key + "=");
    if (start == std::string::npos) throw std::runtime_error("No field " + key);
    start += key.size() + 2;
    return std::stoull(header.substr(start, header.find(' ', start) - start));
}

#endif
