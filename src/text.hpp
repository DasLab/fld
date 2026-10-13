#ifndef FLD_TEXT_HPP
#define FLD_TEXT_HPP

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

// Sentinel for a count with no upper limit.
constexpr size_t UNLIMITED = std::numeric_limits<size_t>::max();
const std::string UNLIMITED_TEXT = "unlimited";

std::vector<std::string> split_string(const std::string& text, char separator);

// Parses a non-negative decimal integer. Throws on any other input.
size_t parse_size(const std::string& text);

// Parses a non-negative decimal integer or the word "unlimited".
size_t parse_count(const std::string& text);

std::string count_text(size_t value);

#endif
