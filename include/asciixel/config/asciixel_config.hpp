#ifndef ASCIIXEL_CONFIG_HPP
#define ASCIIXEL_CONFIG_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <variant>

namespace asciixel {

struct CharsetConfig {
    std::string font_path;
    unsigned    pixel_size = 24;
    std::string candidates;

    void validate() const;
};

struct SampleConfig {
    std::size_t columns = 200;
};

enum class ImageOutput {
    Terminal,
    Png,
    Txt
};

struct ImageConfig {
    ImageOutput output = ImageOutput::Terminal;
    std::optional<std::string> output_path;
};

struct VideoConfig {
    std::optional<std::string> output_path;
    std::optional<unsigned>    fps;
};

using MediaConfig = std::variant<ImageConfig, VideoConfig>;

struct Config {
    CharsetConfig charset;
    SampleConfig  sampling;

    std::string  input_path;
    MediaConfig  media_config;
};

Config makeDefaultConfig();
// Resolve output destination and image encoding, then validate the result.
Config resolveConfig(Config config);
void   validateConfig(const Config& config);

} // namespace asciixel

#endif // ASCIIXEL_CONFIG_HPP
