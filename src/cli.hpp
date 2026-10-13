#ifndef FLD_CLI_HPP
#define FLD_CLI_HPP

#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

extern const std::string PROGRAM_NAME;

// Returns true for --help and -h.
bool is_help_option(const std::string& arg);

// Thrown when the user asks for help. The message is the usage text.
class HelpRequested : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct OptionSpec {
    std::string name;
    std::string short_name;
    std::string help;
    bool is_flag = false;
    bool required = false;
    bool has_default = false;
    std::string default_value;
};

// The options of one subcommand.
class Command {
public:
    Command(const std::string& name, const std::string& description);

    // Adds an option that must be given.
    void add_option(const std::string& name, const std::string& help);

    // Adds an option with a default value.
    void add_option(const std::string& name, const std::string& help, const std::string& default_value);

    // Adds an option that may be absent.
    void add_optional(const std::string& name, const std::string& help);

    void add_flag(const std::string& name, const std::string& help);

    // Gives an option a second, short name such as -o. Values are stored under the long name.
    void add_short_name(const std::string& name, const std::string& short_name);

    // Accepts exactly one positional argument.
    void add_positional(const std::string& name, const std::string& help);

    // Accepts one or more positional arguments.
    void add_positionals(const std::string& name, const std::string& help);

    void parse(const std::vector<std::string>& args);

    bool has(const std::string& name) const;
    std::string value(const std::string& name) const;

    // Parses the value as a non-negative integer.
    size_t size_value(const std::string& name) const;

    // Parses the value as a count, which accepts "unlimited".
    size_t count_value(const std::string& name) const;

    bool flag(const std::string& name) const;
    std::string positional() const;
    const std::vector<std::string>& positionals() const;

    std::string usage() const;

private:
    std::string _name;
    std::string _description;
    std::string _positional_name;
    std::string _positional_help;
    bool _single_positional = false;
    std::vector<OptionSpec> _specs;
    std::map<std::string, std::string> _values;
    std::set<std::string> _flags;
    std::vector<std::string> _positionals;

    const OptionSpec* find_spec(const std::string& name) const;
    const OptionSpec& spec(const std::string& name) const;
    void require_unseen(const OptionSpec& spec) const;
    std::string option_value(const OptionSpec& spec, const std::vector<std::string>& args, size_t i) const;
    void parse_option(const OptionSpec& spec, const std::vector<std::string>& args, size_t& i);
    void check_required() const;
    void check_positionals() const;
    std::string usage_line() const;
    size_t label_width() const;
    std::string options_text() const;
    std::string arguments_text() const;
};

#endif
