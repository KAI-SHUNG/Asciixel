#ifndef ASCIIXEL_IO_TEXT_WRITER_HPP
#define ASCIIXEL_IO_TEXT_WRITER_HPP

#include "asciixel/model/ascii_frame.hpp"
#include <iosfwd>
#include <string>

namespace asciixel {

// Plain ASCII text: preserve trailing spaces and LF after every row.
// Does not flush, emit terminal controls or change platform output modes.
void writeText(const AsciiFrame& frame, std::ostream& output);

// Static stdout output, including redirection; restore platform modes afterwards.
void writeTextToStdout(const AsciiFrame& frame);

// UTF-8 path, exclusive creation; no BOM. Remove partial files on failure.
void writeTextFile(const AsciiFrame& frame, const std::string& path);

} // namespace asciixel
#endif
