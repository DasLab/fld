#include "files.hpp"

#include <filesystem>
#include <stdexcept>

void require_file(const std::string& path) {
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("The file '" + path + "' does not exist");
    }
}

void require_writable(const std::string& path, bool overwrite) {
    if (!overwrite && std::filesystem::exists(path)) {
        throw std::runtime_error("'" + path + "' already exists. Use --overwrite to replace it.");
    }
}

std::ofstream open_output(const std::string& path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot write to '" + path + "'");
    }
    return file;
}

std::string join_path(const std::string& directory, const std::string& name) {
    return (std::filesystem::path(directory) / name).string();
}

std::string file_stem(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}

void write_lines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream file = open_output(path);
    for (const std::string& line : lines) {
        file << line << "\n";
    }
}

static void strip_line_ending(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot read '" + path + "'");
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        strip_line_ending(line);
        lines.push_back(line);
    }
    return lines;
}
