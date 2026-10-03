#ifndef ASCIIXEL_IO_TERMINAL_PROGRESS_HPP
#define ASCIIXEL_IO_TERMINAL_PROGRESS_HPP

#include <chrono>
#include <iosfwd>
#include <optional>
#include <string>

namespace asciixel {

// Generic single-line ANSI display using completed/total units and caller text.
// No knowledge of videos, frames or timestamps is required. Percentages remain
// below 100 until completed; an absent/nonpositive total shows activity instead.
// The stream must support ANSI controls. Clear before other terminal output.
class TerminalProgress {
public:
    TerminalProgress(std::ostream& output, std::string label);
    ~TerminalProgress();
    TerminalProgress(const TerminalProgress&) = delete;
    TerminalProgress& operator=(const TerminalProgress&) = delete;

    void update(double done, std::optional<double> total,
                const std::string& detail, bool completed = false);
    void clear();

private:
    std::ostream& output_;
    std::string label_;
    std::optional<std::chrono::steady_clock::time_point> last_update_;
    std::size_t spinner_ = 0;
    bool visible_ = false;
};

} // namespace asciixel

#endif
