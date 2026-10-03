#ifndef ASCIIXEL_IO_TERMINAL_SESSION_HPP
#define ASCIIXEL_IO_TERMINAL_SESSION_HPP

#include <cstddef>
#include <memory>

namespace asciixel {

struct TerminalSize {
    std::size_t columns;
    std::size_t rows;
};

// Video-only preparation. Starts at column one without clearing or changing rows.
// Restores cursor visibility, output modes and interrupt handlers on destruction.
class TerminalSession {
public:
    TerminalSession();
    ~TerminalSession();
    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;

    TerminalSize size() const;
    bool interrupted() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace asciixel
#endif
