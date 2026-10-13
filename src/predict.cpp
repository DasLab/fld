#include "predict.hpp"

#include <sys/wait.h>

#include <cstdlib>
#include <map>
#include <stdexcept>

#include "files.hpp"
#include "progress.hpp"

// rn-coverage is called as three commands: `tokenize <txt> <h5>`, `predict <h5> -o <dir>`, and
// `extract <h5> <txt>`. Every sequence in one tokenize call must have the same length, since
// tokenize sizes its array from the first line. predict writes its output under <dir> with the
// name of the token file, and it writes a lightning_logs directory into the working directory,
// so every command runs inside the work directory with file names relative to it.

// A character array, so that the probe can run during static initialization.
static const char RN_COVERAGE[] = "rn-coverage";

static std::string shell_quote(const std::string& text) {
    std::string quoted = "'";
    for (char c : text) {
        quoted += c == '\'' ? "'\\''" : std::string(1, c);
    }
    return quoted + "'";
}

// Runs the command inside the directory, where rn-coverage also writes its own logs.
static void run_command(const std::string& directory, const std::string& command) {
    progress() << "  $ " << command << "\n" << std::flush;
    int status = std::system(("cd " + shell_quote(directory) + " && " + command).c_str());
    if (status == -1) {
        throw std::runtime_error("Could not start the command: " + command);
    }
    if (WIFSIGNALED(status)) {
        throw std::runtime_error("Command interrupted by signal " + std::to_string(WTERMSIG(status)));
    }
    if (WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Command failed: " + command);
    }
}

bool rn_coverage_on_path() {
    return std::system((std::string("command -v ") + RN_COVERAGE + " > /dev/null 2>&1").c_str()) == 0;
}

static std::vector<double> read_reads_file(const std::string& path, size_t expected) {
    std::vector<std::string> lines = read_lines(path);
    if (lines.size() != expected) {
        throw std::runtime_error(
            "'" + path + "' has " + std::to_string(lines.size()) + " lines, expected " +
            std::to_string(expected));
    }
    std::vector<double> reads;
    for (const std::string& line : lines) {
        try {
            reads.push_back(std::stod(line));
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid number '" + line + "' in '" + path + "'");
        }
    }
    return reads;
}

// Predicts sequences of one common length through tokenize, predict, and extract. The files
// are named after the label and live in the work directory.
static std::vector<double> predict_equal_length(
    const std::vector<std::string>& sequences,
    const std::string& work_dir,
    const std::string& label
) {
    std::string text_name = label + ".txt";
    std::string tokens_name = label + "_tokens.h5";
    std::string prediction_dir = label + "_predictions";
    std::string prediction_name = join_path(prediction_dir, tokens_name);
    std::string reads_name = label + "_reads.txt";
    write_lines(join_path(work_dir, text_name), sequences);
    run_command(work_dir, std::string(RN_COVERAGE) + " tokenize " + shell_quote(text_name) + " " + shell_quote(tokens_name));
    run_command(work_dir, std::string(RN_COVERAGE) + " predict " + shell_quote(tokens_name) + " -o " + shell_quote(prediction_dir));
    run_command(work_dir, std::string(RN_COVERAGE) + " extract " + shell_quote(prediction_name) + " " + shell_quote(reads_name));
    return read_reads_file(join_path(work_dir, reads_name), sequences.size());
}

static std::map<size_t, std::vector<size_t>> group_by_length(const std::vector<std::string>& sequences) {
    std::map<size_t, std::vector<size_t>> groups;
    for (size_t i = 0; i < sequences.size(); i++) {
        groups[sequences[i].size()].push_back(i);
    }
    return groups;
}

static std::vector<std::string> select(const std::vector<std::string>& sequences, const std::vector<size_t>& indices) {
    std::vector<std::string> selected;
    for (size_t i : indices) {
        selected.push_back(sequences[i]);
    }
    return selected;
}

std::vector<double> predict_reads(
    const std::vector<std::string>& sequences,
    const std::string& work_dir,
    const std::string& label
) {
    std::map<size_t, std::vector<size_t>> groups = group_by_length(sequences);
    progress() << "Predicting reads for " << label << ": " << sequences.size()
              << " sequences in " << groups.size() << " length groups\n";
    std::vector<double> reads(sequences.size(), 0.0);
    for (const auto& [length, indices] : groups) {
        if (length == 0) continue;
        std::string group_label = label + "_" + std::to_string(length);
        std::vector<double> group_reads = predict_equal_length(select(sequences, indices), work_dir, group_label);
        for (size_t i = 0; i < indices.size(); i++) {
            reads[indices[i]] = group_reads[i];
        }
    }
    return reads;
}
