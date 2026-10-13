#include "assign.hpp"

#include <algorithm>
#include <map>
#include <numeric>

constexpr size_t MAX_PASSES = 100;

Groups single_group(size_t count) {
    std::vector<size_t> all(count);
    std::iota(all.begin(), all.end(), 0);
    return Groups{all};
}

Groups groups_by_key(const std::vector<size_t>& keys) {
    std::map<size_t, std::vector<size_t>> by_key;
    for (size_t i = 0; i < keys.size(); i++) {
        by_key[keys[i]].push_back(i);
    }
    Groups groups;
    for (const auto& [key, indices] : by_key) {
        groups.push_back(indices);
    }
    return groups;
}

double variance(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double total = 0.0;
    for (double v : values) {
        total += (v - mean) * (v - mean);
    }
    return total / values.size();
}

static std::vector<size_t> sorted_by(const std::vector<size_t>& indices, const std::vector<double>& values, bool ascending) {
    std::vector<size_t> sorted = indices;
    std::stable_sort(sorted.begin(), sorted.end(), [&](size_t a, size_t b) {
        return ascending ? values[a] < values[b] : values[a] > values[b];
    });
    return sorted;
}

std::vector<size_t> anti_sorted_assignment(
    const std::vector<double>& partial,
    const std::vector<double>& pool,
    const Groups& groups
) {
    std::vector<size_t> assignment(partial.size());
    for (const std::vector<size_t>& group : groups) {
        std::vector<size_t> constructs = sorted_by(group, partial, true);
        std::vector<size_t> components = sorted_by(group, pool, false);
        for (size_t i = 0; i < group.size(); i++) {
            assignment[constructs[i]] = components[i];
        }
    }
    return assignment;
}

static std::vector<double> add_assigned(
    const std::vector<double>& totals,
    const ComponentPool& pool,
    const std::vector<size_t>& assignment,
    double sign
) {
    std::vector<double> result = totals;
    for (size_t i = 0; i < result.size(); i++) {
        result[i] += sign * pool.reads[assignment[i]];
    }
    return result;
}

static std::vector<std::vector<size_t>> initial_assignments(
    const std::vector<double>& base,
    const std::vector<ComponentPool>& pools
) {
    std::vector<std::vector<size_t>> assignments;
    std::vector<double> partial = base;
    for (const ComponentPool& pool : pools) {
        assignments.push_back(anti_sorted_assignment(partial, pool.reads, pool.groups));
        partial = add_assigned(partial, pool, assignments.back(), 1.0);
    }
    return assignments;
}

// Reassigns one pool against the other pools and reports whether the variance fell.
static bool improve_pool(
    std::vector<double>& totals,
    const ComponentPool& pool,
    std::vector<size_t>& assignment
) {
    std::vector<double> partial = add_assigned(totals, pool, assignment, -1.0);
    std::vector<size_t> candidate = anti_sorted_assignment(partial, pool.reads, pool.groups);
    std::vector<double> candidate_totals = add_assigned(partial, pool, candidate, 1.0);
    if (variance(candidate_totals) >= variance(totals)) {
        return false;
    }
    totals = candidate_totals;
    assignment = candidate;
    return true;
}

std::vector<std::vector<size_t>> minimize_variance(
    const std::vector<double>& base,
    const std::vector<ComponentPool>& pools
) {
    std::vector<std::vector<size_t>> assignments = initial_assignments(base, pools);
    std::vector<double> totals = base;
    for (size_t k = 0; k < pools.size(); k++) {
        totals = add_assigned(totals, pools[k], assignments[k], 1.0);
    }
    for (size_t pass = 0; pass < MAX_PASSES; pass++) {
        bool improved = false;
        for (size_t k = 0; k < pools.size(); k++) {
            improved = improve_pool(totals, pools[k], assignments[k]) || improved;
        }
        if (!improved) break;
    }
    return assignments;
}
