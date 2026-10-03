#ifndef PIXEL_HPP
#define PIXEL_HPP

#include "asciixel/model/color.hpp"
#include "asciixel/model/linear_color.hpp"

namespace asciixel {

struct ImagePixel {
    Color color;
};

struct SampledPixel {
    LinearColor color;
};

struct AsciiPixel {
    char character;
    Color color;
};

} // namespace asciixel

#endif
