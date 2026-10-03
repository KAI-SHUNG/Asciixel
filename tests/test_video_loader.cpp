#include "asciixel/io/video_loader.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Losing flush output, using DTS, or returning borrowed storage breaks this test.
void loadsAllFrames(const std::string& path)
{
    asciixel::VideoLoader loader(path);
    auto first = loader.nextFrame();
    require(first.has_value(), "first video frame is missing");
    require(first->image.width == 32 && first->image.height == 24,
            "video dimensions changed");
    require(first->image.pixels.size() == 768, "video pixel storage is invalid");
    require(first->timestamp_us && *first->timestamp_us == 0,
            "first presentation timestamp is incorrect");
    const auto saved = first->image.at(0, 0).color;
    int count = 1;
    while (auto frame = loader.nextFrame()) {
        require(frame->timestamp_us && *frame->timestamp_us == count * 200000,
                "presentation order or time base conversion is incorrect");
        require(frame->duration_us && *frame->duration_us == 200000,
                "frame duration is incorrect");
        ++count;
    }
    require(count == 5, "delayed frames were lost at EOF");
    require(!loader.nextFrame() && !loader.nextFrame(), "EOF must remain stable");
    require(first->image.at(0, 0).color.r == saved.r &&
            first->image.at(0, 0).color.g == saved.g &&
            first->image.at(0, 0).color.b == saved.b,
            "later decoding changed an earlier owned frame");
}

// Rebasing timestamps, decoding audio, or freeing returned pixels breaks this test.
void ignoresAudioAndPreservesSourceTime(const std::string& path)
{
    std::optional<asciixel::VideoFrame> first;
    {
        asciixel::VideoLoader loader(path);
        first = loader.nextFrame();
        require(first && first->timestamp_us && *first->timestamp_us == 3000000,
                "source timestamp must not be rebased");
        const auto second = loader.nextFrame();
        require(second && second->timestamp_us && *second->timestamp_us == 3200000,
                "audio packets interfered with video frames");
        require(!loader.nextFrame(), "unexpected extra frame in audio/video fixture");
    }

    const auto color = first->image.at(0, 0).color;
    require(color.r >= 243 && color.g <= 2 && color.b <= 2,
            "owned red pixels did not survive loader destruction");
}

// The same decoder loop must also finish a stream without delayed output.
void loadsSingleFrame(const std::string& path)
{
    asciixel::VideoLoader loader(path);
    const auto frame = loader.nextFrame();
    require(frame && frame->image.width == 2 && frame->image.height == 1,
            "single-frame stream failed");
    require(frame->image.at(0, 0).color.r == 255 &&
            frame->image.at(1, 0).color.g == 255,
            "single-frame pixels changed");
    require(!loader.nextFrame(), "single-frame stream did not finish");
}

void rejectsInput(const std::string& path)
{
    bool threw = false;
    try {
        asciixel::VideoLoader loader(path);
    }
    catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "invalid or audio-only input must fail");
}

} // namespace

int main(int argc, char** argv)
{
    try {
        require(argc == 6,
                "expected video, audio-only, invalid, audio/video and PNG paths");
        loadsAllFrames(argv[1]);
        rejectsInput(std::string(argv[1]) + ".missing");
        rejectsInput(argv[2]);
        rejectsInput(argv[3]);
        ignoresAudioAndPreservesSourceTime(argv[4]);
        loadsSingleFrame(argv[5]);
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
