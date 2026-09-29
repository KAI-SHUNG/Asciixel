#include "asciixel/config/config_builder.hpp"

#include <charconv>
#include <stdexcept>

namespace asciixel {
namespace {
template <typename T>
T parseInteger(const std::string& value, const std::string& option)
{
    T          number{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
        throw std::invalid_argument("Invalid integer for " + option + ": " + value);
    return number;
}
} // namespace

Config buildConfig(const ConfigValues& values)
{
    const auto input = values.find("input");
    if (input == values.end() || !input->second)
        throw std::invalid_argument("Input path is required");
    Config config = makeDefaultConfig();
    config.input_path = *input->second;
    auto& image = std::get<ImageConfig>(config.media_config);
    for (const auto& [option, value] : values) {
        if (option == "input") continue;
        if (option == "output") {
            image.output = ImageOutput::Png;
            image.output_path = value;
            continue;
        }
        if (!value) throw std::invalid_argument("Missing value: " + option);
        if (option == "font") {
            config.charset.font_path = *value;
        } else if (option == "font-size") {
            config.charset.pixel_size = parseInteger<unsigned>(*value, option);
        } else if (option == "columns") {
            config.sampling.columns = parseInteger<std::size_t>(*value, option);
        } else {
            throw std::invalid_argument("Unknown option: " + option);
        }
    }
    return config;
}

} // namespace asciixel
