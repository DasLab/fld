#ifndef FLD_ASSIGN_HPP
#define FLD_ASSIGN_HPP

#include <cstddef>
#include <vector>

// Index sets. A candidate at index i may only be assigned to constructs in the group of i.
using Groups = std::vector<std::vector<size_t>>;

// A pool of one candidate per construct with its predicted reads.
struct CandidatePool {
    std::vector<double> reads;
    Groups groups;
};

Groups single_group(size_t count);

// Groups indices that share a key.
Groups groups_by_key(const std::vector<size_t>& keys);

double variance(const std::vector<double>& values);

// Pairs the constructs with the lowest partial reads with the candidates with the highest
// reads within each group. Returns the pool index assigned to each construct.
std::vector<size_t> anti_sorted_assignment(
    const std::vector<double>& partial,
    const std::vector<double>& pool,
    const Groups& groups
);

// Assigns every pool to minimize the variance of the summed reads. Each pool is assigned
// against the sum of the base reads and the other pools, and the passes repeat until no
// pool can lower the variance. Returns one assignment per pool.
std::vector<std::vector<size_t>> minimize_variance(
    const std::vector<double>& base,
    const std::vector<CandidatePool>& pools
);

#endif
