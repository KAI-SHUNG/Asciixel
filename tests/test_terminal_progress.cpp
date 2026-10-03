#include "asciixel/io/terminal_progress.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main()
{
    try {
        std::ostringstream output;
        {
            asciixel::TerminalProgress progress(output, "Work");
            progress.update(5, 10, "5 items");
            require(output.str().find("50%") != std::string::npos &&
                    output.str().find("5 items") != std::string::npos,
                    "generic progress must display the supplied fraction and detail");
            const auto first = output.str();
            progress.update(6, 10, "6 items");
            require(output.str() == first, "rapid updates must be throttled");
            progress.update(10, 10, "10 items", true);
            require(output.str().find("100%") != std::string::npos,
                    "completion must refresh despite throttling");
            progress.clear();
            const auto cleared = output.str();
            require(cleared.substr(cleared.size() - 5) == "\r\x1b[2K",
                    "clear must erase the line without advancing the cursor");
            progress.clear();
            require(output.str() == cleared, "clearing twice must not emit extra output");
        }
        output.str("");
        try {
            asciixel::TerminalProgress progress(output, "Work");
            progress.update(3, std::nullopt, "3 items");
            require(output.str().find('%') == std::string::npos &&
                    output.str().find("3 items") != std::string::npos,
                    "unknown totals must show activity and detail without a percentage");
            throw std::runtime_error("operation failed");
        } catch (const std::runtime_error& error) {
            if (std::string(error.what()) != "operation failed") throw;
        }
        require(output.str().substr(output.str().size() - 5) == "\r\x1b[2K",
                "exception cleanup must clear the progress line");
        output.str("");
        {
            asciixel::TerminalProgress progress(output, "Work");
            progress.update(12, 10, "12 items");
            require(output.str().find("99%") != std::string::npos &&
                    output.str().find("100%") == std::string::npos,
                    "estimated totals must not report completion prematurely");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
