#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP

#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace asciixel {

enum class ArgType {
    Flag,
    Option,
    Positional
};

struct Argument {
    ArgType                    type;
    std::string                name;
    std::optional<std::string> alias;
    std::optional<std::string> default_value;
    std::string                description;
    bool                       required = false;

    Argument& set_default(const std::string& default_val);
    Argument& set_description(const std::string& desc);
    Argument& set_required(bool req);
};

class ArgParser {
private:
    std::vector<std::shared_ptr<Argument>>                     registery;
    std::vector<std::shared_ptr<Argument>>                     positional_registery;
    std::unordered_map<std::string, std::shared_ptr<Argument>> name_to_arg;
    std::unordered_map<std::string, std::string>               parsed;

    std::string program_name = "";
    std::string note         = "";

    bool isRegistered(const std::string& name) const;
    void checkRequiredArguments() const;

    static std::vector<std::string> normalizeArgs(int argc, char** argv);
#ifdef _WIN32
    /**
     * @brief Encode one Windows command-line argument as UTF-8.
     *
     * @param value Non-null, null-terminated UTF-16 argument.
     *
     * @return UTF-8 string without its terminating null byte.
     */
    static std::string              toUtf8(const wchar_t* value);
    static std::vector<std::string> normalizeArgs(int argc, wchar_t** argv);
#endif

public:
    /**
     * @brief Set the program name for usage messages.
     */
    void setProgramName(const std::string& name)
    { program_name = name; }

    /**
     * @brief Set a note to be displayed after the help message.
     */
    void setNote(const std::string& n)
    { note = n; }

    /**
     * @brief Add a new argument to the parser, suggest chain calls to set its properties.
     *
     * @param name The name without leading dashes; must be unique.
     * @param alias The optional alias without leading dashes; must be unique.
     * @param type The type of the argument.
     *
     * @return A reference to the newly added argument.
     */
    Argument& add_argument(
        const std::string&                name,
        const std::optional<std::string>& alias = std::nullopt,
        ArgType                           type  = ArgType::Positional);

    /**
     * @brief Check if an argument has been provided.
     * Both the argument name and its alias are supported.
     * @param name The name or alias of the argument to check.
     */
    bool has(const std::string& name) const;

    /**
     * @brief Display help message for all registered arguments.
     */
    void help() const;

    /**
     * @brief
     * Get the value of an argument.
     * If the argument is registered but not provided,
     * return the default value if set, otherwise throw an exception.
     * @param name alias name are not supported.
     */
    template <typename T>
    T get(const std::string& name) const;

    /**
     * @brief Parse command-line arguments.
     */
    void Parse(int argc, char** argv);

#ifdef _WIN32
    /**
     * @brief Parse command-line arguments (Windows version).
     */
    void Parse(int argc, wchar_t** argv);
#endif

    /**
     * @brief Parse normalized command-line arguments.
     */
    void Parse(const std::vector<std::string>& args);
};

} // namespace asciixel

// Implementation of template methods.

template <typename T>
T asciixel::ArgParser::get(const std::string& name) const
{
    // Handle the case where the argument is not registered.
    if (!isRegistered(name)) {
        throw std::invalid_argument("Argument not registered: " + name);
    }

    std::string arg;
    // Handle the case where the argument is registered but not provided.
    if (!has(name)) {
        if (name_to_arg.at(name)->default_value.has_value()) {
            arg = name_to_arg.at(name)->default_value.value();
        }
        else {
            throw std::invalid_argument("Argument not provided: " + name);
        }
    }
    // Handle the case where the argument is registered and provided.
    else {
        arg = parsed.at(name);
    }

    T                 result;
    std::stringstream ss(arg);

    if (!(ss >> result)) {
        throw std::invalid_argument(
            "Failed to convert argument '" + name + "' with value '" + arg + "'");
    }
    // Check for any remaining characters in the stream after reading the value
    // For example, want a integer, input is "123abc" -> 123 is read, but "abc" remains
    ss >> std::ws;
    if (!ss.eof()) {
        throw std::invalid_argument(
            "Invalid value for argument '" + name + "': '" + arg + "'");
    }
    return result;
}

template <>
inline std::string asciixel::ArgParser::get<std::string>(const std::string& name) const
{
    // Handle the case where the argument is not registered.
    if (!isRegistered(name)) {
        throw std::invalid_argument("Argument not registered: " + name);
    }

    // Handle the case where the argument is registered but not provided.
    if (!has(name)) {
        if (name_to_arg.at(name)->default_value.has_value()) {
            return name_to_arg.at(name)->default_value.value();
        }
        else {
            throw std::invalid_argument("Argument not provided: " + name);
        }
    }

    return parsed.at(name);
}

template <>
inline bool asciixel::ArgParser::get<bool>(const std::string& name) const
{
    // Handle the case where the argument is not registered.
    if (!isRegistered(name)) {
        throw std::invalid_argument("Argument not registered: " + name);
    }

    // Handle the case where the argument is registered but not provided.
    if (!has(name)) {
        if (name_to_arg.at(name)->default_value.has_value()) {
            return name_to_arg.at(name)->default_value.value() == "true";
        }
        else {
            throw std::invalid_argument("Argument not provided: " + name);
        }
    }

    return parsed.at(name) == "true";
}

#endif // ARG_PARSER_HPP
