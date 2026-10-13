#include "verify.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

#include "sequence.hpp"

static std::string describe(const Construct& construct) {
    return "construct '" + construct.name + "' in " + construct.sublibrary;
}

static void require_length(const Construct& construct, size_t construct_length) {
    size_t length = full_sequence(construct).size();
    if (length != construct_length) {
        throw std::runtime_error(
            "Verification failed: " + describe(construct) + " has " + std::to_string(length) +
            " bases instead of " + std::to_string(construct_length));
    }
}

static void require_dna_bases(const Construct& construct) {
    for (char base : full_sequence(construct)) {
        if (!is_dna_base(base)) {
            throw std::runtime_error(
                "Verification failed: " + describe(construct) + " contains '" + std::string(1, base) + "'");
        }
    }
}

static void require_design(const Layout& layout, const Construct& construct, const std::string& expected) {
    const std::string& design = construct.parts[design_slot(layout)];
    if (design != expected) {
        throw std::runtime_error(
            "Verification failed: the design of " + describe(construct) + " differs from its input");
    }
    size_t begin = design_begin(layout, construct);
    size_t end = design_end(layout, construct);
    if (full_sequence(construct).substr(begin, end - begin) != design) {
        throw std::runtime_error(
            "Verification failed: the design of " + describe(construct) + " is not at begin:end");
    }
}

static void require_constants(const Layout& layout, const Construct& construct) {
    for (size_t i = 0; i < layout.slots.size(); i++) {
        if (layout.slots[i].kind != SlotKind::Constant) continue;
        if (construct.parts[i] != layout.slots[i].constant) {
            throw std::runtime_error(
                "Verification failed: constant slot " + slot_names(layout)[i] + " of " +
                describe(construct) + " does not match the layout");
        }
    }
}

void verify_library(
    const Library& library,
    const std::vector<std::string>& designs,
    size_t construct_length
) {
    if (designs.size() != library.constructs.size()) {
        throw std::logic_error("Verification failed: the design count does not match the library");
    }
    for (size_t i = 0; i < library.constructs.size(); i++) {
        const Construct& construct = library.constructs[i];
        require_length(construct, construct_length);
        require_dna_bases(construct);
        require_design(library.layout, construct, designs[i]);
        require_constants(library.layout, construct);
    }
}

void require_permutation(const std::vector<size_t>& assignment, const Groups& groups) {
    std::vector<bool> used(assignment.size(), false);
    for (const std::vector<size_t>& group : groups) {
        std::unordered_set<size_t> members(group.begin(), group.end());
        for (size_t i : group) {
            size_t component = assignment[i];
            if (component >= used.size() || used[component] || members.count(component) == 0) {
                throw std::logic_error("Verification failed: a component is reused, missing, or outside its group");
            }
            used[component] = true;
        }
    }
    if (std::count(used.begin(), used.end(), false) > 0) {
        throw std::logic_error("Verification failed: a component is not assigned");
    }
}

void verify_reads(const Library& library) {
    for (const Construct& construct : library.constructs) {
        if (construct.reads.size() != library.read_columns.size()) {
            throw std::logic_error("Verification failed: " + describe(construct) + " has the wrong number of read values");
        }
        for (double reads : construct.reads) {
            if (!std::isfinite(reads)) {
                throw std::runtime_error("Verification failed: " + describe(construct) + " has a non-finite read value");
            }
        }
    }
}
