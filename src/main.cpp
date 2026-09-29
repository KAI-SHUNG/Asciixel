#include "asciixel/app/frame_converter.hpp"
#include "asciixel/core/ascii_renderer.hpp"
#include "asciixel/core/charset_builder.hpp"
#include "asciixel/io/image_loader.hpp"
#include "asciixel/io/arg_parser.hpp"
#include "asciixel/config/config_builder.hpp"
#include "asciixel/io/png_writer.hpp"
#include "asciixel/io/text_writer.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
/**
 * @brief Encode one Windows command-line argument as UTF-8.
 *
 * @param value Non-null, null-terminated UTF-16 argument.
 *
 * @return UTF-8 string without its terminating null byte.
 */
std::string toUtf8(const wchar_t* value)
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
                            utf8.data(), length, nullptr, nullptr) == 0) {
        throw std::runtime_error("Cannot encode image path as UTF-8");
    }
    utf8.pop_back();
    return utf8;
}
#endif

/**
 * @brief Run one image conversion and dispatch its selected output.
 *
 * @param config Resolved image configuration, including output destination.
 *
 * @return No value; configuration and conversion failures propagate.
 */
void convertImage(const asciixel::Config& config)
{
    // Reject unsupported media before loading any conversion resources.
    asciixel::validateConfig(config);
    const auto* options = std::get_if<asciixel::ImageConfig>(&config.media_config);
    if (!options) {
        throw std::invalid_argument("Video conversion is not supported yet");
    }

    // Prepare image pixels and font data for the synchronous frame conversion.
    const asciixel::ImageFrame image = asciixel::loadImage(config.input_path);
    const asciixel::RasterizedCharset charset =
        asciixel::CharsetBuilder::buildCharset(config.charset);
    const auto frame = asciixel::convertFrame(image, charset, config.sampling);

    // Render glyph bitmaps only when the selected output requires pixels.
    switch (options->output) {
    case asciixel::ImageOutput::Terminal:
        asciixel::writeAsciiFrame(frame);
        break;
    case asciixel::ImageOutput::Png:
        asciixel::writePng(asciixel::renderAscii(frame, charset),
                           *options->output_path);
        break;
    case asciixel::ImageOutput::Txt:
        asciixel::writeAsciiFile(frame, *options->output_path);
        break;
    }
}

} // namespace

/**
 * @brief Parse CLI options, run the image task and report failures.
 *
 * @param argc Number of command-line arguments, including the executable.
 * @param argv Argument array; UTF-16 on Windows and native char strings elsewhere.
 *
 * @return 0 on success, 2 for invalid arguments, or 1 for other failures.
 */
#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    try {
        // Normalize arguments while excluding the executable name.
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
#ifdef _WIN32
            args.push_back(toUtf8(argv[i]));
#else
            args.emplace_back(argv[i]);
#endif
        }

        // Handle syntax errors and help before building a conversion request.
        const auto result = asciixel::parseArguments(args);
        if (const auto* error = std::get_if<asciixel::ParseError>(&result)) {
            std::cerr << error->message << '\n' << asciixel::argumentHelp();
            return 2;
        }
        const auto& arguments = std::get<asciixel::ParsedArguments>(result);
        if (arguments.help) {
            std::cout << asciixel::argumentHelp();
            return 0;
        }

        // Combine the input path with options and resolve shared config rules.
        if (arguments.positional.size() != 1)
            throw std::invalid_argument("Exactly one input path is required");
        auto values = arguments.options;
        values.emplace("input", arguments.positional.front());
        const auto config = asciixel::resolveConfig(asciixel::buildConfig(values));

        // Execute synchronously; the surrounding handlers select the exit code.
        convertImage(config);
        return 0;
    }
    catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
