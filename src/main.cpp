#include "asciixel/app/image_pipeline.hpp"
#include "asciixel/app/video_player.hpp"
#include "asciixel/config/config_builder.hpp"
#include "asciixel/io/arg_parser.hpp"

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
                            utf8.data(), length, nullptr, nullptr)
        == 0) {
        throw std::runtime_error("Cannot encode image path as UTF-8");
    }
    utf8.pop_back();
    return utf8;
}
#endif

} // namespace

/**
 * @brief Parse CLI options, dispatch image conversion or video playback.
 *
 * @param argc Number of command-line arguments, including the executable.
 * @param argv Argument array; UTF-16 on Windows and native char strings elsewhere.
 *
 * @return 0 on success, 2 for invalid arguments, 1 for failures, 130 on interrupt.
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
            std::cerr << error->message << '\n'
                      << asciixel::argumentHelp();
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

        // Switch to right media type's pipeline
        if (std::holds_alternative<asciixel::ImageConfig>(config.media_config)) {
            asciixel::convertImage(config);
        }
        else if (std::holds_alternative<asciixel::VideoConfig>(config.media_config)) {
            return asciixel::playVideo(config) ? 0 : 130;
        }
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
