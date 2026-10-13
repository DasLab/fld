#include "layout.hpp"

#include <stdexcept>

#include "sequence.hpp"
#include "stem.hpp"
#include "text.hpp"

static const std::string CONSTANT_KEYWORD = "const";
static const std::string BARCODE_KEYWORD = "barcode";
static const std::string DESIGN_KEYWORD = "design";
static const std::string PADDING_KEYWORD = "padding";

static std::string constant_sequence(const std::string& parameter) {
    if (parameter.empty()) {
        throw std::runtime_error("A const slot needs a sequence, for example const:ACGT");
    }
    std::string seq = normalize_sequence(parameter);
    for (char base : seq) {
        if (!is_dna_base(base)) {
            throw std::runtime_error("A const slot must contain only A, C, G, T, or U: " + parameter);
        }
    }
    return seq;
}

static size_t barcode_stem_length(const std::string& parameter) {
    if (parameter.empty()) {
        throw std::runtime_error("A barcode slot needs a stem length, for example barcode:10");
    }
    size_t stem_length = parse_size(parameter);
    if (stem_length == 0) {
        throw std::runtime_error("A barcode stem length must be positive");
    }
    return stem_length;
}

static void require_no_parameter(const std::string& keyword, const std::string& parameter) {
    if (!parameter.empty()) {
        throw std::runtime_error("The " + keyword + " slot takes no parameter");
    }
}

static Slot parse_slot(const std::string& token) {
    size_t colon = token.find(':');
    std::string keyword = token.substr(0, colon);
    std::string parameter = colon == std::string::npos ? "" : token.substr(colon + 1);
    if (keyword == CONSTANT_KEYWORD) {
        return Slot{SlotKind::Constant, constant_sequence(parameter), 0};
    }
    if (keyword == BARCODE_KEYWORD) {
        return Slot{SlotKind::Barcode, "", barcode_stem_length(parameter)};
    }
    if (keyword == DESIGN_KEYWORD) {
        require_no_parameter(keyword, parameter);
        return Slot{SlotKind::Design, "", 0};
    }
    if (keyword == PADDING_KEYWORD) {
        require_no_parameter(keyword, parameter);
        return Slot{SlotKind::Padding, "", 0};
    }
    throw std::runtime_error("Unknown slot '" + token + "' in layout");
}

static size_t count_slots(const Layout& layout, SlotKind kind) {
    size_t count = 0;
    for (const Slot& slot : layout.slots) {
        if (slot.kind == kind) count++;
    }
    return count;
}

static size_t find_slot(const Layout& layout, SlotKind kind) {
    for (size_t i = 0; i < layout.slots.size(); i++) {
        if (layout.slots[i].kind == kind) return i;
    }
    throw std::logic_error("The layout has no slot of the requested kind");
}

static void validate_layout(const Layout& layout) {
    if (count_slots(layout, SlotKind::Design) != 1) {
        throw std::runtime_error("The layout must contain exactly one design slot");
    }
    if (count_slots(layout, SlotKind::Padding) > 1) {
        throw std::runtime_error("The layout may contain at most one padding slot");
    }
}

Layout parse_layout(const std::string& text) {
    Layout layout;
    for (const std::string& token : split_string(text, ',')) {
        layout.slots.push_back(parse_slot(token));
    }
    validate_layout(layout);
    return layout;
}

size_t design_slot(const Layout& layout) {
    return find_slot(layout, SlotKind::Design);
}

bool has_padding_slot(const Layout& layout) {
    return count_slots(layout, SlotKind::Padding) > 0;
}

size_t padding_slot(const Layout& layout) {
    return find_slot(layout, SlotKind::Padding);
}

PaddingSide padding_side(const Layout& layout) {
    if (padding_slot(layout) < design_slot(layout)) {
        return PaddingSide::FivePrime;
    }
    return PaddingSide::ThreePrime;
}

std::vector<size_t> barcode_slots(const Layout& layout) {
    std::vector<size_t> slots;
    for (size_t i = 0; i < layout.slots.size(); i++) {
        if (layout.slots[i].kind == SlotKind::Barcode) slots.push_back(i);
    }
    return slots;
}

size_t fixed_length(const Layout& layout) {
    size_t length = 0;
    for (const Slot& slot : layout.slots) {
        if (slot.kind == SlotKind::Constant) length += slot.constant.size();
        if (slot.kind == SlotKind::Barcode) length += hairpin_length(slot.stem_length);
    }
    return length;
}

static std::string slot_name(const Slot& slot, size_t ordinal) {
    switch (slot.kind) {
        case SlotKind::Constant: return CONSTANT_KEYWORD + "_" + std::to_string(ordinal);
        case SlotKind::Barcode:  return BARCODE_KEYWORD + "_" + std::to_string(ordinal);
        case SlotKind::Design:   return DESIGN_KEYWORD;
        case SlotKind::Padding:  return PADDING_KEYWORD;
    }
    throw std::logic_error("Unknown slot kind");
}

std::vector<std::string> slot_names(const Layout& layout) {
    std::vector<std::string> names;
    size_t constants = 0;
    size_t barcodes = 0;
    for (const Slot& slot : layout.slots) {
        if (slot.kind == SlotKind::Constant) constants++;
        if (slot.kind == SlotKind::Barcode) barcodes++;
        size_t ordinal = slot.kind == SlotKind::Constant ? constants : barcodes;
        names.push_back(slot_name(slot, ordinal));
    }
    return names;
}
