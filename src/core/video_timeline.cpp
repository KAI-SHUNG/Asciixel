#include "asciixel/core/video_timeline.hpp"

#include <limits>
#include <stdexcept>

namespace asciixel {
namespace {

// Reject broken media times instead of overflowing signed microsecond values.
std::int64_t addTime(std::int64_t left, std::int64_t right)
{
    if ((right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) ||
        (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right)) {
        throw std::runtime_error("Video timeline overflows");
    }
    return left + right;
}

std::int64_t subtractTime(std::int64_t left, std::int64_t right)
{
    if ((right > 0 && left < std::numeric_limits<std::int64_t>::min() + right) ||
        (right < 0 && left > std::numeric_limits<std::int64_t>::max() + right)) {
        throw std::runtime_error("Video timestamp difference overflows");
    }
    return left - right;
}

} // namespace

VideoTimeline::VideoTimeline(std::optional<std::int64_t> frame_interval_us)
    : interval_(frame_interval_us && *frame_interval_us > 0
                    ? *frame_interval_us : 33333),
      duration_(interval_)
{
}

/**
 * @brief Resolve a source timestamp without rewinding or changing playback speed.
 *
 * @param timestamp_us Optional original presentation time in microseconds.
 * @param duration_us Optional positive duration of this frame.
 * @return Playback-relative time; the first frame always starts at zero.
 */
std::int64_t VideoTimeline::advance(std::optional<std::int64_t> timestamp_us,
                                  std::optional<std::int64_t> duration_us)
{
    const auto next = started_ ? addTime(time_, duration_) : 0;
    if (timestamp_us && source_anchor_) {
        const auto candidate = addTime(
            anchor_time_, subtractTime(*timestamp_us, *source_anchor_));
        if (!started_ || candidate > time_) {
            time_ = candidate;
        }
        else {
            // Re-anchor backward/repeated timestamps at the synthesized next time.
            time_ = next;
            source_anchor_ = timestamp_us;
            anchor_time_ = time_;
        }
    }
    else {
        time_ = next;
        if (timestamp_us) {
            // A first valid timestamp after missing ones joins the existing timeline.
            source_anchor_ = timestamp_us;
            anchor_time_ = time_;
        }
    }
    duration_ = duration_us && *duration_us > 0 ? *duration_us : interval_;
    started_ = true;
    return time_;
}

// Hold the final frame for its known duration, or the same fallback as other frames.
std::int64_t VideoTimeline::endTime() const
{
    return addTime(time_, duration_);
}

} // namespace asciixel
