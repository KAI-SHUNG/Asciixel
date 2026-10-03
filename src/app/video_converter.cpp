#include "asciixel/app/video_converter.hpp"
#include "asciixel/app/frame_converter.hpp"
#include "asciixel/core/video_timeline.hpp"
#include "asciixel/core/charset_builder.hpp"
#include "asciixel/core/grid_layout.hpp"
#include "asciixel/io/video_loader.hpp"

#include <stdexcept>
#include <utility>

namespace asciixel {

std::optional<AsciiVideo> prepareVideo(const Config& config,
                                     const std::function<bool()>& interrupted,
                                     const VideoProgressCallback& progress)
{
    validateConfig(config);
    if (!std::holds_alternative<VideoConfig>(config.media_config)) {
        throw std::invalid_argument("Video conversion requires a video configuration");
    }
    if (interrupted && interrupted()) {
        return std::nullopt;
    }

    VideoLoader loader(config.input_path);
    VideoConversionProgress status;
    status.total_us = loader.durationUs();
    if (progress) progress(status);
    const auto charset = CharsetBuilder::buildCharset(config.charset);
    VideoTimeline timeline(loader.nominalFrameDurationUs());
    AsciiVideo result;
    std::size_t source_width = 0;
    std::size_t source_height = 0;

    for (;;) {
        if (interrupted && interrupted()) {
            return std::nullopt;
        }
        auto source = loader.nextFrame();
        if (!source) {
            break;
        }
        if (result.frames.empty()) {
            source_width = source->image.width;
            source_height = source->image.height;
        }
        else if (source->image.width != source_width || source->image.height != source_height) {
            throw std::runtime_error("Video dimensions changed during conversion");
        }
        const auto grid = calculateGrid(source_width, source_height, config.sampling, charset.layout);
        if (grid.columns > 1048576 / grid.rows) {
            throw std::runtime_error("Video grid exceeds 1048576 cells");
        }

        auto frame = convertFrame(source->image, charset, config.sampling);
        const auto time = timeline.advance(source->timestamp_us, source->duration_us);
        result.frames.push_back({std::move(frame), time, timeline.endTime() - time});
        status.converted_frames = result.frames.size();
        status.processed_us = timeline.endTime();
        if (progress) progress(status);
        // The source RGB image is released here; only the converted frame remains.
    }
    if (result.frames.empty()) {
        throw std::runtime_error("Video contains no decodable frames");
    }
    if (interrupted && interrupted()) return std::nullopt;
    status.completed = true;
    if (progress) progress(status);
    return result;
}

} // namespace asciixel
