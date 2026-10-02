#include "asciixel/config/asciixel_config.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace asciixel {


/**
 * @brief Check this charset's font request and printable ASCII charset.
 *
 * @return No value; invalid fields or missing space throw invalid_argument.
 */
void CharsetConfig::validate() const
{
    // Check font path syntax and the supported rasterization size.
    if (font_path.empty() || font_path.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Invalid font_path");
    }
    if (font_size < 1 || font_size > 256) {
        throw std::invalid_argument("font_size must be 1..256");
    }

    // Reject duplicate and non-printable charset.
    bool seen[127] = {};
    for (unsigned char ch : charset) {
        if (ch < 32 || ch > 126 || seen[ch]) {
            throw std::invalid_argument("charset must be unique printable ASCII");
        }
        seen[ch] = true;
    }

    // Space is required to represent an empty cell.
    if (!seen[' ']) {
        throw std::invalid_argument("charset must include a space");
    }
}

bool isImageInput(const std::string& input_path)
{
    auto extension = std::filesystem::u8path(input_path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
                   });
    return extension == ".png" || extension == ".jpg"
           || extension == ".jpeg" || extension == ".bmp";
}

bool isVideoInput(const std::string& input_path)
{
    auto extension = std::filesystem::u8path(input_path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
                   });
    return extension == ".mp4"
        // More video formats are not supported yet, but could be added in the future:
        //  || extension == ".avi" || extension == ".mov" || extension == ".mkv"
        ;
}

/**
 * @brief Infer the output format from a case-insensitive file extension.
 *
 * @param path Output file path; empty paths and stdout markers are rejected.
 *
 * @return PNG or TXT output kind; unsupported extensions throw.
 */
ImageOutput imageOutputFromPath(const std::string& path)
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

VideoOutput videoOutputFromPath(const std::string& path)
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
    if (extension == ".mp4") return VideoOutput::Mp4;
    throw std::invalid_argument("Output path requires a .mp4 extension");
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
            if (!image->output_path || imageOutputFromPath(*image->output_path) != image->output)
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
        // if (video->output_path) {
        //     throw std::invalid_argument("Video file output is not supported yet");
        // }
        // if (video->fps) {
        //     throw std::invalid_argument("Video FPS override is not supported yet");
        // }
    }
}

} // namespace asciixel
