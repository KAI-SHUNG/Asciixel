#ifndef ASCIIXEL_MODEL_VIDEO_FRAME_HPP
#define ASCIIXEL_MODEL_VIDEO_FRAME_HPP

#include "asciixel/model/image_frame.hpp"

#include <cstdint>
#include <optional>

namespace asciixel {

// Owns normalized sRGB8 pixels, independent of the decoder's buffers.
// Times are in microseconds in the source stream timeline, not rebased to zero.
// Missing timestamps and nonpositive/unknown durations remain absent.
struct VideoFrame {
    ImageFrame image;
    std::optional<std::int64_t> timestamp_us;
    std::optional<std::int64_t> duration_us;
};

} // namespace asciixel

#endif
