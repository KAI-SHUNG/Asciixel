#include "arg_parser/arg_parser.hpp"
#include "asciixel/app/image_pipeline.hpp"
#include "asciixel/app/video_pipeline.hpp"
#include "asciixel/config/asciixel_config.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

arg_parser::ArgParser registerArgs();

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    // Register and parse command-line arguments.
    arg_parser::ArgParser parser;
    try {
        parser = registerArgs();
    }
    catch (const std::exception& error) {
        std::cerr << "Failed to register arguments: " << error.what() << '\n';
        return 1;
    }
    // Handle argument parsing errors and display help if requested.
    try {
        parser.parse(argc, argv);
    }
    catch (const std::exception& error) {
        std::cerr << "Failed to parse arguments: " << error.what() << '\n';
        parser.help();
        return 22;
    }
    if (parser.has("help")) {
        parser.help();
        return 0;
    }

    // Build configuration from parsed arguments.
    asciixel::Config config;
    config.input_path        = parser.get<std::string>("input");
    config.charset.font_path = parser.get<std::string>("font");
    config.charset.font_size = parser.get<unsigned int>("font-size");
    config.sampling.columns  = parser.get<std::size_t>("columns");
    // Resolve input file type.
    bool is_image = asciixel::isImageInput(config.input_path);
    bool is_video = asciixel::isVideoInput(config.input_path);
    if (is_image) {
        config.media_config = asciixel::ImageConfig{};
    }
    else if (is_video) {
        config.media_config = asciixel::VideoConfig{};
    }
    else {
        std::cerr << "Unsupported input file type: " << config.input_path << '\n';
        return 22;
    }

    // Resolve output file type if provided.
    if (parser.has("output")) {
        std::string output_path = parser.get<std::string>("output");
        try {
            if (is_image) {
                auto& image_config       = std::get<asciixel::ImageConfig>(config.media_config);
                image_config.output      = asciixel::imageOutputFromPath(output_path);
                image_config.output_path = output_path;
            }
            else if (is_video) {
                auto& video_config       = std::get<asciixel::VideoConfig>(config.media_config);
                video_config.output      = asciixel::videoOutputFromPath(output_path);
                video_config.output_path = output_path;
                video_config.fps         = 30; // placeholder for future FPS override support
            }
        }
        catch (const std::exception& error) {
            std::cerr << "Failed to resolve output type: " << error.what() << '\n';
            return 22;
        }
    }
    else {
        // If no output path is provided, use default output types.
        if (is_image) {
            auto& image_config       = std::get<asciixel::ImageConfig>(config.media_config);
            image_config.output      = asciixel::ImageOutput::Terminal;
            image_config.output_path = std::nullopt;
        }
        else if (is_video) {
            auto& video_config       = std::get<asciixel::VideoConfig>(config.media_config);
            video_config.output      = asciixel::VideoOutput::Terminal;
            video_config.output_path = std::nullopt;
            video_config.fps         = 30; // placeholder for future FPS override support
        }
    }
    // The above configuration is too long, with a lot of try-catch blocks
    // and should be refactored into a separate function.

    // Switch to right media type's pipeline
    try {
        if (is_image) {
            asciixel::convertImage(config);
        }
        else if (is_video) {
            return asciixel::convertVideo(config) ? 0 : 130;
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error occurred during conversion: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

arg_parser::ArgParser registerArgs()
{
    arg_parser::ArgParser parser;
    parser.add_argument("help", "h", arg_parser::ArgType::Flag)
        .set_default("false")
        .set_required(false)
        .set_description("Display this help message and exit.");
    parser.add_argument("input", std::nullopt, arg_parser::ArgType::Positional)
        .set_required(true)
        .set_description("Path to the input image or video file.");
    parser.add_argument("output", "o", arg_parser::ArgType::Option)
        .set_required(false)
        .set_description("Path to the output file (PNG or TXT).");
    parser.add_argument("font", std::nullopt, arg_parser::ArgType::Option)
        .set_default(std::string(asciixel::cDefaultFontPath))
        .set_required(false)
        .set_description("Path to the font file for ASCII rendering.");
    parser.add_argument("font-size", std::nullopt, arg_parser::ArgType::Option)
        .set_default(std::to_string(asciixel::cDefaultFontSize))
        .set_required(false)
        .set_description("Font size in pixels for ASCII rendering.");
    parser.add_argument("columns", "c", arg_parser::ArgType::Option)
        .set_default(std::to_string(asciixel::cDefaultColumns))
        .set_required(false)
        .set_description("Number of columns in the output.");
    return std::move(parser);
}
