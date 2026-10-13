#include "predict.hpp"

#include <sys/wait.h>

#include <cstdlib>
#include <filesystem>
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

// The files of one rn-coverage run, named relative to the work directory.
struct PredictionFiles {
    std::string text;
    std::string tokens;
    std::string prediction_dir;
    std::string prediction;
    std::string reads;
};

static PredictionFiles prediction_files(const std::string& label) {
    PredictionFiles files;
    files.text = label + ".txt";
    files.tokens = label + "_tokens.h5";
    files.prediction_dir = label + "_predictions";
    files.prediction = join_path(files.prediction_dir, files.tokens);
    files.reads = label + "_reads.txt";
    return files;
}

// Removes the output files of an earlier run, because rn-coverage appends to existing files.
static void remove_earlier_outputs(const std::string& work_dir, const PredictionFiles& files) {
    for (const std::string& name : {files.tokens, files.prediction, files.reads}) {
        std::filesystem::remove(join_path(work_dir, name));
    }
}

// Predicts sequences of one common length through tokenize, predict, and extract. The files
// are named after the label and live in the work directory.
static std::vector<double> predict_equal_length(
    const std::vector<std::string>& sequences,
    const std::string& work_dir,
    const std::string& label
) {
    PredictionFiles files = prediction_files(label);
    remove_earlier_outputs(work_dir, files);
    write_lines(join_path(work_dir, files.text), sequences);
    run_command(work_dir, std::string(RN_COVERAGE) + " tokenize " + shell_quote(files.text) + " " + shell_quote(files.tokens));
    run_command(work_dir, std::string(RN_COVERAGE) + " predict " + shell_quote(files.tokens) + " -o " + shell_quote(files.prediction_dir));
    run_command(work_dir, std::string(RN_COVERAGE) + " extract " + shell_quote(files.prediction) + " " + shell_quote(files.reads));
    return read_reads_file(join_path(work_dir, files.reads), sequences.size());
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
