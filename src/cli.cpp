#include "cli.hpp"

#include <algorithm>

#include "text.hpp"

const std::string PROGRAM_NAME = "fld";
static const std::string HELP_LONG = "--help";
static const std::string HELP_SHORT = "-h";

bool is_help_option(const std::string& arg) {
    return arg == HELP_LONG || arg == HELP_SHORT;
}

Command::Command(const std::string& name, const std::string& description)
    : _name(name), _description(description) {}

void Command::add_option(const std::string& name, const std::string& help) {
    OptionSpec spec;
    spec.name = name;
    spec.help = help;
    spec.required = true;
    _specs.push_back(spec);
}

void Command::add_option(const std::string& name, const std::string& help, const std::string& default_value) {
    OptionSpec spec;
    spec.name = name;
    spec.help = help;
    spec.has_default = true;
    spec.default_value = default_value;
    _specs.push_back(spec);
}

void Command::add_optional(const std::string& name, const std::string& help) {
    OptionSpec spec;
    spec.name = name;
    spec.help = help;
    _specs.push_back(spec);
}

void Command::add_flag(const std::string& name, const std::string& help) {
    OptionSpec spec;
    spec.name = name;
    spec.help = help;
    spec.is_flag = true;
    _specs.push_back(spec);
}

void Command::add_positionals(const std::string& name, const std::string& help) {
    _positional_name = name;
    _positional_help = help;
}

const OptionSpec* Command::find_spec(const std::string& name) const {
    for (const OptionSpec& spec : _specs) {
        if (spec.name == name) return &spec;
    }
    return nullptr;
}

const OptionSpec& Command::spec(const std::string& name) const {
    const OptionSpec* found = find_spec(name);
    if (found == nullptr) {
        throw std::logic_error("Unknown option " + name);
    }
    return *found;
}

void Command::parse_option(const OptionSpec& spec, const std::vector<std::string>& args, size_t& i) {
    if (spec.is_flag) {
        _flags.insert(spec.name);
        return;
    }
    if (i + 1 >= args.size()) {
        throw std::runtime_error("Option " + spec.name + " needs a value");
    }
    i++;
    _values[spec.name] = args[i];
}

static bool looks_like_option(const std::string& arg) {
    return arg.size() > 1 && arg[0] == '-';
}

void Command::parse(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        const std::string& arg = args[i];
        if (is_help_option(arg)) {
            throw HelpRequested(usage());
        }
        const OptionSpec* found = find_spec(arg);
        if (found != nullptr) {
            parse_option(*found, args, i);
        } else if (looks_like_option(arg)) {
            throw std::runtime_error("Unknown option " + arg);
        } else {
            _positionals.push_back(arg);
        }
    }
    check_required();
    check_positionals();
}

void Command::check_required() const {
    for (const OptionSpec& spec : _specs) {
        if (spec.required && _values.count(spec.name) == 0) {
            throw std::runtime_error("Option " + spec.name + " is required");
        }
    }
}

void Command::check_positionals() const {
    if (_positional_name.empty() && !_positionals.empty()) {
        throw std::runtime_error("Unexpected argument '" + _positionals.front() + "'");
    }
    if (!_positional_name.empty() && _positionals.empty()) {
        throw std::runtime_error("Expected at least one " + _positional_name);
    }
}

bool Command::has(const std::string& name) const {
    return _values.count(name) > 0 || spec(name).has_default;
}

std::string Command::value(const std::string& name) const {
    auto it = _values.find(name);
    if (it != _values.end()) {
        return it->second;
    }
    const OptionSpec& s = spec(name);
    if (!s.has_default) {
        throw std::runtime_error("Option " + name + " was not given");
    }
    return s.default_value;
}

size_t Command::size_value(const std::string& name) const {
    try {
        return parse_size(value(name));
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Option " + name + ": " + e.what());
    }
}

size_t Command::count_value(const std::string& name) const {
    try {
        return parse_count(value(name));
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Option " + name + ": " + e.what());
    }
}

bool Command::flag(const std::string& name) const {
    spec(name);
    return _flags.count(name) > 0;
}

const std::vector<std::string>& Command::positionals() const {
    return _positionals;
}

static std::string option_label(const OptionSpec& spec) {
    return spec.is_flag ? spec.name : spec.name + " <value>";
}

static std::string option_help(const OptionSpec& spec) {
    if (spec.has_default) {
        return spec.help + " (default: " + spec.default_value + ")";
    }
    if (spec.required) {
        return spec.help + " (required)";
    }
    return spec.help;
}

static std::string padded(const std::string& text, size_t width) {
    return text + std::string(width > text.size() ? width - text.size() : 0, ' ');
}

std::string Command::usage_line() const {
    std::string text = "Usage: " + PROGRAM_NAME + " " + _name + " [options]";
    if (!_positional_name.empty()) {
        text += " <" + _positional_name + ">...";
    }
    return text + "\n";
}

size_t Command::label_width() const {
    size_t width = 0;
    for (const OptionSpec& spec : _specs) {
        width = std::max(width, option_label(spec).size());
    }
    return width + 2;
}

std::string Command::options_text() const {
    std::string text = "Options:\n";
    for (const OptionSpec& spec : _specs) {
        text += "  " + padded(option_label(spec), label_width()) + option_help(spec) + "\n";
    }
    return text;
}

std::string Command::arguments_text() const {
    if (_positional_name.empty()) {
        return "";
    }
    return "\nArguments:\n  " + padded("<" + _positional_name + ">", label_width()) + _positional_help + "\n";
}

std::string Command::usage() const {
    return usage_line() + "\n" + _description + "\n\n" + options_text() + arguments_text();
}
