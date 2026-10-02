#include "asciixel/io/arg_parser.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace asciixel {

/// Argument class member functions

Argument& Argument::set_default(const std::string& default_val)
{
    default_value = default_val;
    return *this;
}

Argument& Argument::set_description(const std::string& desc)
{
    description = desc;
    return *this;
}

Argument& Argument::set_required(bool req)
{
    required = req;
    return *this;
}

// ArgParser member functions

bool ArgParser::is_registered(const std::string& name) const
{
    return name_to_arg.count(name) > 0;
}

void ArgParser::check_required_arguments() const
{
    for (const auto& arg : registery) {
        if (arg->required && !has(arg->name)) {
            throw std::invalid_argument("Required argument not provided: " + arg->name);
        }
    }
}

std::vector<std::string> ArgParser::normalize_args(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(argc > 1 ? argc - 1 : 0);

    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    return args;
}

#ifdef _WIN32
std::string ArgParser::to_utf8(const wchar_t* value)
{
    // Query the required buffer length, including the terminating null byte.
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           value, -1, nullptr, 0, nullptr, nullptr);
    if (length == 0) {
        throw std::runtime_error("Cannot encode image path as UTF-8");
    }

    // Encode into owned storage and remove the API's null terminator.
    std::string utf8(static_cast<std::size_t>(length), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
                            utf8.data(), length, nullptr, nullptr)
        == 0) {
        throw std::runtime_error("Cannot encode image path as UTF-8");
    }
    utf8.pop_back();
    return utf8;
}

std::vector<std::string> ArgParser::normalize_args(int argc, wchar_t** argv)
{
    std::vector<std::string> args;
    args.reserve(argc > 1 ? argc - 1 : 0);

    for (int i = 1; i < argc; ++i) {
        args.emplace_back(to_utf8(argv[i]));
    }

    return args;
}
#endif

Argument& ArgParser::add_argument(
    const std::string&                name,
    const std::optional<std::string>& alias,
    ArgType                           type)
{
    // Validate the argument name and alias.
    if (name.empty()) {
        throw std::invalid_argument("Argument name must not be empty");
    }
    // Validate that the argument name and alias are not already registered.
    if (is_registered(name)) {
        throw std::invalid_argument("Argument already registered: " + name);
    }
    if (alias.has_value() && is_registered(alias.value())) {
        throw std::invalid_argument("Argument already registered: " + alias.value());
    }

    auto arg   = std::make_shared<Argument>();
    arg->name  = name;
    arg->alias = alias;
    arg->type  = type;
    registery.push_back(arg);

    // Add the argument to the name-to-argument map for both the canonical name and the alias (if provided).
    name_to_arg[name] = arg;
    if (alias.has_value()) {
        name_to_arg[alias.value()] = arg;
    }

    // If the argument is positional, add it to the positional registry.
    if (type == ArgType::Positional) {
        positional_registery.push_back(arg);
    }

    return *arg;
}

bool ArgParser::has(const std::string& name) const
{
    if (!is_registered(name)) {
        return false;
    }

    std::string canonical_name = name_to_arg.at(name)->name;
    return parsed.count(canonical_name) > 0;
}

void ArgParser::help() const
{
    std::string usage = "Usage: " + program_name + " [options]";
    for (const auto& arg : positional_registery) {
        usage += " <" + arg->name + "> ";
    }
    std::cout << usage << "\n\n";
    std::cout << "Options:\n";
    for (const auto& arg : registery) {
        std::string option_str = "  --" + arg->name;
        if (arg->alias.has_value()) {
            option_str += ", -" + arg->alias.value();
        }
        if (arg->type == ArgType::Option) {
            option_str += " <value>";
        }
        std::cout << option_str << "\n      " << arg->description << "\n";
    }
    if (!note.empty()) {
        std::cout << "\n"
                  << note << "\n";
    }
}

void ArgParser::parse(int argc, char** argv)
{
    auto args = normalize_args(argc, argv);
    parse(args);
}

#ifdef _WIN32
void ArgParser::parse(int argc, wchar_t** argv)
{
    auto args = normalize_args(argc, argv);
    parse(args);
}
#endif

void ArgParser::parse(const std::vector<std::string>& args)
{
    bool        options_ended    = false;
    std::size_t positional_index = 0;
    std::string arg;

    for (std::size_t i = 0; i < args.size(); ++i) {
        arg = args[i];

        // Handle the special case of "--" which indicates the end of options.
        if (!options_ended && arg == "--") {
            options_ended = true;
            continue;
        }

        // If options have ended or the argument does not start with a dash, treat it as a positional argument.
        if (options_ended || arg.empty() || arg[0] != '-') {
            if (positional_index >= positional_registery.size()) {
                throw std::invalid_argument("Unexpected positional argument: " + arg);
            }

            auto positional_arg          = positional_registery[positional_index++];
            parsed[positional_arg->name] = arg;
            continue;
        }

        // Determine the start index for the argument name, skipping leading dashes.
        int start_index = 1;
        if (arg.size() > 1 && arg[1] == '-') {
            start_index = 2;
        }
        // Check if the argument contains an equals sign, which indicates a key-value pair.
        bool equals_sign = arg.find('=') != std::string::npos;

        // Extract the argument name and check if it is registered.
        std::string name;
        name = arg.substr(
            start_index,
            (equals_sign ? arg.find('=') : arg.size()) - start_index);
        if (!is_registered(name)) {
            throw std::invalid_argument("Unrecognized argument: " + arg);
        }
        // Check if the argument name or alias has already been provided.
        if (has(name)) {
            throw std::invalid_argument("Argument already provided: " + arg);
        }

        std::string value;
        auto        argument = name_to_arg.at(name);
        if (argument->type == ArgType::Positional) {
            throw std::invalid_argument("Positional argument cannot be provided as an option: " + arg);
        }
        // Extract the argument value.
        if (argument->type == ArgType::Flag) {
            if (equals_sign) {
                throw std::invalid_argument("Flag argument cannot have a value: " + arg);
            }

            value = "true";
        }
        else if (argument->type == ArgType::Option) {
            if (equals_sign) {
                value = arg.substr(arg.find('=') + 1);
                if (value.empty()) {
                    throw std::invalid_argument("Missing value for argument: " + arg);
                }
            }
            else {
                if (i + 1 >= args.size() || args[i + 1].empty() || args[i + 1][0] == '-') {
                    throw std::invalid_argument("Missing value for argument: " + arg);
                }
                value = args[++i];
            }
        }
        parsed[argument->name] = value;
    }

    // Check for required arguments after parsing all inputs.
    check_required_arguments();
};

} // namespace asciixel
