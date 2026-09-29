#ifndef ASCIIXEL_CONFIG_HPP
#define ASCIIXEL_CONFIG_HPP

#include <cstddef>
#include <optional>
#include <variant>
#include <string>

namespace asciixel {

struct CharsetConfig {
    std::string font_path;
    unsigned pixel_size = 24;
    std::string candidates;

    void validate() const;
};

struct SampleConfig {
    std::size_t columns = 200;
};

enum class OutputConfig {
    Terminal,
    File
};

struct ImageConfig {
    std::optional<std::string> output_path;
};

struct VideoConfig {
    std::optional<std::string> output_path;
    std::optional<unsigned>    fps;
};

using MediaConfig = std::variant<ImageConfig, VideoConfig>;

struct Config {
    CharsetConfig charset;
    SampleConfig sampling;

    std::string input_path;
    OutputConfig output_config = OutputConfig::Terminal;
    MediaConfig media_config;
};

}

#endif // ASCIIXEL_CONFIG_HPP
