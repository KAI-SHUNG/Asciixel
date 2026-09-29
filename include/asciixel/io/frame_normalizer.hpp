#ifndef ASCIIXEL_IO_FRAME_NORMALIZER_HPP
#define ASCIIXEL_IO_FRAME_NORMALIZER_HPP

#include "asciixel/model/image_frame.hpp"

struct AVFrame;

namespace asciixel {

// Convert decoded pixels to linear RGB and composite over a linear background.
ImageFrame normalizeFrame(const AVFrame& source, Color background);

} // namespace asciixel

#endif
