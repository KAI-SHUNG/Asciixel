#include "asciixel/config/asciixel_config.hpp"

#include <stdexcept>
#include <filesystem>
#include <algorithm>

namespace asciixel {
namespace {
ImageFormat resolveImageFormat(const ImageConfig& image)
{
    if (!image.output_path || image.output_path->empty() || *image.output_path == "-")
        throw std::invalid_argument("File output requires --output <file>");
    auto extension = std::filesystem::u8path(*image.output_path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    });
    if (image.format && *image.format != ImageFormat::Png)
        throw std::invalid_argument("Unsupported image format");
    if (!extension.empty() && extension != ".png") {
        if (image.format)
            throw std::invalid_argument("Output extension conflicts with PNG format: " + extension);
        throw std::invalid_argument("Unsupported output extension: " + extension + "; only PNG export is supported");
    }
    if (extension.empty() && !image.format)
        throw std::invalid_argument("Cannot infer output format; use a .png extension or --format png");
    return ImageFormat::Png;
}
} // namespace

Config resolveConfig(Config config)
{
    if (auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        config.output_config = image->output_path ? OutputConfig::File : OutputConfig::Terminal;
        if (image->output_path) image->format = resolveImageFormat(*image);
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
    if (config.output_config != OutputConfig::Terminal && config.output_config != OutputConfig::File)
        throw std::invalid_argument("Invalid output mode");
    std::visit([&](const auto& media) {
        if (config.output_config == OutputConfig::File && (!media.output_path || media.output_path->empty() || *media.output_path == "-"))
            throw std::invalid_argument("File output requires --output <file>");
        if (config.output_config == OutputConfig::Terminal && media.output_path)
            throw std::invalid_argument("Terminal output does not accept --output");
    },
               config.media_config);
    if (const auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        if (config.output_config == OutputConfig::Terminal && image->format)
            throw std::invalid_argument("Image format requires --output <file>");
        if (config.output_config == OutputConfig::File) {
            const auto format = resolveImageFormat(*image);
            if (!image->format || *image->format != format)
                throw std::invalid_argument("Output format is unresolved; call resolveConfig first");
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
