#ifndef ASCIIXEL_MODEL_VIDEO_CONVERSION_PROGRESS_HPP
#define ASCIIXEL_MODEL_VIDEO_CONVERSION_PROGRESS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>

namespace asciixel {

struct VideoConversionProgress {
    std::size_t converted_frames = 0;
    std::int64_t processed_us = 0;
    std::optional<std::int64_t> total_us;
    bool completed = false;
};

} // namespace asciixel

#endif
