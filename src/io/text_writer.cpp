#include "asciixel/io/text_writer.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#ifdef _WIN32
#include <atomic>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
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

/**
 * @brief Check character-frame storage and printable ASCII contents.
 *
 * @param frame Frame to validate before serialization.
 *
 * @return No value; inconsistent dimensions or invalid characters throw.
 */
void validateFrame(const AsciiFrame& frame)
{
    // Reject empty or inconsistent storage before iterating over characters.
    if (!frame.width || !frame.height ||
        frame.width > std::numeric_limits<std::size_t>::max() / frame.height ||
        frame.pixels.size() != frame.width * frame.height)
        throw std::invalid_argument("Invalid ASCII frame dimensions");

    // ASCII output excludes control characters and multibyte encodings.
    for (const auto& pixel : frame.pixels) {
        const auto ch = static_cast<unsigned char>(pixel.character);
        if (ch < 32 || ch > 126)
            throw std::invalid_argument("Frame must contain printable ASCII");
    }
}

/**
 * @brief Serialize rows with trailing spaces preserved and LF terminators.
 *
 * @tparam Write Callable accepting a byte pointer and byte count.
 * @param frame Validated character frame.
 * @param write Synchronous byte consumer; must not retain the supplied pointer.
 *
 * @return No value; exceptions from the byte consumer propagate.
 */
template<typename Write>
void writeRows(const AsciiFrame& frame, Write write)
{
    // Reuse one row buffer while emitting every character, including spaces.
    std::string row(frame.width, ' ');
    for (std::size_t y = 0; y < frame.height; ++y) {
        for (std::size_t x = 0; x < frame.width; ++x)
            row[x] = frame.pixels[y * frame.width + x].character;

        // Terminate every row, including the final one, with a single LF.
        write(row.data(), row.size());
        write("\n", 1);
    }
}
} // namespace

/**
 * @brief Return the cursor to the start of a previously written frame.
 *
 * @param rows Number of output lines since the frame started; zero is a no-op.
 *
 * @return No value; stdout failures throw std::runtime_error.
 */
void resetCursor(std::size_t rows)
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

struct TextTerminalSession::Impl {
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
TextTerminalSession::TextTerminalSession() : impl_(std::make_unique<Impl>())
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

TextTerminalSession::~TextTerminalSession() = default;

/**
 * @brief Query the current visible terminal capacity before writing a frame.
 */
TerminalSize TextTerminalSession::size() const
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
bool TextTerminalSession::interrupted() const
{
#ifdef _WIN32
    return video_interrupted.load();
#else
    return video_interrupted != 0;
#endif
}

/**
 * @brief Write a character frame to standard output without ANSI formatting.
 *
 * @param frame Valid printable ASCII frame.
 *
 * @return No value; invalid frames or output failures throw.
 */
void writeAsciiFrame(const AsciiFrame& frame)
{
    // Validate first, then preserve literal LF bytes on Windows.
    validateFrame(frame);
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        throw std::runtime_error("Cannot set stdout to binary mode");
#endif

    // Stream rows to stdout and report any stream failure.
    writeRows(frame, [](const char* bytes, std::size_t size) {
        std::cout.write(bytes, static_cast<std::streamsize>(size));
    });
    if (!std::cout) throw std::runtime_error("Cannot write ASCII frame");
}

/**
 * @brief Save a character frame as UTF-8-compatible ASCII without a BOM.
 *
 * @param frame Valid printable ASCII frame; trailing spaces are preserved.
 * @param path UTF-8 destination path; existing files are never overwritten.
 *
 * @return No value; validation or file errors throw an exception.
 */
void writeAsciiFile(const AsciiFrame& frame, const std::string& path)
{
    // Validate contents before attempting to create the destination.
    validateFrame(frame);
    const auto destination = std::filesystem::u8path(path);

    // Use exclusive creation and native Unicode paths on Windows.
#ifdef _WIN32
    const int descriptor = _wopen(destination.c_str(),
                                  _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
    if (descriptor < 0)
        throw std::runtime_error("Cannot create TXT (file may already exist): " + path);
    std::FILE* raw = _fdopen(descriptor, "wb");
    if (!raw) {
        _close(descriptor);
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw std::runtime_error("Cannot open TXT output stream: " + path);
    }
#else
    std::FILE* raw = std::fopen(destination.c_str(), "wbx");
#endif
    if (!raw) {
        throw std::runtime_error("Cannot create TXT (file may already exist): " + path);
    }

    // Own the stream while writing rows and checking the final close.
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file(raw, std::fclose);
    try {
        writeRows(frame, [&](const char* bytes, std::size_t size) {
            if (std::fwrite(bytes, 1, size, file.get()) != size)
                throw std::runtime_error("Cannot write TXT: " + path);
        });
        if (std::fclose(file.release()) != 0)
            throw std::runtime_error("Cannot close TXT: " + path);
    } catch (...) {
        // Close the stream before removing a partially written file.
        file.reset();
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw;
    }
}
} // namespace asciixel
