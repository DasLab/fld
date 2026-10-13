#include "stem.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "sequence.hpp"

struct BasePair {
    char five;
    char three;
};

enum class PairKind { AU, GC, GU };

struct PairBudget {
    size_t au;
    size_t gc;
    size_t gu;
};

static const std::vector<std::string> TETRALOOPS = {"TTCG", "GTGA", "TACG"};
static const std::vector<BasePair> AU_PAIRS = {{BASE_A, BASE_T}, {BASE_T, BASE_A}};
static const std::vector<BasePair> GC_PAIRS = {{BASE_G, BASE_C}, {BASE_C, BASE_G}};
static const std::vector<BasePair> GU_PAIRS = {{BASE_G, BASE_T}, {BASE_T, BASE_G}};

size_t saturating_add(size_t a, size_t b) {
    return a > UNLIMITED - b ? UNLIMITED : a + b;
}

size_t saturating_mul(size_t a, size_t b) {
    if (a == 0 || b == 0) return 0;
    return a > UNLIMITED / b ? UNLIMITED : a * b;
}

static size_t pair_capacity(const StemLimits& limits) {
    return saturating_add(saturating_add(limits.max_au, limits.max_gc), limits.max_gu);
}

void validate_stem_length(size_t stem_length, const StemLimits& limits) {
    if (stem_length < SHORTEST_STEM) {
        throw std::runtime_error(
            "Stem length " + std::to_string(stem_length) + " is shorter than the minimum of " +
            std::to_string(SHORTEST_STEM));
    }
    if (limits.closing_gc > stem_length) {
        throw std::runtime_error(
            "The closing GC count " + std::to_string(limits.closing_gc) +
            " exceeds the stem length " + std::to_string(stem_length));
    }
    if (limits.closing_gc > limits.max_gc) {
        throw std::runtime_error(
            "The closing GC count " + std::to_string(limits.closing_gc) +
            " exceeds the maximum GC count " + count_text(limits.max_gc));
    }
    if (pair_capacity(limits) < stem_length) {
        throw std::runtime_error(
            "The pair limits allow at most " + std::to_string(pair_capacity(limits)) +
            " pairs, but the stem length is " + std::to_string(stem_length));
    }
}

void validate_stem_limits(const StemLimits& limits) {
    if (limits.max_length < limits.min_length) {
        throw std::runtime_error(
            "The maximum stem length " + std::to_string(limits.max_length) +
            " is less than the minimum stem length " + std::to_string(limits.min_length));
    }
    validate_stem_length(limits.min_length, limits);
    validate_stem_length(limits.max_length, limits);
}

size_t hairpin_length(size_t stem_length) {
    return 2 * stem_length + HAIRPIN_LOOP_LENGTH;
}

size_t tetraloop_count() {
    return TETRALOOPS.size();
}

static PairKind pair_kind(const BasePair& pair) {
    bool has_g = pair.five == BASE_G || pair.three == BASE_G;
    bool has_c = pair.five == BASE_C || pair.three == BASE_C;
    if (has_g && has_c) return PairKind::GC;
    if (has_g) return PairKind::GU;
    return PairKind::AU;
}

static void spend(size_t& budget) {
    if (budget != UNLIMITED) {
        budget--;
    }
}

static void charge(PairBudget& budget, const BasePair& pair) {
    switch (pair_kind(pair)) {
        case PairKind::AU: spend(budget.au); break;
        case PairKind::GC: spend(budget.gc); break;
        case PairKind::GU: spend(budget.gu); break;
    }
}

static std::vector<BasePair> affordable_pairs(const PairBudget& budget) {
    std::vector<BasePair> pairs;
    if (budget.au > 0) pairs.insert(pairs.end(), AU_PAIRS.begin(), AU_PAIRS.end());
    if (budget.gc > 0) pairs.insert(pairs.end(), GC_PAIRS.begin(), GC_PAIRS.end());
    if (budget.gu > 0) pairs.insert(pairs.end(), GU_PAIRS.begin(), GU_PAIRS.end());
    return pairs;
}

static BasePair random_pair(PairBudget& budget, Rng& rng) {
    BasePair pair = sample_element(affordable_pairs(budget), rng);
    charge(budget, pair);
    return pair;
}

static std::vector<BasePair> random_stem(size_t stem_length, const StemLimits& limits, Rng& rng) {
    PairBudget budget{limits.max_au, limits.max_gc, limits.max_gu};
    std::vector<BasePair> pairs;
    for (size_t i = 0; i < limits.closing_gc; i++) {
        pairs.push_back(sample_element(GC_PAIRS, rng));
        spend(budget.gc);
    }
    while (pairs.size() < stem_length) {
        pairs.push_back(random_pair(budget, rng));
    }
    std::shuffle(pairs.begin() + limits.closing_gc, pairs.end(), rng);
    return pairs;
}

static std::string join_hairpin(const std::vector<BasePair>& pairs, const std::string& loop) {
    std::string sense;
    std::string antisense;
    for (const BasePair& pair : pairs) {
        sense += pair.five;
        antisense.insert(antisense.begin(), pair.three);
    }
    return sense + loop + antisense;
}

std::string random_hairpin(size_t stem_length, const StemLimits& limits, Rng& rng) {
    validate_stem_length(stem_length, limits);
    std::vector<BasePair> pairs = random_stem(stem_length, limits, rng);
    return join_hairpin(pairs, sample_element(TETRALOOPS, rng));
}
