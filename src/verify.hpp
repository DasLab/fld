#ifndef FLD_VERIFY_HPP
#define FLD_VERIFY_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "assign.hpp"
#include "library.hpp"

// Checks every assembled construct and throws on the first violation: the construct length,
// the bases, the design against its input, and the constant regions.
void verify_library(
    const Library& library,
    const std::vector<std::string>& designs,
    size_t construct_length
);

// Throws unless the assignment uses every candidate exactly once within its group.
void require_permutation(const std::vector<size_t>& assignment, const Groups& groups);

// Throws unless every construct has one finite value per read column.
void verify_reads(const Library& library);

#endif
