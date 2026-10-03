#ifndef ASCIIXEL_CORE_COLOR_CONVERSION_HPP
#define ASCIIXEL_CORE_COLOR_CONVERSION_HPP

#include "asciixel/model/color.hpp"
#include "asciixel/model/linear_color.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace asciixel {

// Decode the 256 possible channel values once; sampling then uses table lookups.
inline float srgbToLinear(std::uint8_t channel)
{
    static const std::array<float, 256> table = [] {
        std::array<float, 256> values{};
        for (std::size_t i = 0; i < values.size(); ++i) {
            const double value = i / 255.0;
            values[i]          = static_cast<float>(value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4));
        }
        return values;
    }();
    return table[channel];
}

// The caller supplies a finite linear value; clamp roundoff at the endpoints.
inline std::uint8_t linearToSrgb(double channel)
{
    const double value   = std::clamp(channel, 0.0, 1.0);
    const double encoded = value <= 0.0031308 ? value * 12.92 : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
    return static_cast<std::uint8_t>(std::lround(encoded * 255.0));
}

inline Color toSrgb(const LinearColor& color)
{
    return {linearToSrgb(color.r), linearToSrgb(color.g), linearToSrgb(color.b)};
}

} // namespace asciixel

#endif
