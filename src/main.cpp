#include "asciixel/app/image_pipeline.hpp"
#include "asciixel/app/video_player.hpp"
#include "asciixel/config/config_builder.hpp"
#include "asciixel/io/arg_parser.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

asciixel::ArgParser registerArgs();

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    asciixel::ArgParser parser;
    try {
        parser = registerArgs();
    }
    catch (const std::exception& error) {
        std::cerr << "Failed to register arguments: " << error.what() << '\n';
        return 1;
    }
    try {
        parser.Parse(argc, argv);
    }
    catch (const std::exception& error) {
        std::cerr << "Failed to parse arguments: " << error.what() << '\n';
        parser.help();
        return 22;
    }
    try {
        std::cout << "Input: " << parser.get<std::string>("input") << std::endl;
        std::cout << "Output: " << parser.get<std::string>("output") << std::endl;
        std::cout << "Font: " << parser.get<std::string>("font") << std::endl;
        std::cout << "Font size: " << parser.get<int>("font-size") << std::endl;
        std::cout << "Columns: " << parser.get<int>("column") << std::endl;
    }
    catch (const std::exception& error) {
        std::cerr << "Error retrieving argument values: " << error.what() << '\n';
        return 1;
    }
    // const auto config = asciixel::resolveConfig();

    // Switch to right media type's pipeline
    // if (std::holds_alternative<asciixel::ImageConfig>(config.media_config)) {
    // asciixel::convertImage(config);
    // }
    // else if (std::holds_alternative<asciixel::VideoConfig>(config.media_config)) {
    //     return asciixel::playVideo(config) ? 0 : 130;
    // }
    return 0;
}

asciixel::ArgParser registerArgs()
{
    asciixel::ArgParser parser;
    parser.add_argument("help", "h", asciixel::ArgType::Flag)
        .set_default("false")
        .set_required(false)
        .set_description("Display this help message and exit.");
    parser.add_argument("input", std::nullopt, asciixel::ArgType::Positional)
        .set_required(true)
        .set_description("Path to the input image or video file.");
    parser.add_argument("output", "o", asciixel::ArgType::Option)
        .set_required(false)
        .set_description("Path to the output file (PNG or TXT).");
    parser.add_argument("font", std::nullopt, asciixel::ArgType::Option)
#ifdef _WIN32
        .set_default("C:/Windows/Fonts/consola.ttf")
#else
        .set_default("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")
#endif
        .set_required(false)
        .set_description("Path to the font file for ASCII rendering.");
    parser.add_argument("font-size", std::nullopt, asciixel::ArgType::Option)
        .set_default("24")
        .set_required(false)
        .set_description("Font size in pixels for ASCII rendering.");
    parser.add_argument("column", "c", asciixel::ArgType::Option)
        .set_default("100")
        .set_required(false)
        .set_description("Number of columns in the output.");
    return std::move(parser);
}
