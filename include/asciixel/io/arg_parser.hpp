#ifndef ASCIIXEL_IO_ARG_PARSER_HPP
#define ASCIIXEL_IO_ARG_PARSER_HPP

#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

namespace asciixel {

struct ParsedArguments {
    std::vector<std::string> positional;
    // Canonical long names without leading dashes; aliases normalize here.
    std::unordered_map<std::string, std::string> options;
    bool help = false;
};

struct ParseError {
    std::string message;
};

using ParseResult = std::variant<ParsedArguments, ParseError>;

// UTF-8 arguments, excluding argv[0]. Parses the currently supported image CLI.
ParseResult parseArguments(const std::vector<std::string>& args);
const char* argumentHelp();

} // namespace asciixel
#endif
