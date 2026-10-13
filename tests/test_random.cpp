#include "doctest.hpp"
#include "random.hpp"

TEST_CASE("sample_element rejects an empty vector") {
    Rng rng = make_rng(1);
    std::vector<int> values;
    CHECK_THROWS_AS(sample_element(values, rng), std::logic_error);
    values = {4};
    CHECK(sample_element(values, rng) == 4);
}
