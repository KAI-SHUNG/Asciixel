#ifndef ASCIIXEL_MODEL_ASCII_VIDEO_HPP
#define ASCIIXEL_MODEL_ASCII_VIDEO_HPP

#include "asciixel/model/ascii_frame.hpp"

#include <cstdint>
#include <vector>

namespace asciixel {

struct TimedAsciiFrame {
    AsciiFrame frame;
    std::int64_t timestamp_us;
    std::int64_t duration_us;
};

// Converted frames, without source pixels. prepareVideo produces these invariants:
// timestamps start at zero and strictly increase; durations are positive;
// every timestamp_us + duration_us is representable as int64_t.
struct AsciiVideo {
    std::vector<TimedAsciiFrame> frames;
};

} // namespace asciixel

#endif
