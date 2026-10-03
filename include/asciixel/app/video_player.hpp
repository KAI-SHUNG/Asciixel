#ifndef ASCIIXEL_APP_VIDEO_PLAYER_HPP
#define ASCIIXEL_APP_VIDEO_PLAYER_HPP

#include "asciixel/model/ascii_video.hpp"

#include <cstdint>
#include <functional>

namespace asciixel {

// Device-independent services. Deadlines are playback-relative microseconds;
// waits return false on cancellation. Device details belong to the output.
struct VideoPlaybackHooks {
    std::function<bool(std::int64_t)> wait_until;
    std::function<void(const AsciiFrame&)> write_frame;
    std::function<bool()> interrupted;
};

// Schedule a cache satisfying AsciiVideo's timing contract (as prepareVideo does).
// No decoding, rendering or terminal I/O.
bool playVideo(const AsciiVideo& video, const VideoPlaybackHooks& hooks);

} // namespace asciixel

#endif
