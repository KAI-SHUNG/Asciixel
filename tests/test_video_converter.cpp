#include "asciixel/app/video_converter.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

asciixel::Config videoConfig(const std::string& path)
{
    asciixel::Config config;
    config.input_path = path;
    config.media_config = asciixel::VideoConfig{};
    config.sampling.columns = 8;
    return config;
}

void preparesEveryConvertedFrame(const std::string& path)
{
    const auto video = asciixel::prepareVideo(videoConfig(path));
    require(video && video->frames.size() == 5, "all decoded frames must be cached");
    require(video->frames.back().timestamp_us + video->frames.back().duration_us == 1000000, "last-frame hold must be preserved");
    for (std::size_t i = 0; i < video->frames.size(); ++i) {
        const auto& timed = video->frames[i];
        require(timed.timestamp_us == static_cast<std::int64_t>(i) * 200000,
                "cached frame deadline changed");
        require(timed.duration_us == 200000, "resolved frame duration changed");
        require(timed.frame.width == 8 && timed.frame.height > 0 &&
                timed.frame.pixels.size() == timed.frame.width * timed.frame.height,
                "cache must contain converted character frames");
        for (const auto& pixel : timed.frame.pixels) {
            require(pixel.character >= 32 && pixel.character <= 126,
                    "cache must contain matched characters");
        }
    }
}

void rebasesOffsetBeforePlayback(const std::string& path)
{
    std::vector<asciixel::VideoConversionProgress> events;
    const auto video = asciixel::prepareVideo(videoConfig(path), {},
        [&](const asciixel::VideoConversionProgress& progress) { events.push_back(progress); });
    require(video && video->frames.size() == 2, "offset video frames missing");
    require(video->frames[0].timestamp_us == 0 && video->frames[1].timestamp_us == 200000 &&
            video->frames[1].duration_us == 200000, "source offset must not delay playback");
    require(events[1].processed_us == 200000 && events[1].total_us == 400000 &&
            events[2].processed_us == 400000,
            "progress must use the video stream duration and exclude its timestamp offset");
}

void conversionDoesNotDependOnOutput(const std::string& path)
{
    auto config = videoConfig(path);
    auto& output = std::get<asciixel::VideoConfig>(config.media_config);
    output.output = asciixel::VideoOutput::Mp4;
    output.output_path = "future-export.mp4";
    const auto video = asciixel::prepareVideo(config);
    require(video && video->frames.size() == 5,
            "character conversion must be reusable for MP4 and terminal output");
}


void cancelsWithoutReturningPartialCache(const std::string& path)
{
    int checks = 0;
    const auto result = asciixel::prepareVideo(videoConfig(path), [&] { return ++checks >= 3; });
    require(!result, "cancelled conversion must not return a partial cache");
}

void reportsConvertedFramesAndCompletion(const std::string& path)
{
    std::vector<asciixel::VideoConversionProgress> events;
    const auto video = asciixel::prepareVideo(videoConfig(path), {},
        [&](const asciixel::VideoConversionProgress& progress) { events.push_back(progress); });
    require(video && events.size() == 7, "expected initial progress, five converted frames and completion");
    require(events.front().converted_frames == 0 && !events.front().completed,
            "conversion must start with zero completed frames");
    for (std::size_t i = 1; i <= 5; ++i) {
        require(events[i].converted_frames == i && events[i].processed_us == i * 200000 &&
                events[i].total_us == 1000000 && !events[i].completed,
                "progress must follow converted frames on the rebased timeline");
    }
    require(events.back().completed && events.back().converted_frames == 5,
            "completion must follow decoder EOF");
    bool cancelled = false;
    events.clear();
    const auto partial = asciixel::prepareVideo(videoConfig(path), [&] { return cancelled; },
        [&](const asciixel::VideoConversionProgress& progress) {
            events.push_back(progress);
            if (progress.converted_frames == 1) cancelled = true;
        });
    require(!partial && events.size() == 2 && !events.back().completed,
            "cancelled conversion must not report completion");
}
}

int main(int argc, char** argv)
{
    try {
        require(argc == 3, "expected two video fixtures");
        preparesEveryConvertedFrame(argv[1]);
        rebasesOffsetBeforePlayback(argv[2]);
        conversionDoesNotDependOnOutput(argv[1]);
        cancelsWithoutReturningPartialCache(argv[1]);
        reportsConvertedFramesAndCompletion(argv[1]);
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
