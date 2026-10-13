#ifndef FLD_LAYOUT_HPP
#define FLD_LAYOUT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "padding.hpp"

enum class SlotKind { Constant, Barcode, Design, Padding };

struct Slot {
    SlotKind kind;
    std::string constant;
    size_t stem_length = 0;
};

// The construct from 5' to 3' as an ordered list of slots.
struct Layout {
    std::vector<Slot> slots;
};

// Parses a comma-separated list of slots such as "const:ACGT,barcode:10,design,padding".
Layout parse_layout(const std::string& text);

size_t design_slot(const Layout& layout);
bool has_padding_slot(const Layout& layout);
size_t padding_slot(const Layout& layout);
PaddingSide padding_side(const Layout& layout);
std::vector<size_t> barcode_slots(const Layout& layout);

// Sums the lengths of the constant and barcode slots.
size_t fixed_length(const Layout& layout);

// Names each slot by kind and ordinal, for example const_1, barcode_2, design, padding.
std::vector<std::string> slot_names(const Layout& layout);

#endif
