#include "doctest.hpp"
#include "assign.hpp"
#include "random.hpp"

#include <random>

#include <algorithm>
#include <numeric>

static std::vector<double> totals(const std::vector<double>& base, const std::vector<ComponentPool>& pools,
                                  const std::vector<std::vector<size_t>>& assignments) {
    std::vector<double> result = base;
    for (size_t k = 0; k < pools.size(); k++) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] += pools[k].reads[assignments[k][i]];
        }
    }
    return result;
}

static bool is_permutation_within(const std::vector<size_t>& assignment, const Groups& groups) {
    for (const std::vector<size_t>& group : groups) {
        std::vector<size_t> assigned;
        for (size_t i : group) assigned.push_back(assignment[i]);
        std::sort(assigned.begin(), assigned.end());
        std::vector<size_t> expected = group;
        std::sort(expected.begin(), expected.end());
        if (assigned != expected) return false;
    }
    return true;
}

TEST_CASE("anti_sorted_assignment pairs low with high") {
    std::vector<double> partial = {3, 1, 2};
    std::vector<double> pool = {10, 30, 20};
    std::vector<size_t> assignment = anti_sorted_assignment(partial, pool, single_group(3));
    CHECK(assignment == std::vector<size_t>{0, 1, 2});
    std::vector<double> paired = {partial[0] + pool[assignment[0]], partial[1] + pool[assignment[1]], partial[2] + pool[assignment[2]]};
    std::vector<double> sorted_pairing = {3 + 30, 1 + 10, 2 + 20};
    CHECK(variance(paired) < variance(sorted_pairing));
}

TEST_CASE("anti_sorted_assignment stays within groups") {
    std::vector<double> partial = {1, 5, 2, 6};
    std::vector<double> pool = {10, 40, 20, 30};
    Groups groups = groups_by_key({0, 1, 0, 1});
    std::vector<size_t> assignment = anti_sorted_assignment(partial, pool, groups);
    CHECK(is_permutation_within(assignment, groups));
    CHECK(assignment[0] == 2);
    CHECK(assignment[2] == 0);
    CHECK(assignment[1] == 1);
    CHECK(assignment[3] == 3);
}

TEST_CASE("minimize_variance with one pool is the anti-sorted assignment") {
    std::vector<double> base = {5, 1, 3, 2, 4};
    ComponentPool pool{{1, 2, 3, 4, 5}, single_group(5)};
    std::vector<std::vector<size_t>> assignments = minimize_variance(base, {pool});
    std::vector<double> sums = totals(base, {pool}, assignments);
    for (double s : sums) CHECK(s == doctest::Approx(6));
}

TEST_CASE("minimize_variance with several pools does not exceed the sequential variance") {
    Rng rng = make_rng(12);
    std::uniform_real_distribution<double> dist(0, 100);
    size_t n = 200;
    std::vector<double> base(n);
    for (double& v : base) v = dist(rng);
    std::vector<ComponentPool> pools;
    std::vector<size_t> keys(n);
    for (size_t i = 0; i < n; i++) keys[i] = i % 3;
    for (int k = 0; k < 3; k++) {
        ComponentPool pool;
        pool.reads.resize(n);
        for (double& v : pool.reads) v = dist(rng);
        pool.groups = k == 0 ? groups_by_key(keys) : single_group(n);
        pools.push_back(pool);
    }
    std::vector<std::vector<size_t>> assignments = minimize_variance(base, pools);
    for (size_t k = 0; k < pools.size(); k++) {
        CHECK(is_permutation_within(assignments[k], pools[k].groups));
    }
    std::vector<std::vector<size_t>> identity(3, std::vector<size_t>(n));
    for (auto& a : identity) std::iota(a.begin(), a.end(), 0);
    CHECK(variance(totals(base, pools, assignments)) < variance(totals(base, pools, identity)));
    // Every pool is anti-sorted against the rest, so no single reassignment helps.
    std::vector<double> sums = totals(base, pools, assignments);
    for (size_t k = 0; k < pools.size(); k++) {
        std::vector<double> partial = sums;
        for (size_t i = 0; i < n; i++) partial[i] -= pools[k].reads[assignments[k][i]];
        std::vector<size_t> again = anti_sorted_assignment(partial, pools[k].reads, pools[k].groups);
        std::vector<double> candidate = partial;
        for (size_t i = 0; i < n; i++) candidate[i] += pools[k].reads[again[i]];
        CHECK(variance(candidate) >= variance(sums) - 1e-9);
    }
}
