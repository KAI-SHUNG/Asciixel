#include "asciixel/io/arg_parser.hpp"

namespace asciixel {

/**
 * @brief Parse CLI tokens into canonical options and positional arguments.
 *
 * @param args UTF-8 argument strings excluding the executable name.
 *
 * @return Parsed arguments or a syntax error; values remain unvalidated text.
 */
ParseResult parseArguments(const std::vector<std::string>& args)
{
    // Track whether the explicit option terminator has been consumed.
    ParsedArguments parsed;
    bool options_ended = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];

        // Preserve positional tokens, including everything following '--'.
        if (!options_ended && arg == "--") {
            options_ended = true;
            continue;
        }
        if (options_ended || arg.empty() || arg[0] != '-') {
            parsed.positional.push_back(arg);
            continue;
        }

        // Split inline values and normalize supported aliases.
        const auto equals = arg.find('=');
        const auto name = arg.substr(0, equals);
        std::string key;
        if (name == "--output" || name == "-o") key = "output";
        else if (name == "--help" || name == "-h") key = "help";
        else if (name == "--font") key = "font";
        else if (name == "--font-size") key = "font-size";
        else if (name == "--columns") key = "columns";
        else return ParseError{"Unknown option: " + name};

        // Help is a standalone request rather than a conversion option.
        if (key == "help") {
            if (equals != std::string::npos)
                return ParseError{"Help does not accept a value"};
            if (args.size() != 1)
                return ParseError{"Help must be used alone"};
            parsed.help = true;
            continue;
        }

        // Reject duplicates after alias normalization.
        if (parsed.options.count(key))
            return ParseError{"Repeated option: " + name};

        // Accept inline values or consume the next non-option token.
        if (equals != std::string::npos) {
            // Preserve empty values and any subsequent '=' for the builder.
            parsed.options.emplace(key, arg.substr(equals + 1));
        }
        else {
            // Negative numeric values are left for configuration validation.
            const bool next_is_option = i + 1 < args.size() &&
                args[i + 1].size() > 1 && args[i + 1][0] == '-' &&
                (args[i + 1][1] < '0' || args[i + 1][1] > '9');
            if (i + 1 == args.size() || next_is_option) {
                if (key == "output") {
                    parsed.options.emplace(key, std::nullopt);
                    continue;
                }
                return ParseError{"Missing option value: " + name};
            }
            parsed.options.emplace(key, args[++i]);
        }
    }

    return parsed;
}

/**
 * @brief Describe the supported CLI syntax and default option values.
 *
 * @return Static null-terminated help text; the caller does not own it.
 */
const char* argumentHelp()
{
    return "Usage: asciixel <image-path> [options]\n"
           "  -o, --output [path]     Save .png or .txt; no path: <input-stem>_asciixel.png\n"
           "                         Default file is beside input; omit -o for terminal.\n"
           "  --font <path>           Font file (default: platform font)\n"
           "  --font-size <N>         Pixel size, 1..256 (default: 24)\n"
           "  --columns <N>           Columns, 1..4096 (default: 200)\n"
           "  -h, --help              Show help; use alone\n"
           "  Values support --name=value and -o=value as well as spaces.\n"
           "  --                      End options (for input paths starting with '-')\n";
}
} // namespace asciixel
