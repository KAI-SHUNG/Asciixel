#include "asciixel/app/video_player.hpp"

#include <stdexcept>

namespace asciixel {

bool playVideo(const AsciiVideo& video, const VideoPlaybackHooks& hooks)
{
    if (!hooks.wait_until || !hooks.write_frame) {
        throw std::invalid_argument("Video playback services are incomplete");
    }
    if (video.frames.empty()) {
        throw std::runtime_error("Video contains no converted frames");
    }
    for (const auto& timed : video.frames) {
        if (hooks.interrupted && hooks.interrupted()) {
            return false;
        }
        if (!hooks.wait_until(timed.timestamp_us)) {
            return false;
        }
        if (hooks.interrupted && hooks.interrupted()) {
            return false;
        }
        hooks.write_frame(timed.frame);
    }
    const auto& last = video.frames.back();
    return hooks.wait_until(last.timestamp_us + last.duration_us);
}

} // namespace asciixel
