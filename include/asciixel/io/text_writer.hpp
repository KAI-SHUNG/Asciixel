#ifndef ASCIIXEL_TEXT_WRITER_HPP
#define ASCIIXEL_TEXT_WRITER_HPP

#include "asciixel/model/ascii_frame.hpp"
#include <string>

namespace asciixel {

// Write the frame to stdout, preserving trailing spaces and LF line endings.
void writeAsciiFrame(const AsciiFrame& frame);
// UTF-8 path, exclusive creation; ASCII is UTF-8, no BOM, LF line endings.
void writeAsciiFile(const AsciiFrame& frame, const std::string& path);

} // namespace asciixel
#endif
