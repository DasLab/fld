#include "text.hpp"

#include <stdexcept>

std::vector<std::string> split_string(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == separator) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

size_t parse_size(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("Expected a non-negative integer, got '" + text + "'");
    }
    try {
        return std::stoull(text);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Integer is too large: " + text);
    }
}

size_t parse_count(const std::string& text) {
    if (text == UNLIMITED_TEXT) {
        return UNLIMITED;
    }
    return parse_size(text);
}

std::string count_text(size_t value) {
    if (value == UNLIMITED) {
        return UNLIMITED_TEXT;
    }
    return std::to_string(value);
}
