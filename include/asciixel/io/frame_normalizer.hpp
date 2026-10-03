#ifndef ASCIIXEL_IO_FRAME_NORMALIZER_HPP
#define ASCIIXEL_IO_FRAME_NORMALIZER_HPP

#include "asciixel/model/image_frame.hpp"

struct AVFrame;

namespace asciixel {

// Return sRGB8 pixels; composite transparency in linear light over an sRGB8 background.
ImageFrame normalizeFrame(const AVFrame& source, Color background);

} // namespace asciixel

#endif
