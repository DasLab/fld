#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cli.hpp"
#include "commands.hpp"
#include "version.hpp"

static const std::string PROGRAM_NAME = "fld";
static const std::string VERSION_OPTION = "--version";
static const std::string HELP_OPTION = "--help";

static std::string top_usage() {
    return "Usage: " + PROGRAM_NAME + " <command> [options]\n\n"
           "Commands:\n"
           "  " + PIPELINE_COMMAND + "   Design a library from FASTA files\n"
           "  " + BARCODES_COMMAND + "   Write a set of hairpin barcodes\n"
           "  " + M2_COMMAND + "         Write the single mutants of each sequence\n\n"
           "Run '" + PROGRAM_NAME + " <command> --help' for the options of a command.\n";
}

static int dispatch(const std::string& name, const std::vector<std::string>& args) {
    if (name == PIPELINE_COMMAND) return run_pipeline_command(args);
    if (name == BARCODES_COMMAND) return run_barcodes_command(args);
    if (name == M2_COMMAND) return run_m2_command(args);
    throw std::runtime_error("Unknown command '" + name + "'\n\n" + top_usage());
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args.front() == HELP_OPTION) {
        std::cout << top_usage();
        return 0;
    }
    if (args.front() == VERSION_OPTION) {
        std::cout << PROGRAM_NAME << " v" << FLD_VERSION << "\n";
        return 0;
    }
    std::vector<std::string> rest(args.begin() + 1, args.end());
    try {
        return dispatch(args.front(), rest);
    } catch (const HelpRequested& help) {
        std::cout << help.what();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
