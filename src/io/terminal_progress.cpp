#include "asciixel/io/terminal_progress.hpp"

#include <ostream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <utility>

namespace asciixel {
namespace {
std::optional<std::size_t> stdoutColumns()
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        return static_cast<std::size_t>(info.srWindow.Right - info.srWindow.Left + 1);
    }
#else
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
        return size.ws_col;
    }
#endif
    return std::nullopt;
}
} // namespace

TerminalProgress::TerminalProgress(std::ostream& output, std::string label)
    : output_(output), label_(std::move(label)) {}

TerminalProgress::~TerminalProgress()
{
    try { clear(); } catch (...) { }
}

void TerminalProgress::update(double done, std::optional<double> total,
                              const std::string& detail, bool completed)
{
    const auto now = std::chrono::steady_clock::now();
    if (!completed && last_update_ && now - *last_update_ < std::chrono::milliseconds(100)) {
        return;
    }
    std::ostringstream line;
    line << label_ << ' ';
    if (completed || (total && std::isfinite(*total) && *total > 0 && std::isfinite(done))) {
        const int percent = completed ? 100 :
            static_cast<int>(std::clamp(done / *total, 0.0, 0.99) * 100);
        const int filled = percent * 24 / 100;
        line << '[' << std::string(filled, '=') << std::string(24 - filled, '-')
             << "] " << percent << '%';
        line << "  " << detail;
        // Keep the percentage when the full bar does not fit the terminal.
        if (&output_ == &std::cout) {
            const auto columns = stdoutColumns();
            if (columns && line.str().size() >= *columns) {
                line.str("");
                line << label_ << ' ' << percent << "%  " << detail;
            }
        }
    }
    else {
        line << "|/-\\"[spinner_++ % 4] << "  " << detail;
    }
    auto text = line.str();
    if (&output_ == &std::cout) {
        const auto columns = stdoutColumns();
        if (columns && *columns > 0 && text.size() >= *columns) text.resize(*columns - 1);
    }
    visible_ = true;
    output_ << "\r\x1b[2K" << text << std::flush;
    last_update_ = now;
}

void TerminalProgress::clear()
{
    if (visible_) {
        output_ << "\r\x1b[2K" << std::flush;
        visible_ = false;
    }
    last_update_.reset();
}

} // namespace asciixel
