#include "asciixel/io/terminal_video_output.hpp"
#include "asciixel/io/text_writer.hpp"
#include "asciixel/io/terminal_session.hpp"

extern "C" {
#include <libavutil/log.h>
}

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace asciixel {
namespace {

/**
 * @brief Return the cursor to the start of a previously written frame.
 *
 * @param rows Number of output lines since the frame started; zero is a no-op.
 *
 * @return No value; stdout failures throw std::runtime_error.
 */
void moveToPreviousFrame(std::size_t rows)
{
    // ANSI treats zero as one, so an empty movement must emit no sequence.
    if (rows == 0) {
        return;
    }

    // Cursor Previous Line moves upward and sets the column to one.
    // The caller flushes after writing the next frame, keeping both in order.
    const std::string sequence = "\x1b[" + std::to_string(rows) + "F";
    std::cout.write(sequence.data(),
                    static_cast<std::streamsize>(sequence.size()));
    if (!std::cout) {
        throw std::runtime_error("Cannot reset terminal cursor");
    }
}


// FFmpeg diagnostics must not displace the terminal cursor during conversion.
class QuietVideoDiagnostics {
public:
    QuietVideoDiagnostics() : previous_level_(av_log_get_level())
    {
        av_log_set_level(AV_LOG_QUIET);
    }
    ~QuietVideoDiagnostics()
    {
        av_log_set_level(previous_level_);
    }
private:
    int previous_level_;
};

} // namespace

struct TerminalVideoOutput::Impl {
    QuietVideoDiagnostics diagnostics;
    TerminalSession terminal;
    std::optional<std::chrono::steady_clock::time_point> origin;
    std::size_t previous_rows = 0;
};

TerminalVideoOutput::TerminalVideoOutput() : impl_(std::make_unique<Impl>()) {}
TerminalVideoOutput::~TerminalVideoOutput() = default;

bool TerminalVideoOutput::interrupted() const
{
    return impl_->terminal.interrupted();
}

bool TerminalVideoOutput::waitUntil(std::int64_t target)
{
    if (!impl_->origin) {
        impl_->origin = std::chrono::steady_clock::now();
    }
    while (!interrupted()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - *impl_->origin).count();
        if (elapsed >= target) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(
            std::min<std::int64_t>(target - elapsed, 20000)));
    }
    return false;
}

void TerminalVideoOutput::writeFrame(const AsciiFrame& frame)
{
    moveToPreviousFrame(impl_->previous_rows);
    writeText(frame, std::cout);
    std::cout.flush();
    if (!std::cout) {
        throw std::runtime_error("Cannot flush video frame");
    }
    impl_->previous_rows = frame.height;
}

} // namespace asciixel
