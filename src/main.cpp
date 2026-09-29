#include "asciixel/core/ascii_renderer.hpp"
#include "asciixel/core/charset_builder.hpp"
#include "asciixel/core/glyph_matcher.hpp"
#include "asciixel/core/grid_layout.hpp"
#include "asciixel/core/image_sampler.hpp"
#include "asciixel/io/image_loader.hpp"
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

const char* defaultFontPath()
{
#ifdef _WIN32
    return "C:/Windows/Fonts/consola.ttf";
#else
    return "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif
}

asciixel::CharsetConfig defaultCharsetConfig()
{
    asciixel::CharsetConfig config;
    config.font_path  = defaultFontPath();
    config.pixel_size = 24;
    // for (int ch = 32; ch <= 126; ++ch) {
    //     config.candidates += static_cast<char>(ch);
    // }
    config.candidates = " !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
    return config;
}

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

void convertImage(const std::string& path, const std::string& output)
{
    const asciixel::ImageFrame        image = asciixel::loadImage(path);
    const asciixel::RasterizedCharset charset =
        asciixel::CharsetBuilder::buildCharset(defaultCharsetConfig());
    const asciixel::SampleConfig sampling_config;
    const asciixel::GridSize       grid =
        asciixel::calculateGrid(image.width, image.height, sampling_config, charset.layout);
    const asciixel::SampledFrame sampled =
        asciixel::ImageSampler::sample(image, grid.columns, grid.rows);
    const auto frame = asciixel::GlyphMatcher::match(sampled, charset);
    if (output.empty())
        asciixel::writeAsciiFrame(frame);
    else
        asciixel::writePng(asciixel::renderAscii(frame, charset), output);
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    if (argc < 2) {
        std::cerr << "Usage: asciixel <image-path> [--format png --output <path>]\n";
        return 2;
    }

    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
#ifdef _WIN32
            args.push_back(toUtf8(argv[i]));
#else
            args.emplace_back(argv[i]);
#endif
        }
        std::string format     = "terminal", output;
        bool        has_format = false, has_output = false;
        for (std::size_t i = 1; i < args.size(); ++i) {
            const auto& option = args[i];
            if (i + 1 >= args.size()) throw std::invalid_argument("Missing option value: " + option);
            if (option == "--format" && !has_format) {
                has_format = true;
                format     = args[++i];
            }
            else if (option == "--output" && !has_output) {
                has_output = true;
                output     = args[++i];
            }
            else
                throw std::invalid_argument("Unknown or repeated option: " + option);
        }
        if (format != "terminal" && format != "png")
            throw std::invalid_argument("Format must be terminal or png");
        if ((format == "png" && (!has_output || output.empty() || output == "-")) || (format == "terminal" && has_output))
            throw std::invalid_argument("PNG requires --output <file>; terminal does not accept --output");
        convertImage(args[0], output);
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
