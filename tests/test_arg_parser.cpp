#include "asciixel/io/arg_parser.hpp"
#include "asciixel/config/config_builder.hpp"
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

void requireAt(bool ok, int line) {
    if (!ok) {
        std::fprintf(stderr, "argument parser assertion failed at line %d\n", line);
        std::exit(1);
    }
}
#define require(ok) requireAt((ok), __LINE__)

asciixel::Config buildParsedConfig(const asciixel::ParsedArguments& arguments) {
    if (arguments.help || arguments.positional.size() != 1)
        throw std::invalid_argument("Expected one input");
    auto values = arguments.options;
    values.emplace("input", arguments.positional.front());
    return asciixel::buildConfig(values);
}

// Exercise the complete CLI-to-config pipeline for existing behavior checks.
std::variant<asciixel::Config, asciixel::ParseError> parseConfig(
    const std::vector<std::string>& args) {
    using namespace asciixel;
    const auto parsed = parseArguments(args);
    if (const auto* error = std::get_if<ParseError>(&parsed)) return *error;
    try {
        return resolveConfig(buildParsedConfig(std::get<ParsedArguments>(parsed)));
    } catch (const std::invalid_argument& error) {
        return ParseError{error.what()};
    }
}

int main() {
    using namespace asciixel;
    const auto default_output = parseConfig({"dir/example.photo.jpg", "-o", "--columns", "80"});
    require(std::holds_alternative<Config>(default_output));
    const auto& default_config = std::get<Config>(default_output);
    require(std::get<ImageConfig>(default_config.media_config).output == ImageOutput::Png);
    require(std::filesystem::u8path(*std::get<ImageConfig>(default_config.media_config).output_path) == std::filesystem::u8path("dir/example.photo_asciixel.png"));
    require(default_config.sampling.columns == 80);
    require(std::holds_alternative<Config>(parseConfig({"a", "-o", "out.txt"})));
    require(std::holds_alternative<ParseError>(parseArguments({"a", "--format", "png"})));
    for (const auto& args : std::vector<std::vector<std::string>>{
        {"a", "--output", "out.png"}, {"a", "--output", "out.PNG"},
        {"a", "--output"}}) {
        const auto result = parseConfig(args);
        require(std::holds_alternative<Config>(result));
        require(std::get<ImageConfig>(std::get<Config>(result).media_config).output == ImageOutput::Png);
    }
    for (const auto& args : std::vector<std::vector<std::string>>{
        {"a", "--output=out.png"}, {"a", "-o", "out.png"},
        {"a", "-o=out.png"}, {"a", "--font=custom.ttf", "--columns=80"}}) {
        require(std::holds_alternative<Config>(parseConfig(args)));
    }
    require(std::get<ParsedArguments>(parseArguments({"-h"})).help);
    for (const auto& args : std::vector<std::vector<std::string>>{
        {"a", "-o", "-h"}, {"a", "--help=yes"},
        {"a", "-o=x.png", "--output", "y.png"},
        {"a", "--output=x.png", "-o", "y.png"}, {"a", "-f", "font.ttf"},
        {"a", "-ox.png"}, {"-ho"}}) {
        require(std::holds_alternative<ParseError>(parseArguments(args)));
    }
    const auto bare = std::get<ParsedArguments>(parseArguments({"a", "-o"}));
    require(bare.options.count("output") && !bare.options.at("output"));
    require(std::get<ImageConfig>(resolveConfig(buildConfig({
        {"input", "photo.jpg"}, {"output", std::nullopt}})).media_config).output_path == "photo_asciixel.png");
    const auto txt = std::get<Config>(parseConfig({"a", "-o", "out.TXT"}));
    require(std::get<ImageConfig>(txt.media_config).output == ImageOutput::Txt);
    const auto gui = resolveConfig(buildConfig({
        {"input", "photo.png"}, {"output", "gui.png"},
        {"font", "custom font.ttf"}, {"font-size", "32"}, {"columns", "80"}}));
    require(gui.input_path == "photo.png" && gui.charset.font_path == "custom font.ttf");
    require(gui.charset.pixel_size == 32 && gui.sampling.columns == 80);
    require(std::get<ImageConfig>(gui.media_config).output == ImageOutput::Png);
    for (const auto& values : std::vector<ConfigValues>{
        {}, {{"input", std::nullopt}}, {{"input", "a"}, {"font", std::nullopt}},
        {{"input", "a"}, {"bogus", "x"}},
        {{"input", "a"}, {"--font", "x"}},
        {{"input", "a"}, {"font-size", "12x"}}}) {
        bool rejected = false;
        try { buildConfig(values); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected);
    }
    const auto equals = std::get<ParsedArguments>(parseArguments({
        "a", "--font=dir/a=b.ttf", "--output=", "--columns=-1"}));
    require(equals.options.at("font") == "dir/a=b.ttf");
    require(equals.options.at("output")->empty());
    require(equals.options.at("columns") == "-1");
    const auto dashed = std::get<ParsedArguments>(parseArguments({"a", "-o=-out.png"}));
    require(dashed.options.at("output") == "-out.png");
    const auto defaults = makeDefaultConfig();
    const auto parsed = parseConfig({"photo.png"});
    require(std::holds_alternative<Config>(parsed));
    const auto& config = std::get<Config>(parsed);
    require(config.input_path == "photo.png");
    require(config.charset.font_path == defaults.charset.font_path);
    require(config.charset.candidates == defaults.charset.candidates);
    require(config.charset.pixel_size == defaults.charset.pixel_size);
    require(config.sampling.columns == defaults.sampling.columns);
    require(std::get<ImageConfig>(config.media_config).output == ImageOutput::Terminal);
    require(!std::get<ImageConfig>(config.media_config).output_path);
    const auto custom = parseConfig({"--font", "custom font.ttf", "photo.png",
        "--font-size", "32", "--columns", "80", "--output", "out.png"});
    require(std::holds_alternative<Config>(custom));
    const auto& c = std::get<Config>(custom);
    require(c.charset.font_path == "custom font.ttf" && c.charset.pixel_size == 32);
    require(c.sampling.columns == 80 && std::get<ImageConfig>(c.media_config).output == ImageOutput::Png);
    require(std::get<ImageConfig>(c.media_config).output_path == "out.png");
    require(std::get<ParsedArguments>(parseArguments({"--help"})).help);
    // Syntax parsing preserves raw values; business conversion happens later.
    const auto raw = parseArguments({"a", "--columns", "not-a-number", "--output", ""});
    require(std::holds_alternative<ParsedArguments>(raw));
    const auto& arguments = std::get<ParsedArguments>(raw);
    require(arguments.positional == std::vector<std::string>{"a"});
    require(arguments.options.at("columns") == "not-a-number");
    require(arguments.options.at("output")->empty());
    const auto unresolved = buildParsedConfig(std::get<ParsedArguments>(
        parseArguments({"a", "--output", "out.png"})));
    require(std::get<ImageConfig>(unresolved.media_config).output_path == "out.png");
    const auto out_of_range = buildParsedConfig(std::get<ParsedArguments>(
        parseArguments({"a", "--columns", "0"})));
    require(out_of_range.sampling.columns == 0);
    require(std::holds_alternative<ParsedArguments>(parseArguments({"a", "b"})));
    require(std::get<Config>(parseConfig({"--", "--photo.png"})).input_path == "--photo.png");
    for (const auto& args : std::vector<std::vector<std::string>>{
        {}, {""}, {"a", "b"}, {"--help", "a"}, {"a", "--bad"},
        {"a", "--font"}, {"a", "--font", ""}, {"a", "--font", "--columns", "12"},
        {"a", "--columns", "12x"}, {"a", "--columns", "-1"},
        {"a", "--columns", "0"}, {"a", "--columns", "4097"},
        {"a", "--columns", "99999999999999999999999999999"},
        {"a", "--font-size", "0"}, {"a", "--font-size", "257"},
        {"a", "--format", "txt"}, {"a", "--format", "png"},
        {"a", "-o="}, {"a", "-o", ""}, {"a", "-o", "-"},
        {"a", "--output", "out.jpg"}, {"a", "--output", "out"},
        {"a", "--output", "out.jpg", "--format", "png"},
        {"a", "--output", "out.xyz"},
        {"a", "--format", "terminal", "--output", "out.png"},
        {"a", "--output", "out.png", "--format", "terminal"},
        {"a", "--format", "png", "--output", "-"},
        {"a", "--format", "png", "--output", ""},
        {"a", "--columns", "12", "--columns", "13"}}) {
        const auto result = parseConfig(args);
        require(std::holds_alternative<ParseError>(result));
        require(!std::get<ParseError>(result).message.empty());
    }
    auto direct = defaults;
    direct.input_path = "photo.png";
    std::get<ImageConfig>(direct.media_config).output_path = "preview.PNG";
    const auto resolved = resolveConfig(direct);
    require(std::get<ImageConfig>(resolved.media_config).output == ImageOutput::Png);
    require(std::get<ImageConfig>(resolveConfig(resolved).media_config).output == ImageOutput::Png);
    require(std::get<ImageConfig>(direct.media_config).output == ImageOutput::Terminal);
    auto invalid = defaults;
    invalid.input_path = "a";
    invalid.sampling.columns = 0;
    bool rejected = false;
    try { validateConfig(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
