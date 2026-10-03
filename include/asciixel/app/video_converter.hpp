#ifndef ASCIIXEL_APP_VIDEO_CONVERTER_HPP
#define ASCIIXEL_APP_VIDEO_CONVERTER_HPP

#include "asciixel/config/asciixel_config.hpp"
#include "asciixel/model/ascii_video.hpp"
#include "asciixel/model/video_conversion_progress.hpp"

#include <functional>
#include <optional>

namespace asciixel {

using VideoProgressCallback = std::function<void(const VideoConversionProgress&)>;

// Decode and convert every frame into an output-independent character cache.
// Source images are released per frame; the decoder is closed before returning.
// Nullopt means cancellation. Conversion failures propagate to the caller.
std::optional<AsciiVideo> prepareVideo(
    const Config& config, const std::function<bool()>& interrupted = {},
    const VideoProgressCallback& progress = {});

} // namespace asciixel

#endif
