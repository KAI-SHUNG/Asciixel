#include "asciixel/io/terminal_session.hpp"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <atomic>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <csignal>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace asciixel {
namespace {
// Handlers only record interruption; playback performs all cleanup on its thread.
#ifdef _WIN32
std::atomic<bool> video_interrupted{false};

BOOL WINAPI handleVideoInterrupt(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT) {
        video_interrupted.store(true);
        return TRUE;
    }
    return FALSE;
}
#else
volatile std::sig_atomic_t video_interrupted = 0;

void handleVideoInterrupt(int)
{
    video_interrupted = 1;
}
#endif

} // namespace

struct TerminalSession::Impl {
    bool cursor_hidden = false;
    bool handler_installed = false;
#ifdef _WIN32
    HANDLE output = INVALID_HANDLE_VALUE;
    DWORD original_mode = 0;
    CONSOLE_CURSOR_INFO original_cursor{};
    int original_stdout_mode = -1;
    bool mode_changed = false;
    bool cursor_saved = false;
#else
    termios original_mode{};
    bool mode_changed = false;
    using SignalHandler = void (*)(int);
    SignalHandler original_handler = SIG_DFL;
#endif

    // Best-effort restoration must also work after a partially failed constructor.
    ~Impl()
    {
#ifdef _WIN32
        if (cursor_saved) {
            SetConsoleCursorInfo(output, &original_cursor);
        }
        if (handler_installed) {
            SetConsoleCtrlHandler(handleVideoInterrupt, FALSE);
        }
        if (mode_changed) {
            SetConsoleMode(output, original_mode);
        }
        if (original_stdout_mode != -1) {
            _setmode(_fileno(stdout), original_stdout_mode);
        }
#else
        if (cursor_hidden) {
            const char restore[] = "\x1b[?25h";
            // Bypass a failed ostream so its error state cannot suppress cleanup.
            const auto ignored = ::write(STDOUT_FILENO, restore, sizeof(restore) - 1);
            (void)ignored;
        }
        if (handler_installed) {
            std::signal(SIGINT, original_handler);
        }
        if (mode_changed) {
            tcsetattr(STDOUT_FILENO, TCSANOW, &original_mode);
        }
#endif
    }
};

/**
 * @brief Prepare an interactive terminal for relative-position video output.
 *
 * @return No value; unavailable terminal or mode changes throw runtime_error.
 */
TerminalSession::TerminalSession() : impl_(std::make_unique<Impl>())
{
#ifdef _WIN32
    impl_->output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(impl_->output, &impl_->original_mode)) {
        throw std::runtime_error("Video playback requires an interactive terminal");
    }
    if (!GetConsoleCursorInfo(impl_->output, &impl_->original_cursor)) {
        throw std::runtime_error("Cannot read terminal cursor state");
    }
    impl_->cursor_saved = true;

    // Process LF as a newline and enable ANSI cursor controls only for video.
    const DWORD mode = (impl_->original_mode | ENABLE_PROCESSED_OUTPUT |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING) &
                       ~DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(impl_->output, mode)) {
        throw std::runtime_error("Cannot enable terminal cursor controls");
    }
    impl_->mode_changed = true;
    impl_->original_stdout_mode = _setmode(_fileno(stdout), _O_BINARY);
    if (impl_->original_stdout_mode == -1) {
        throw std::runtime_error("Cannot set video stdout to binary mode");
    }
    video_interrupted.store(false);
    if (!SetConsoleCtrlHandler(handleVideoInterrupt, TRUE)) {
        throw std::runtime_error("Cannot install video interrupt handler");
    }
#else
    if (!isatty(STDOUT_FILENO) ||
        tcgetattr(STDOUT_FILENO, &impl_->original_mode) != 0) {
        throw std::runtime_error("Video playback requires an interactive terminal");
    }
    auto mode = impl_->original_mode;
    mode.c_oflag |= OPOST | ONLCR;
    mode.c_oflag &= ~(OCRNL | ONOCR | ONLRET);
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &mode) != 0) {
        throw std::runtime_error("Cannot enable terminal newline processing");
    }
    impl_->mode_changed = true;
    video_interrupted = 0;
    impl_->original_handler = std::signal(SIGINT, handleVideoInterrupt);
    if (impl_->original_handler == SIG_ERR) {
        throw std::runtime_error("Cannot install video interrupt handler");
    }
#endif
    impl_->handler_installed = true;

    // Start at column one of the current line; keep the screen and row intact.
    impl_->cursor_hidden = true;
    std::cout << "\r\x1b[?25l" << std::flush;
    if (!std::cout) {
        throw std::runtime_error("Cannot hide terminal cursor");
    }
}

TerminalSession::~TerminalSession() = default;

/**
 * @brief Query the current visible terminal capacity before writing a frame.
 */
TerminalSize TerminalSession::size() const
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(impl_->output, &info)) {
        throw std::runtime_error("Cannot query terminal dimensions");
    }
    return {static_cast<std::size_t>(info.srWindow.Right - info.srWindow.Left + 1),
            static_cast<std::size_t>(info.srWindow.Bottom - info.srWindow.Top + 1)};
#else
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) {
        throw std::runtime_error("Cannot query terminal dimensions");
    }
    return {size.ws_col, size.ws_row};
#endif
}

// Called outside the handler, including between short slices of a timed wait.
bool TerminalSession::interrupted() const
{
#ifdef _WIN32
    return video_interrupted.load();
#else
    return video_interrupted != 0;
#endif
}

} // namespace asciixel
