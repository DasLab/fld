#ifndef FLD_COMMANDS_HPP
#define FLD_COMMANDS_HPP

#include <string>
#include <vector>

const std::string PIPELINE_COMMAND = "pipeline";
const std::string BARCODES_COMMAND = "barcodes";
const std::string M2_COMMAND = "m2";

// Each function parses its arguments, runs the subcommand, and returns the exit code.
int run_pipeline_command(const std::vector<std::string>& args);
int run_barcodes_command(const std::vector<std::string>& args);
int run_m2_command(const std::vector<std::string>& args);

#endif
