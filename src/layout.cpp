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
        throw std::runtime_error("A const region needs a sequence, for example const:ACGT");
    }
    std::string seq = normalize_sequence(parameter);
    for (char base : seq) {
        if (!is_dna_base(base)) {
            throw std::runtime_error("A const region must contain only A, C, G, T, or U: " + parameter);
        }
    }
    return seq;
}

static size_t barcode_stem_length(const std::string& parameter) {
    if (parameter.empty()) {
        throw std::runtime_error("A barcode region needs a stem length, for example barcode:10");
    }
    size_t stem_length = parse_size(parameter);
    if (stem_length == 0) {
        throw std::runtime_error("A barcode stem length must be positive");
    }
    return stem_length;
}

static void require_no_parameter(const std::string& keyword, const std::string& parameter) {
    if (!parameter.empty()) {
        throw std::runtime_error("The " + keyword + " region takes no parameter");
    }
}

static Region parse_region(const std::string& token) {
    size_t colon = token.find(':');
    std::string keyword = token.substr(0, colon);
    std::string parameter = colon == std::string::npos ? "" : token.substr(colon + 1);
    if (keyword == CONSTANT_KEYWORD) {
        return Region{RegionKind::Constant, constant_sequence(parameter), 0};
    }
    if (keyword == BARCODE_KEYWORD) {
        return Region{RegionKind::Barcode, "", barcode_stem_length(parameter)};
    }
    if (keyword == DESIGN_KEYWORD) {
        require_no_parameter(keyword, parameter);
        return Region{RegionKind::Design, "", 0};
    }
    if (keyword == PADDING_KEYWORD) {
        require_no_parameter(keyword, parameter);
        return Region{RegionKind::Padding, "", 0};
    }
    throw std::runtime_error("Unknown region '" + token + "' in the construct template");
}

static size_t count_regions(const Layout& layout, RegionKind kind) {
    size_t count = 0;
    for (const Region& region : layout.regions) {
        if (region.kind == kind) count++;
    }
    return count;
}

static size_t find_region(const Layout& layout, RegionKind kind) {
    for (size_t i = 0; i < layout.regions.size(); i++) {
        if (layout.regions[i].kind == kind) return i;
    }
    throw std::logic_error("The construct template has no region of the requested kind");
}

static void validate_layout(const Layout& layout) {
    if (count_regions(layout, RegionKind::Design) != 1) {
        throw std::runtime_error("The construct template must contain exactly one design region");
    }
    if (count_regions(layout, RegionKind::Padding) > 1) {
        throw std::runtime_error("The construct template may contain at most one padding region");
    }
}

Layout parse_layout(const std::string& text) {
    Layout layout;
    for (const std::string& token : split_string(text, ',')) {
        layout.regions.push_back(parse_region(token));
    }
    validate_layout(layout);
    return layout;
}

size_t design_region(const Layout& layout) {
    return find_region(layout, RegionKind::Design);
}

bool has_padding_region(const Layout& layout) {
    return count_regions(layout, RegionKind::Padding) > 0;
}

size_t padding_region(const Layout& layout) {
    return find_region(layout, RegionKind::Padding);
}

PaddingSide padding_side(const Layout& layout) {
    if (padding_region(layout) < design_region(layout)) {
        return PaddingSide::FivePrime;
    }
    return PaddingSide::ThreePrime;
}

std::vector<size_t> barcode_regions(const Layout& layout) {
    std::vector<size_t> regions;
    for (size_t i = 0; i < layout.regions.size(); i++) {
        if (layout.regions[i].kind == RegionKind::Barcode) regions.push_back(i);
    }
    return regions;
}

size_t fixed_length(const Layout& layout) {
    size_t length = 0;
    for (const Region& region : layout.regions) {
        if (region.kind == RegionKind::Constant) length += region.constant.size();
        if (region.kind == RegionKind::Barcode) length += hairpin_length(region.stem_length);
    }
    return length;
}

static std::string region_name(const Region& region, size_t ordinal) {
    switch (region.kind) {
        case RegionKind::Constant: return CONSTANT_KEYWORD + "_" + std::to_string(ordinal);
        case RegionKind::Barcode:  return BARCODE_KEYWORD + "_" + std::to_string(ordinal);
        case RegionKind::Design:   return DESIGN_KEYWORD;
        case RegionKind::Padding:  return PADDING_KEYWORD;
    }
    throw std::logic_error("Unknown region kind");
}

std::vector<std::string> region_names(const Layout& layout) {
    std::vector<std::string> names;
    size_t constants = 0;
    size_t barcodes = 0;
    for (const Region& region : layout.regions) {
        if (region.kind == RegionKind::Constant) constants++;
        if (region.kind == RegionKind::Barcode) barcodes++;
        size_t ordinal = region.kind == RegionKind::Constant ? constants : barcodes;
        names.push_back(region_name(region, ordinal));
    }
    return names;
}
