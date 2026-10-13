#include "doctest.hpp"
#include "cli.hpp"
#include "text.hpp"

static Command sample_command() {
    Command command("sample", "A sample command.");
    command.add_positionals("file", "Input files");
    command.add_option("--output", "Output");
    command.add_short_name("--output", "-o");
    command.add_option("--count", "Count", "5");
    command.add_optional("--seed", "Seed");
    command.add_flag("--overwrite", "Overwrite");
    return command;
}

TEST_CASE("Command parses options, flags, and positionals") {
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "out", "--overwrite", "b.fasta", "--seed", "7"});
    CHECK(command.positionals() == std::vector<std::string>{"a.fasta", "b.fasta"});
    CHECK(command.value("--output") == "out");
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

TEST_CASE("Command rejects an option as the value of another option") {
    CHECK_THROWS_WITH(sample_command().parse({"a.fasta", "-o", "--overwrite"}), doctest::Contains("--overwrite"));
    CHECK_THROWS_WITH(sample_command().parse({"a.fasta", "-o", "--help"}), doctest::Contains("needs a value"));
    Command command = sample_command();
    command.parse({"a.fasta", "-o", "-weird"});
    CHECK(command.value("--output") == "-weird");
}

TEST_CASE("Command rejects a repeated option or flag") {
    CHECK_THROWS_WITH(sample_command().parse({"a.fasta", "-o", "x", "-o", "y"}), doctest::Contains("more than once"));
    CHECK_THROWS_WITH(sample_command().parse({"a.fasta", "-o", "x", "--output", "y"}), doctest::Contains("more than once"));
    CHECK_THROWS_WITH(sample_command().parse({"a.fasta", "-o", "x", "--overwrite", "--overwrite"}), doctest::Contains("more than once"));
}

static Command single_command() {
    Command command("single", "Takes one file.");
    command.add_positional("file", "Input file");
    return command;
}

TEST_CASE("Command with one positional accepts exactly one") {
    CHECK_THROWS_WITH(single_command().parse({}), doctest::Contains("Expected one file"));
    CHECK_THROWS_WITH(single_command().parse({"a", "b"}), doctest::Contains("got 2"));
    Command command = single_command();
    command.parse({"a"});
    CHECK(command.positional() == "a");
    CHECK(command.usage().find("<file>\n") != std::string::npos);
    CHECK(command.usage().find("<file>...") == std::string::npos);
}

TEST_CASE("Command accepts the long and short name of an option") {
    Command command = sample_command();
    command.parse({"a.fasta", "--output", "out"});
    CHECK(command.value("--output") == "out");
    CHECK(command.usage().find("-o, --output <value>") != std::string::npos);
}
