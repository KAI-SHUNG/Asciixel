#ifndef ASCIIXEL_IO_TERMINAL_VIDEO_OUTPUT_HPP
#define ASCIIXEL_IO_TERMINAL_VIDEO_OUTPUT_HPP

#include "asciixel/model/ascii_frame.hpp"

#include <cstdint>
#include <memory>

namespace asciixel {

// Own terminal modes, cursor state, interruption handling and the playback clock.
// Construct before preparation; the clock starts only on the first waitUntil.
class TerminalVideoOutput {
public:
    TerminalVideoOutput();
    ~TerminalVideoOutput();
    TerminalVideoOutput(const TerminalVideoOutput&) = delete;
    TerminalVideoOutput& operator=(const TerminalVideoOutput&) = delete;

    bool interrupted() const;
    bool waitUntil(std::int64_t time_us);
    void writeFrame(const AsciiFrame& frame);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace asciixel

#endif
