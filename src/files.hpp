#ifndef FLD_FILES_HPP
#define FLD_FILES_HPP

#include <fstream>
#include <string>
#include <vector>

// Throws if the path is not a regular file.
void require_file(const std::string& path);

// Throws if the path exists and overwriting is not allowed.
void require_writable(const std::string& path, bool overwrite);

// Opens a file for writing and throws if that fails.
std::ofstream open_output(const std::string& path);

std::string join_path(const std::string& directory, const std::string& name);

// Returns the file name without directory and extension.
std::string file_stem(const std::string& path);

void write_lines(const std::string& path, const std::vector<std::string>& lines);

// Reads a file as lines without their line endings.
std::vector<std::string> read_lines(const std::string& path);

#endif
