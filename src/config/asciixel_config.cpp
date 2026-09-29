#include "asciixel/config/asciixel_config.hpp"

#include <stdexcept>
#include <filesystem>
#include <algorithm>

namespace asciixel {
namespace {

/**
 * @brief Infer the output format from a case-insensitive file extension.
 *
 * @param path Output file path; empty paths and stdout markers are rejected.
 *
 * @return PNG or TXT output kind; unsupported extensions throw.
 */
ImageOutput outputFromPath(const std::string& path)
{
    // Reject destinations that do not identify an output file.
    if (path.empty() || path == "-")
        throw std::invalid_argument("Output path must be a nonempty file path");

    // Normalize only ASCII extension letters, preserving the original path.
    auto extension = std::filesystem::u8path(path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) {
        return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    });

    // Map supported extensions to the output enum.
    if (extension == ".png") return ImageOutput::Png;
    if (extension == ".txt") return ImageOutput::Txt;
    throw std::invalid_argument("Output path requires a .png or .txt extension");
}
} // namespace

/**
 * @brief Complete image output paths and validate the resulting configuration.
 *
 * @param config Configuration copied for output-path and format resolution.
 *
 * @return Resolved configuration satisfying the shared validation rules.
 */
Config resolveConfig(Config config)
{
    // Resolve image destinations without changing video options.
    if (auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        if (image->output != ImageOutput::Terminal &&
            image->output != ImageOutput::Png &&
            image->output != ImageOutput::Txt)
            throw std::invalid_argument("Invalid image output");

        // A valueless file-output request writes beside the input file.
        if (!image->output_path && image->output != ImageOutput::Terminal) {
            const auto input = std::filesystem::u8path(config.input_path);
            const auto filename = input.stem().u8string() + "_asciixel" +
                (image->output == ImageOutput::Txt ? ".txt" : ".png");
            image->output_path = (input.parent_path() / std::filesystem::u8path(filename)).u8string();
        }

        // An explicit extension determines the final file format.
        if (image->output_path) image->output = outputFromPath(*image->output_path);
    }

    // Apply the same final contract for CLI and other callers.
    validateConfig(config);
    return config;
}

/**
 * @brief Create an image configuration with platform font defaults.
 *
 * @return Default configuration with a punctuation charset and no input path.
 */
Config makeDefaultConfig()
{
    // Choose the platform's default monospace font.
    Config config;
#ifdef _WIN32
    config.charset.font_path = "C:/Windows/Fonts/consola.ttf";
#else
    config.charset.font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif

    // Include space as the zero-coverage candidate.
    config.charset.candidates = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return config;
}

/**
 * @brief Check shared settings and media-specific output constraints.
 *
 * @param config Configuration with resolved image output paths and format.
 *
 * @return No value; invalid settings throw std::invalid_argument.
 */
void validateConfig(const Config& config)
{
    // Validate settings required by every media task.
    if (config.input_path.empty())
        throw std::invalid_argument("Input path is required");
    config.charset.validate();
    if (config.sampling.columns < 1 || config.sampling.columns > 4096)
        throw std::invalid_argument("Columns must be 1..4096");

    // Require image destinations to agree with the resolved output kind.
    if (const auto* image = std::get_if<ImageConfig>(&config.media_config)) {
        switch (image->output) {
        case ImageOutput::Terminal:
            if (image->output_path)
                throw std::invalid_argument("Terminal output does not accept a file path");
            break;
        case ImageOutput::Png:
        case ImageOutput::Txt:
            if (!image->output_path ||
                outputFromPath(*image->output_path) != image->output)
                throw std::invalid_argument("Output path and format must be resolved first");
            break;
        default:
            throw std::invalid_argument("Invalid image output");
        }
    }

    // Validate the currently represented video settings.
    if (const auto* video = std::get_if<VideoConfig>(&config.media_config)) {
        if (video->fps && *video->fps == 0)
            throw std::invalid_argument("FPS must be positive");
    }
}

/**
 * @brief Check this charset's font request and printable ASCII candidates.
 *
 * @return No value; invalid fields or missing space throw invalid_argument.
 */
void CharsetConfig::validate() const
{
    // Check font path syntax and the supported rasterization size.
    if (font_path.empty() || font_path.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Invalid font_path");
    }
    if (pixel_size < 1 || pixel_size > 256) {
        throw std::invalid_argument("pixel_size must be 1..256");
    }

    // Reject duplicate and non-printable candidates.
    bool seen[127] = {};
    for (unsigned char ch : candidates) {
        if (ch < 32 || ch > 126 || seen[ch]) {
            throw std::invalid_argument("candidates must be unique printable ASCII");
        }
        seen[ch] = true;
    }

    // Space is required to represent an empty cell.
    if (!seen[' ']) {
        throw std::invalid_argument("candidates must include a space");
    }
}

} // namespace asciixel
