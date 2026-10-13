#ifndef FLD_CSV_HPP
#define FLD_CSV_HPP

#include <string>
#include <vector>

// Quotes a field when it contains a comma, a quote, or a line break.
std::string csv_field(const std::string& value);

std::string csv_line(const std::vector<std::string>& fields);

#endif
