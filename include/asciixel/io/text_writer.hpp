#ifndef ASCIIXEL_TEXT_WRITER_HPP
#define ASCIIXEL_TEXT_WRITER_HPP

#include "asciixel/model/ascii_frame.hpp"

namespace asciixel {

// Write the frame to stdout, preserving trailing spaces and LF line endings.
void writeAsciiFrame(const AsciiFrame& frame);

} // namespace asciixel
#endif
