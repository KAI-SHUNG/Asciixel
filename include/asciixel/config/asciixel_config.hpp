#ifndef ASCIIXEL_CONFIG_HPP
#define ASCIIXEL_CONFIG_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace asciixel {

// Defaults for CharsetConfig.
#ifdef _WIN32
constexpr const std::string_view cDefaultFontPath = "C:\\Windows\\Fonts\\consola.ttf";
#else
constexpr const std::string_view cDefaultFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif

inline constexpr unsigned int     cDefaultFontSize = 24;
inline constexpr std::string_view cDefaultCharset  = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

// Default for SampleConfig.
inline constexpr std::size_t cDefaultColumns = 100;

// Default for MediaConfig.
inline constexpr std::string_view cDefaultImageOutputPath = "a.png";
inline constexpr std::string_view cDefaultVideoOutputPath = "a.mp4";

struct CharsetConfig {
    std::string  font_path = std::string(cDefaultFontPath);
    unsigned int font_size = cDefaultFontSize; // Font size in pixels.
    std::string  charset   = std::string(cDefaultCharset);

    void validate() const;
};

struct SampleConfig {
    std::size_t columns = cDefaultColumns;
};

enum class ImageOutput {
    Terminal,
    Png,
    Txt
};

enum class VideoOutput {
    Terminal,
    Mp4
};

struct ImageConfig {
    ImageOutput                output      = ImageOutput::Terminal;
    std::optional<std::string> output_path = std::nullopt;
};

struct VideoConfig {
    VideoOutput                output      = VideoOutput::Terminal;
    std::optional<std::string> output_path = std::nullopt;
    std::optional<unsigned>    fps;
};

using MediaConfig = std::variant<ImageConfig, VideoConfig>;

struct Config {
    std::string input_path;

    CharsetConfig charset;
    SampleConfig  sampling;
    MediaConfig   media_config;
};

// Resolve output destination and image encoding, then validate the result.
bool        isImageInput(const std::string& input_path);
bool        isVideoInput(const std::string& input_path);
ImageOutput imageOutputFromPath(const std::string& output_path);
VideoOutput videoOutputFromPath(const std::string& output_path);

void validateConfig(const Config& config);

} // namespace asciixel

#endif // ASCIIXEL_CONFIG_HPP
