#ifndef ASCIIXEL_TEXT_WRITER_HPP
#define ASCIIXEL_TEXT_WRITER_HPP

#include "asciixel/model/ascii_frame.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace asciixel {

// Write the frame to stdout, preserving trailing spaces and LF line endings.
void writeAsciiFrame(const AsciiFrame& frame);
// UTF-8 path, exclusive creation; ASCII is UTF-8, no BOM, LF line endings.
void writeAsciiFile(const AsciiFrame& frame, const std::string& path);

// Move up rows lines and return to the first column; zero rows does nothing.
// Caller must enable ANSI controls; wrapping or scrolling can affect alignment.
// Does not clear the screen, change terminal modes or flush stdout.
void resetCursor(std::size_t rows);

struct TerminalSize {
    std::size_t columns;
    std::size_t rows;
};

// Video-only preparation. Starts at column one without clearing or changing rows.
// Restores cursor visibility, output modes and interrupt handlers on destruction.
class TextTerminalSession {
public:
    TextTerminalSession();
    ~TextTerminalSession();
    TextTerminalSession(const TextTerminalSession&) = delete;
    TextTerminalSession& operator=(const TextTerminalSession&) = delete;

    TerminalSize size() const;
    bool interrupted() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace asciixel
#endif
