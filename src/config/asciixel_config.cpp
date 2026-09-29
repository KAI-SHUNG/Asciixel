#include "asciixel/config/asciixel_config.hpp"

#include <stdexcept>
#include <filesystem>
#include <algorithm>

namespace asciixel {
namespace {
ImageOutput outputFromPath(const std::string& path)
{
    if (path.empty() || path == "-")
        throw std::invalid_argument("Output path must be a nonempty file path");
    auto extension = std::filesystem::u8path(path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    });
    if (extension == ".png") return ImageOutput::Png;
    if (extension == ".txt") return ImageOutput::Txt;
    throw std::invalid_argument("Output path requires a .png or .txt extension");
}
} // namespace

Config resolveConfig(Config config)
{
    if (auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        if (image->output != ImageOutput::Terminal && image->output != ImageOutput::Png &&
            image->output != ImageOutput::Txt)
            throw std::invalid_argument("Invalid image output");
        if (!image->output_path && image->output != ImageOutput::Terminal) {
            const auto input = std::filesystem::u8path(config.input_path);
            const auto filename = input.stem().u8string() + "_asciixel" +
                (image->output == ImageOutput::Txt ? ".txt" : ".png");
            image->output_path = (input.parent_path() / std::filesystem::u8path(filename)).u8string();
        }
        if (image->output_path) image->output = outputFromPath(*image->output_path);
    }
    validateConfig(config);
    return config;
}

Config makeDefaultConfig()
{
    Config config;
#ifdef _WIN32
    config.charset.font_path = "C:/Windows/Fonts/consola.ttf";
#else
    config.charset.font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif
    config.charset.candidates = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return config;
}

void validateConfig(const Config& config)
{
    if (config.input_path.empty())
        throw std::invalid_argument("Input path is required");
    config.charset.validate();
    if (config.sampling.columns < 1 || config.sampling.columns > 4096)
        throw std::invalid_argument("Columns must be 1..4096");
    if (const auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        switch (image->output) {
        case ImageOutput::Terminal:
            if (image->output_path)
                throw std::invalid_argument("Terminal output does not accept a file path");
            break;
        case ImageOutput::Png:
        case ImageOutput::Txt:
            if (!image->output_path || outputFromPath(*image->output_path) != image->output)
                throw std::invalid_argument("Output path and format must be resolved first");
            break;
        default:
            throw std::invalid_argument("Invalid image output");
        }
    }
    if (const auto* video = std::get_if<VideoConfig>(&config.media_config)) {
        if (video->fps && *video->fps == 0)
            throw std::invalid_argument("FPS must be positive");
    }
}

void CharsetConfig::validate() const
{
    if (font_path.empty() || font_path.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Invalid font_path");
    }
    if (pixel_size < 1 || pixel_size > 256) {
        throw std::invalid_argument("pixel_size must be 1..256");
    }
    bool seen[127] = {};
    for (unsigned char ch : candidates) {
        if (ch < 32 || ch > 126 || seen[ch]) {
            throw std::invalid_argument("candidates must be unique printable ASCII");
        }
        seen[ch] = true;
    }
    if (!seen[' ']) {
        throw std::invalid_argument("candidates must include a space");
    }
}

} // namespace asciixel
