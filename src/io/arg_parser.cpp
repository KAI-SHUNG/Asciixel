#include "asciixel/io/arg_parser.hpp"

namespace asciixel {

ParseResult parseArguments(const std::vector<std::string>& args)
{
    ParsedArguments parsed;
    bool options_ended = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (!options_ended && arg == "--") {
            options_ended = true;
            continue;
        }
        if (options_ended || arg.empty() || arg[0] != '-') {
            parsed.positional.push_back(arg);
            continue;
        }
        const auto equals = arg.find('=');
        const auto name = arg.substr(0, equals);
        std::string key;
        if (name == "--output" || name == "-o") key = "output";
        else if (name == "--help" || name == "-h") key = "help";
        else if (name == "--format") key = "format";
        else if (name == "--font") key = "font";
        else if (name == "--font-size") key = "font-size";
        else if (name == "--columns") key = "columns";
        else return ParseError{"Unknown option: " + name};

        if (key == "help") {
            if (equals != std::string::npos)
                return ParseError{"Help does not accept a value"};
            if (args.size() != 1)
                return ParseError{"Help must be used alone"};
            parsed.help = true;
            continue;
        }
        if (parsed.options.count(key))
            return ParseError{"Repeated option: " + name};
        if (equals != std::string::npos) {
            // Preserve empty values and any subsequent '=' for the builder.
            parsed.options.emplace(key, arg.substr(equals + 1));
        } else {
            if (i + 1 == args.size())
                return ParseError{"Missing option value: " + name};
            const auto& value = args[i + 1];
            // '-' and negative numbers remain values. Dash-prefixed paths can
            // always be supplied unambiguously using --name=value.
            if (value.size() > 1 && value[0] == '-' &&
                (value[1] < '0' || value[1] > '9'))
                return ParseError{"Missing option value: " + name};
            parsed.options.emplace(key, args[++i]);
        }
    }
    return parsed;
}

const char* argumentHelp()
{
    return "Usage: asciixel <image-path> [options]\n"
           "  --format terminal|png   File encoding; terminal is a compatibility option\n"
           "  -o, --output <path>         File destination; infer PNG from .png (case-insensitive)\n"
           "  --font <path>           Font file (default: platform font)\n"
           "  --font-size <N>         Pixel size, 1..256 (default: 24)\n"
           "  --columns <N>           Columns, 1..4096 (default: 200)\n"
           "  -h, --help              Show help; use alone\n"
           "  Values support --name=value and -o=value as well as spaces.\n"
           "  --                      End options (for input paths starting with '-')\n";
}
} // namespace asciixel
