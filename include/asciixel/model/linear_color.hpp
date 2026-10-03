#ifndef ASCIIXEL_MODEL_LINEAR_COLOR_HPP
#define ASCIIXEL_MODEL_LINEAR_COLOR_HPP

namespace asciixel {

// Temporary linear-light channels in [0, 1] for sampling and glyph matching.
// Full-resolution images and final character frames use the compact Color type.
struct LinearColor {
    float r;
    float g;
    float b;
};

} // namespace asciixel

#endif
