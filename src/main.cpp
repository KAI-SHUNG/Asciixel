#include "asciixel/core/ascii_renderer.hpp"
#include "asciixel/core/charset_builder.hpp"
#include "asciixel/core/glyph_matcher.hpp"
#include "asciixel/core/grid_layout.hpp"
#include "asciixel/core/image_sampler.hpp"
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
std::string toUtf8(const wchar_t* value)
{
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           value, -1, nullptr, 0, nullptr, nullptr);
    if (length == 0) {
        throw std::runtime_error("Cannot encode image path as UTF-8");
    }
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

void convertImage(const asciixel::Config& config)
{
    asciixel::validateConfig(config);
    const auto* options = std::get_if<asciixel::ImageConfig>(&config.media_config);
    if (!options) throw std::invalid_argument("Video conversion is not supported yet");
    const asciixel::ImageFrame image = asciixel::loadImage(config.input_path);
    const asciixel::RasterizedCharset charset =
        asciixel::CharsetBuilder::buildCharset(config.charset);
    const asciixel::GridSize       grid =
        asciixel::calculateGrid(image.width, image.height, config.sampling, charset.layout);
    const asciixel::SampledFrame sampled =
        asciixel::ImageSampler::sample(image, grid.columns, grid.rows);
    const auto frame = asciixel::GlyphMatcher::match(sampled, charset);
    if (config.output_config == asciixel::OutputConfig::Terminal)
        asciixel::writeAsciiFrame(frame);
    else
        asciixel::writePng(asciixel::renderAscii(frame, charset), *options->output_path);
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
#ifdef _WIN32
            args.push_back(toUtf8(argv[i]));
#else
            args.emplace_back(argv[i]);
#endif
        }
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
        if (arguments.positional.size() != 1)
            throw std::invalid_argument("Exactly one input path is required");
        auto values = arguments.options;
        values.emplace("input", arguments.positional.front());
        const auto config = asciixel::resolveConfig(asciixel::buildConfig(values));
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
