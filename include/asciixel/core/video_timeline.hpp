#ifndef ASCIIXEL_CORE_VIDEO_TIMELINE_HPP
#define ASCIIXEL_CORE_VIDEO_TIMELINE_HPP

#include <cstdint>
#include <optional>

namespace asciixel {

// Resolve source presentation times to a strictly increasing playback timeline.
// A missing/invalid stream interval falls back to 30 fps; no clock or I/O here.
class VideoTimeline {
public:
    explicit VideoTimeline(std::optional<std::int64_t> frame_interval_us);
    std::int64_t advance(std::optional<std::int64_t> timestamp_us,
                         std::optional<std::int64_t> duration_us);
    std::int64_t endTime() const;

private:
    std::optional<std::int64_t> source_anchor_;
    std::int64_t anchor_time_ = 0;
    std::int64_t time_ = 0;
    std::int64_t interval_;
    std::int64_t duration_;
    bool started_ = false;
};

} // namespace asciixel

#endif
