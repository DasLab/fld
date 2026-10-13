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

// Consumes one character inside a quoted field and reports whether the field is still quoted.
static bool consume_quoted(const std::string& line, size_t& i, std::string& current) {
    if (line[i] != '"') {
        current += line[i];
        return true;
    }
    if (i + 1 < line.size() && line[i + 1] == '"') {
        current += '"';
        i++;
        return true;
    }
    return false;
}

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); i++) {
        if (quoted) {
            quoted = consume_quoted(line, i, current);
        } else if (line[i] == '"') {
            quoted = true;
        } else if (line[i] == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current += line[i];
        }
    }
    fields.push_back(current);
    return fields;
}
