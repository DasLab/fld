#ifndef FLD_LAYOUT_HPP
#define FLD_LAYOUT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "padding.hpp"

enum class RegionKind { Constant, Barcode, Design, Padding };

struct Region {
    RegionKind kind;
    std::string constant;
    size_t stem_length = 0;
};

// The construct from 5' to 3' as an ordered list of regions.
struct Layout {
    std::vector<Region> regions;
};

// Parses a comma-separated list of regions such as "const:ACGT,barcode:10,design,padding".
Layout parse_layout(const std::string& text);

size_t design_region(const Layout& layout);
bool has_padding_region(const Layout& layout);
size_t padding_region(const Layout& layout);
PaddingSide padding_side(const Layout& layout);
std::vector<size_t> barcode_regions(const Layout& layout);

// Sums the lengths of the constant and barcode regions.
size_t fixed_length(const Layout& layout);

// Names each region by kind and ordinal, for example const_1, barcode_2, design, padding.
std::vector<std::string> region_names(const Layout& layout);

#endif
