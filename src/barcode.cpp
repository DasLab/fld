#include "barcode.hpp"

#include <algorithm>
#include <stdexcept>

#include "sequence.hpp"

static const std::string SUBSTITUTION_BASES = "ACGT";
constexpr size_t MAX_BARCODE_ATTEMPTS = 100000;

std::vector<std::string> hamming_ball(const std::string& seq) {
    std::vector<std::string> ball = {seq};
    for (size_t i = 0; i < seq.size(); i++) {
        for (char base : SUBSTITUTION_BASES) {
            if (base == seq[i]) continue;
            std::string variant = seq;
            variant[i] = base;
            ball.push_back(variant);
        }
    }
    return ball;
}

bool has_hamming_neighbor(const std::string& seq, const std::unordered_set<std::string>& existing) {
    for (const std::string& variant : hamming_ball(seq)) {
        if (existing.count(variant) > 0) {
            return true;
        }
    }
    return false;
}

using CountTable = std::vector<std::vector<size_t>>;

static CountTable zero_table(size_t rows, size_t columns) {
    return CountTable(rows, std::vector<size_t>(columns, 0));
}

// Counts the ways to extend the previous table by one pair of each kind.
static size_t extended_count(const CountTable& prev, size_t au, size_t gc) {
    size_t count = prev[au][gc];
    if (au > 0) count = saturating_add(count, prev[au - 1][gc]);
    if (gc > 0) count = saturating_add(count, prev[au][gc - 1]);
    return count;
}

// Counts pair-kind sequences of the given length with at most max_au AU pairs, max_gc GC
// pairs, and max_gu GU pairs. Entry [au][gc] of the result holds the count for exactly that
// many AU and GC pairs.
static CountTable count_kind_sequences(size_t length, size_t max_au, size_t max_gc, size_t max_gu) {
    CountTable prev = zero_table(max_au + 1, max_gc + 1);
    prev[0][0] = 1;
    for (size_t i = 1; i <= length; i++) {
        CountTable cur = zero_table(max_au + 1, max_gc + 1);
        for (size_t au = 0; au <= max_au; au++) {
            for (size_t gc = 0; gc <= max_gc; gc++) {
                if (au + gc > i || i - au - gc > max_gu) continue;
                cur[au][gc] = extended_count(prev, au, gc);
            }
        }
        prev = cur;
    }
    return prev;
}

static size_t sum_table(const CountTable& table, size_t length, size_t max_gu) {
    size_t total = 0;
    for (size_t au = 0; au < table.size(); au++) {
        for (size_t gc = 0; gc < table[au].size(); gc++) {
            if (au + gc > length || length - au - gc > max_gu) continue;
            total = saturating_add(total, table[au][gc]);
        }
    }
    return total;
}

static size_t orientation_count(size_t stem_length) {
    if (stem_length >= 64) return UNLIMITED;
    return size_t(1) << stem_length;
}

static size_t count_stems(size_t stem_length, const StemLimits& limits) {
    size_t free_pairs = stem_length - limits.closing_gc;
    size_t max_au = std::min(limits.max_au, free_pairs);
    size_t max_gc = std::min(limits.max_gc - limits.closing_gc, free_pairs);
    size_t max_gu = std::min(limits.max_gu, free_pairs);
    CountTable table = count_kind_sequences(free_pairs, max_au, max_gc, max_gu);
    size_t kinds = sum_table(table, free_pairs, max_gu);
    return saturating_mul(kinds, orientation_count(stem_length));
}

size_t count_hairpins(size_t stem_length, const StemLimits& limits) {
    validate_stem_length(stem_length, limits);
    return saturating_mul(count_stems(stem_length, limits), tetraloop_count());
}

static std::string random_unique_barcode(
    size_t stem_length,
    const StemLimits& limits,
    const std::unordered_set<std::string>& existing,
    Rng& rng
) {
    for (size_t attempt = 0; attempt < MAX_BARCODE_ATTEMPTS; attempt++) {
        std::string barcode = random_hairpin(stem_length, limits, rng);
        if (!has_hamming_neighbor(barcode, existing)) {
            return barcode;
        }
    }
    throw std::runtime_error(
        "Could not find another barcode at Hamming distance two or more from the existing ones after " +
        std::to_string(MAX_BARCODE_ATTEMPTS) + " attempts. Increase the stem length or relax the pair limits.");
}

static void require_capacity(size_t count, size_t stem_length, const StemLimits& limits) {
    size_t capacity = count_hairpins(stem_length, limits);
    if (capacity < count) {
        throw std::runtime_error(
            "Only " + std::to_string(capacity) + " distinct hairpins have a stem of length " +
            std::to_string(stem_length) + " under the pair limits, but " + std::to_string(count) +
            " barcodes are required. Increase the stem length or relax the pair limits.");
    }
}

void require_hamming_separated(const std::vector<std::string>& barcodes) {
    std::unordered_set<std::string> seen;
    for (const std::string& barcode : barcodes) {
        if (has_hamming_neighbor(barcode, seen)) {
            throw std::runtime_error(
                "Barcode " + barcode + " is within Hamming distance one of another barcode");
        }
        seen.insert(barcode);
    }
}

std::vector<std::string> random_barcodes(
    size_t count,
    size_t stem_length,
    const StemLimits& limits,
    Rng& rng
) {
    require_capacity(count, stem_length, limits);
    std::vector<std::string> barcodes;
    std::unordered_set<std::string> existing;
    while (barcodes.size() < count) {
        std::string barcode = random_unique_barcode(stem_length, limits, existing, rng);
        existing.insert(barcode);
        barcodes.push_back(barcode);
    }
    require_hamming_separated(barcodes);
    return barcodes;
}
