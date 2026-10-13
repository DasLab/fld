#include "doctest.hpp"
#include "layout.hpp"
#include "stem.hpp"

TEST_CASE("parse_layout reads every region kind") {
    Layout layout = parse_layout("const:acgu,barcode:10,design,padding,barcode:8,const:TTTT");
    REQUIRE(layout.regions.size() == 6);
    CHECK(layout.regions[0].kind == RegionKind::Constant);
    CHECK(layout.regions[0].constant == "ACGT");
    CHECK(layout.regions[1].kind == RegionKind::Barcode);
    CHECK(layout.regions[1].stem_length == 10);
    CHECK(layout.regions[2].kind == RegionKind::Design);
    CHECK(layout.regions[3].kind == RegionKind::Padding);
    CHECK(design_region(layout) == 2);
    CHECK(padding_region(layout) == 3);
    CHECK(padding_side(layout) == PaddingSide::ThreePrime);
    CHECK(barcode_regions(layout) == std::vector<size_t>{1, 4});
    CHECK(fixed_length(layout) == 4 + hairpin_length(10) + hairpin_length(8) + 4);
    CHECK(region_names(layout) == std::vector<std::string>{
        "const_1", "barcode_1", "design", "padding", "barcode_2", "const_2"});
}

TEST_CASE("parse_layout rejects invalid layouts") {
    CHECK_THROWS(parse_layout("const:ACGT"));
    CHECK_THROWS(parse_layout("design,design"));
    CHECK_THROWS(parse_layout("padding,design,padding"));
    CHECK_THROWS(parse_layout("const,design"));
    CHECK_THROWS(parse_layout("const:ACNT,design"));
    CHECK_THROWS(parse_layout("barcode,design"));
    CHECK_THROWS(parse_layout("barcode:0,design"));
    CHECK_THROWS(parse_layout("barcode:x,design"));
    CHECK_THROWS(parse_layout("design:1"));
    CHECK_THROWS(parse_layout("spacer,design"));
    CHECK_THROWS(parse_layout(""));
}

TEST_CASE("a layout without padding reports so") {
    Layout layout = parse_layout("padding,design");
    CHECK(has_padding_region(layout));
    CHECK(padding_side(layout) == PaddingSide::FivePrime);
    CHECK_FALSE(has_padding_region(parse_layout("design")));
}
