#include "asciixel/io/terminal_video_output.hpp"
#include "asciixel/io/text_writer.hpp"

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
    TextTerminalSession terminal;
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
    resetCursor(impl_->previous_rows);
    writeAsciiFrame(frame);
    std::cout.flush();
    if (!std::cout) {
        throw std::runtime_error("Cannot flush video frame");
    }
    impl_->previous_rows = frame.height;
}

} // namespace asciixel
