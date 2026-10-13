#ifndef FLD_LIBRARY_HPP
#define FLD_LIBRARY_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "layout.hpp"

// One construct. It holds one sequence per region of the layout.
struct Construct {
    size_t index = 0;
    std::string name;
    std::string sublibrary;
    std::vector<std::string> regions;
    std::vector<double> reads;
};

struct Library {
    Layout layout;
    std::vector<Construct> constructs;
    std::vector<std::string> read_columns;
};

std::string full_sequence(const Construct& construct);

size_t design_length(const Layout& layout, const Construct& construct);

// 0-based positions of the design in the full sequence. The end is exclusive, so the design is
// full_sequence[begin:end] in Python notation.
size_t design_begin(const Layout& layout, const Construct& construct);
size_t design_end(const Layout& layout, const Construct& construct);

// Computes the padding length that brings the construct to the given total length. Throws
// when the other regions alone exceed that length.
size_t padding_length(const Layout& layout, const Construct& construct, size_t construct_length);

// Builds the FASTA header: the name, then sublibrary, begin, and end as key=value fields.
std::string fasta_name(const Layout& layout, const Construct& construct);

std::vector<std::string> csv_header(const Library& library);
std::vector<std::string> csv_row(const Construct& construct);

void write_library_csv(const Library& library, const std::string& path);
void write_library_fasta(const Library& library, const std::string& path);

#endif
