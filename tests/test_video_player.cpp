#include "asciixel/app/video_player.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

asciixel::AsciiVideo cachedVideo()
{
    asciixel::AsciiVideo video;
    for (int i = 0; i < 5; ++i) {
        asciixel::AsciiFrame frame(8, 3);
        for (auto& pixel : frame.pixels) pixel.character = static_cast<char>('A' + i);
        video.frames.push_back({std::move(frame), i * 200000, 200000});
    }
    return video;
}

void playsEveryCachedFrameAtItsDeadline()
{
    const auto video = cachedVideo();
    std::vector<std::int64_t> deadlines;
    std::size_t writes = 0;
    asciixel::VideoPlaybackHooks hooks{
        [&](std::int64_t time) { deadlines.push_back(time); return true; },
        [&](const asciixel::AsciiFrame& frame) {
            require(frame.pixels.front().character == 'A' + writes, "cached frame order changed");
            ++writes;
        }};
    require(asciixel::playVideo(video, hooks), "cached playback interrupted");
    require(writes == 5, "cached frames were dropped");
    require(deadlines == std::vector<std::int64_t>{0, 200000, 400000, 600000, 800000, 1000000},
            "frame deadlines or final hold changed");
}

void stopsBeforeWritingInterruptedFrame()
{
    const auto video = cachedVideo();
    int writes = 0;
    asciixel::VideoPlaybackHooks hooks{
        [](std::int64_t time) { return time == 0; },
        [&](const asciixel::AsciiFrame&) { ++writes; }};
    require(!asciixel::playVideo(video, hooks) && writes == 1,
            "interrupt must stop before displaying the second frame");
    writes = 0;
    hooks.interrupted = [] { return true; };
    require(!asciixel::playVideo(video, hooks) && writes == 0,
            "pending interrupt must stop before displaying any cached frame");
}

void propagatesOutputFailure()
{
    const auto video = cachedVideo();
    int writes = 0;
    asciixel::VideoPlaybackHooks hooks{
        [](std::int64_t) { return true; },
        [&](const asciixel::AsciiFrame&) {
            if (writes == 1) throw std::runtime_error("output unavailable");
            ++writes;
        }};
    bool rejected = false;
    try { asciixel::playVideo(video, hooks); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected && writes == 1, "output failure must stop further playback");
}

void holdsTheLastFrameForItsStoredDuration()
{
    auto video = cachedVideo();
    video.frames.back().duration_us = 500000;
    std::int64_t last_deadline = -1;
    int writes = 0;
    asciixel::VideoPlaybackHooks hooks{
        [&](std::int64_t time) { last_deadline = time; return true; },
        [&](const asciixel::AsciiFrame&) { ++writes; }};
    require(asciixel::playVideo(video, hooks) && writes == 5 && last_deadline == 1300000,
            "final hold must use the cached frame duration");
}

}

int main()
{
    try {
        playsEveryCachedFrameAtItsDeadline();
        stopsBeforeWritingInterruptedFrame();
        propagatesOutputFailure();
        holdsTheLastFrameForItsStoredDuration();
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
