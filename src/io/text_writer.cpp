#include "asciixel/io/text_writer.hpp"

#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace asciixel {

void writeAsciiFrame(const AsciiFrame& frame)
{
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        throw std::runtime_error("Cannot set stdout to binary mode");
    }
#endif
    for (std::size_t y = 0; y < frame.height; ++y) {
        for (std::size_t x = 0; x < frame.width; ++x) {
            std::cout.put(frame.pixels[y * frame.width + x].character);
        }
        std::cout.put('\n');
    }
    if (!std::cout) {
        throw std::runtime_error("Cannot write ASCII frame");
    }
}

} // namespace asciixel
