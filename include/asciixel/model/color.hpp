#ifndef COLOR_HPP
#define COLOR_HPP

#include <cstdint>

namespace asciixel {

// Encoded sRGB channels in [0, 255]. Convert to linear light before arithmetic.
struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

}// namespace asciixel

#endif
