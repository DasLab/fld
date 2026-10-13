#include "doctest.hpp"
#include "verify.hpp"

#include <cmath>

// A two-construct library with the layout const,barcode,design,padding,const.
static Library sample_library() {
    Library library;
    library.layout = parse_layout("const:GG,barcode:4,design,padding,const:CC");
    Construct a;
    a.name = "a";
    a.sublibrary = "s";
    a.regions = {"GG", "ACGTTTCGACGT", "AAAA", "TT", "CC"};
    Construct b = a;
    b.name = "b";
    b.regions = {"GG", "TGCATTCGTGCA", "AAAAAA", "", "CC"};
    library.constructs = {a, b};
    return library;
}

static std::vector<std::string> sample_designs() {
    return {"AAAA", "AAAAAA"};
}

TEST_CASE("verify_library accepts a correct library") {
    CHECK_NOTHROW(verify_library(sample_library(), sample_designs(), 22));
}

TEST_CASE("verify_library rejects a wrong construct length") {
    CHECK_THROWS(verify_library(sample_library(), sample_designs(), 23));
    Library library = sample_library();
    library.constructs[1].regions[3] = "T";
    CHECK_THROWS(verify_library(library, sample_designs(), 22));
}

TEST_CASE("verify_library rejects bases outside ACGT") {
    Library library = sample_library();
    library.constructs[0].regions[3] = "TN";
    CHECK_THROWS(verify_library(library, sample_designs(), 22));
}

TEST_CASE("verify_library rejects a design that differs from its input") {
    Library library = sample_library();
    library.constructs[0].regions[2] = "AAAC";
    CHECK_THROWS(verify_library(library, sample_designs(), 22));
    CHECK_THROWS(verify_library(sample_library(), {"AAAA", "AAAAAC"}, 22));
}

TEST_CASE("verify_library rejects a changed constant region") {
    Library library = sample_library();
    library.constructs[1].regions[4] = "CG";
    CHECK_THROWS(verify_library(library, sample_designs(), 22));
}

TEST_CASE("verify_library rejects a design count mismatch") {
    CHECK_THROWS(verify_library(sample_library(), {"AAAA"}, 22));
}

TEST_CASE("require_permutation accepts permutations within groups") {
    CHECK_NOTHROW(require_permutation({1, 0, 3, 2}, single_group(4)));
    CHECK_NOTHROW(require_permutation({2, 3, 0, 1}, groups_by_key({0, 1, 0, 1})));
}

TEST_CASE("require_permutation rejects reuse, gaps, and group crossing") {
    CHECK_THROWS(require_permutation({0, 0, 2, 3}, single_group(4)));
    CHECK_THROWS(require_permutation({0, 1, 2, 4}, single_group(4)));
    CHECK_THROWS(require_permutation({1, 0, 3, 2}, groups_by_key({0, 1, 0, 1})));
}

TEST_CASE("verify_reads checks counts and finite values") {
    Library library = sample_library();
    library.read_columns = {"reads_design", "reads"};
    library.constructs[0].reads = {1.0, 2.0};
    library.constructs[1].reads = {3.0, 4.0};
    CHECK_NOTHROW(verify_reads(library));
    library.constructs[1].reads = {3.0};
    CHECK_THROWS(verify_reads(library));
    library.constructs[1].reads = {3.0, std::nan("")};
    CHECK_THROWS(verify_reads(library));
}
