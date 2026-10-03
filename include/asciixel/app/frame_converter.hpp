#ifndef ASCIIXEL_APP_FRAME_CONVERTER_HPP
#define ASCIIXEL_APP_FRAME_CONVERTER_HPP

#include "asciixel/config/asciixel_config.hpp"
#include "asciixel/model/ascii_frame.hpp"
#include "asciixel/model/image_frame.hpp"
#include "asciixel/model/rasterized_charset.hpp"

namespace asciixel {

// Synchronously convert one sRGB8 image using a caller-prepared charset.
// Computes the grid, samples pixels and matches glyphs; performs no I/O.
AsciiFrame convertFrame(const ImageFrame&        image,
                        const RasterizedCharset& charset,
                        const SampleConfig&      config);

} // namespace asciixel

#endif
