#include "asciixel/core/video_timeline.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void resolvesMissingAndBackwardTimes()
{
    asciixel::VideoTimeline timeline(40000);
    require(timeline.advance(std::nullopt, std::nullopt) == 0, "first time must be zero");
    require(timeline.endTime() == 40000, "stream interval was not used");
    require(timeline.advance(1000000, std::nullopt) == 40000,
            "first valid timestamp must join the synthetic timeline");
    require(timeline.advance(1040000, 50000) == 80000, "source interval changed");
    require(timeline.advance(std::nullopt, 50000) == 130000,
            "missing timestamp did not use the preceding duration");
    require(timeline.advance(1040000, std::nullopt) == 180000,
            "repeated timestamp moved playback backward");
    require(timeline.advance(1080000, std::nullopt) == 220000,
            "timeline was not re-anchored after backward time");
    require(timeline.endTime() == 260000, "final hold is incorrect");
}

void preservesVariableIntervalsAndFallbacks()
{
    asciixel::VideoTimeline timeline(std::nullopt);
    require(timeline.advance(-1000000, 0) == 0, "negative source start was not rebased");
    require(timeline.advance(-950000, -1) == 50000, "variable source interval was lost");
    require(timeline.advance(std::nullopt, std::nullopt) == 83333,
            "invalid durations must fall back to 30 fps");
    require(timeline.endTime() == 116666, "fallback last-frame hold is incorrect");
}

void rejectsOverflow()
{
    asciixel::VideoTimeline timeline(40000);
    timeline.advance(std::numeric_limits<std::int64_t>::min(), 40000);
    bool rejected = false;
    try {
        timeline.advance(std::numeric_limits<std::int64_t>::max(), 40000);
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "timestamp subtraction must not overflow");
}

} // namespace

int main()
{
    try {
        resolvesMissingAndBackwardTimes();
        preservesVariableIntervalsAndFallbacks();
        rejectsOverflow();
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
