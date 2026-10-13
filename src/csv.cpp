#include "csv.hpp"

static bool needs_quotes(const std::string& value) {
    return value.find_first_of(",\"\n\r") != std::string::npos;
}

std::string csv_field(const std::string& value) {
    if (!needs_quotes(value)) {
        return value;
    }
    std::string quoted = "\"";
    for (char c : value) {
        quoted += c == '"' ? "\"\"" : std::string(1, c);
    }
    return quoted + "\"";
}

std::string csv_line(const std::vector<std::string>& fields) {
    std::string line;
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0) line += ",";
        line += csv_field(fields[i]);
    }
    return line;
}
