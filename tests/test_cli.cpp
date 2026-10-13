#include "doctest.hpp"
#include "cli.hpp"
#include "text.hpp"

static Command sample_command() {
    Command command("sample", "A sample command.");
    command.add_positionals("file", "Input files");
    command.add_option("-o", "Output");
    command.add_option("--count", "Count", "5");
    command.add_optional("--seed", "Seed");
    command.add_flag("--overwrite", "Overwrite");
    return command;
}

TEST_CASE("Command parses options, flags, and positionals") {
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--overwrite", "b.fasta", "--seed", "7"});
    CHECK(command.positionals() == std::vector<std::string>{"a.fasta", "b.fasta"});
    CHECK(command.value("-o") == "out");
    CHECK(command.count_value("--count") == 5);
    CHECK(command.has("--seed"));
    CHECK(command.count_value("--seed") == 7);
    CHECK(command.flag("--overwrite"));
}

TEST_CASE("Command reports missing and unknown arguments") {
    CHECK_THROWS(sample_command().parse({"a.fasta"}));
    CHECK_THROWS(sample_command().parse({"-o", "out"}));
    CHECK_THROWS(sample_command().parse({"a.fasta", "-o"}));
    CHECK_THROWS(sample_command().parse({"a.fasta", "-o", "out", "--bogus"}));
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "out"});
    CHECK_FALSE(command.has("--seed"));
    CHECK_FALSE(command.flag("--overwrite"));
}

TEST_CASE("Command rejects negative and malformed counts") {
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--count", "-5"});
    CHECK_THROWS(command.count_value("--count"));
    command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--count", "unlimited"});
    CHECK(command.count_value("--count") == UNLIMITED);
}

TEST_CASE("size_value rejects unlimited") {
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--seed", "unlimited"});
    CHECK_THROWS(command.size_value("--seed"));
    CHECK(command.count_value("--seed") == UNLIMITED);
    command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--seed", "12"});
    CHECK(command.size_value("--seed") == 12);
}

TEST_CASE("Command throws HelpRequested on --help and -h") {
    CHECK_THROWS_AS(sample_command().parse({"--help"}), HelpRequested);
    CHECK_THROWS_AS(sample_command().parse({"-h"}), HelpRequested);
    CHECK(is_help_option("-h"));
    CHECK(is_help_option("--help"));
    CHECK_FALSE(is_help_option("-o"));
}
