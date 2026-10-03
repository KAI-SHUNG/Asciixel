#include "asciixel/app/video_pipeline.hpp"

#include <filesystem>
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

void releasesSourceBeforeAnyPlayback(const std::string& path, const std::string& temporary)
{
    const auto copy = std::filesystem::u8path(temporary);
    std::filesystem::copy_file(std::filesystem::u8path(path), copy,
                              std::filesystem::copy_options::overwrite_existing);
    std::vector<std::int64_t> deadlines;
    std::size_t writes = 0;
    bool conversion_completed = false;
    asciixel::VideoPlaybackHooks hooks{
        [&](std::int64_t time) { deadlines.push_back(time); return true; },
        [&](const asciixel::AsciiFrame& frame) {
            require(conversion_completed, "completion must be reported before playback");
            if (writes == 0) {
                // On Windows the decoder's open file would prevent deletion.
                // Subsequent frames must play entirely from the converted cache.
                require(std::filesystem::remove(copy), "source must be closed before playback");
            }
            require(frame.width == 8 && frame.height > 0, "cached playback frame changed");
            ++writes;
        }};
    require(asciixel::convertVideo(videoConfig(temporary), hooks,
        [&](const asciixel::VideoConversionProgress& status) {
            require(writes == 0, "conversion progress must stop before playback");
            conversion_completed = status.completed;
        }), "pipeline was interrupted");
    require(writes == 5, "playback must display every cached frame after deleting source");
    require(deadlines == std::vector<std::int64_t>{0, 200000, 400000, 600000, 800000, 1000000},
            "preprocessing must not consume playback time");
}

void cancelsPreprocessingWithoutOutput(const std::string& path)
{
    int checks = 0;
    int writes = 0;
    int waits = 0;
    asciixel::VideoPlaybackHooks hooks{
        [&](std::int64_t) { ++waits; return true; },
        [&](const asciixel::AsciiFrame&) { ++writes; }};
    hooks.interrupted = [&] { return ++checks >= 3; };
    require(!asciixel::convertVideo(videoConfig(path), hooks), "preprocessing must stop on interrupt");
    require(writes == 0 && waits == 0, "cancelled preprocessing must never start playback");
}

void rejectsFailedPreparationWithoutOutput(const std::string& path)
{
    int writes = 0;
    asciixel::VideoPlaybackHooks hooks{
        [](std::int64_t) { return true; },
        [&](const asciixel::AsciiFrame&) { ++writes; }};
    auto config = videoConfig(path);
    config.charset.font_path = path + ".missing-font";
    bool rejected = false;
    try { asciixel::convertVideo(config, hooks); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected && writes == 0, "failed preparation must not display a partial video");
}
void rejectsMp4BeforeConversion(const std::string& path)
{
    auto config = videoConfig(path + ".missing");
    auto& media = std::get<asciixel::VideoConfig>(config.media_config);
    media.output = asciixel::VideoOutput::Mp4;
    media.output_path = "future-export.mp4";
    int output_calls = 0;
    asciixel::VideoPlaybackHooks hooks{
        [&](std::int64_t) { ++output_calls; return true; },
        [&](const asciixel::AsciiFrame&) { ++output_calls; }};
    bool rejected = false;
    try { asciixel::convertVideo(config, hooks); }
    catch (const std::invalid_argument& error) {
        rejected = std::string(error.what()).find("MP4") != std::string::npos;
    }
    require(rejected && output_calls == 0, "MP4 must dispatch without entering realtime playback");
    rejected = false;
    try { asciixel::convertVideo(config); }
    catch (const std::invalid_argument& error) {
        rejected = std::string(error.what()).find("MP4") != std::string::npos;
    }
    require(rejected, "MP4 must fail before terminal setup or decoding");
}

}

int main(int argc, char** argv)
{
    try {
        require(argc == 3, "expected a fixture and a temporary video path");
        releasesSourceBeforeAnyPlayback(argv[1], argv[2]);
        cancelsPreprocessingWithoutOutput(argv[1]);
        rejectsFailedPreparationWithoutOutput(argv[1]);
        rejectsMp4BeforeConversion(argv[1]);
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
