#ifndef ASCIIXEL_APP_VIDEO_PIPELINE_HPP
#define ASCIIXEL_APP_VIDEO_PIPELINE_HPP

#include "asciixel/app/video_converter.hpp"
#include "asciixel/app/video_player.hpp"

namespace asciixel {

// Dispatch the requested output. Terminal playback prepares the entire cache
// first; MP4 export is not implemented yet and fails before opening resources.
// Returns false on a handled interrupt during preparation or playback.
bool convertVideo(const Config& config);
bool convertVideo(const Config& config, const VideoPlaybackHooks& hooks,
                  const VideoProgressCallback& progress = {});

} // namespace asciixel

#endif
