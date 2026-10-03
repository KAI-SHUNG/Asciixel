#ifndef ASCIIXEL_IO_VIDEO_LOADER_HPP
#define ASCIIXEL_IO_VIDEO_LOADER_HPP

#include "asciixel/model/video_frame.hpp"

#include <memory>
#include <optional>
#include <string>

namespace asciixel {

// Sequential local-file decoder. Memory is bounded; no playback or frame skipping.
// Normalization follows normalizeFrame's current color/geometry support.
class VideoLoader {
public:
    explicit VideoLoader(const std::string& path,
                         Color background = {0, 0, 0});
    ~VideoLoader();
    VideoLoader(const VideoLoader&) = delete;
    VideoLoader& operator=(const VideoLoader&) = delete;

    // Returns one owned frame, or nullopt after all delayed frames are drained.
    // File, demuxing, decoding and normalization failures throw runtime_error.
    std::optional<VideoFrame> nextFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace asciixel

#endif
