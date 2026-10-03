#include "asciixel/app/video_pipeline.hpp"
#include "asciixel/io/terminal_video_output.hpp"
#include "asciixel/io/terminal_progress.hpp"

#include <iostream>
#include <stdexcept>

namespace asciixel {
namespace {

VideoOutput requestedOutput(const Config& config)
{
    validateConfig(config);
    const auto* video = std::get_if<VideoConfig>(&config.media_config);
    if (!video) {
        throw std::invalid_argument("Video conversion requires a video configuration");
    }
    if (video->output == VideoOutput::Terminal && video->output_path) {
        throw std::invalid_argument("Terminal output does not accept a file path");
    }
    return video->output;
}

VideoPlaybackHooks terminalHooks(TerminalVideoOutput& output)
{
    return {
        [&](std::int64_t time) { return output.waitUntil(time); },
        [&](const AsciiFrame& frame) { output.writeFrame(frame); },
        [&] { return output.interrupted(); },
    };
}

} // namespace

bool convertVideo(const Config& config, const VideoPlaybackHooks& hooks,
                  const VideoProgressCallback& progress)
{
    switch (requestedOutput(config)) {
    case VideoOutput::Terminal: {
        const auto video = prepareVideo(config, hooks.interrupted, progress);
        return video && playVideo(*video, hooks);
    }
    case VideoOutput::Mp4:
        throw std::invalid_argument("MP4 video export is not implemented yet");
    default:
        throw std::invalid_argument("Invalid video output");
    }
}

bool convertVideo(const Config& config)
{
    switch (requestedOutput(config)) {
    case VideoOutput::Terminal: {
        // IO installs the interrupt handler before preparation; timing starts later.
        TerminalVideoOutput output;
        TerminalProgress progress(std::cout, "Converting");
        const auto hooks = terminalHooks(output);
        const auto video = prepareVideo(config, hooks.interrupted,
            [&](const VideoConversionProgress& status) {
                progress.update(status.processed_us,
                    status.total_us ? std::optional<double>{*status.total_us} : std::nullopt,
                    std::to_string(status.converted_frames) + " frames", status.completed);
            });
        progress.clear();
        return video && playVideo(*video, hooks);
    }
    case VideoOutput::Mp4:
        // A future export path will render and encode without a playback clock.
        throw std::invalid_argument("MP4 video export is not implemented yet");
    default:
        throw std::invalid_argument("Invalid video output");
    }
}

} // namespace asciixel
