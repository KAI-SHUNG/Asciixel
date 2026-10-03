#include "asciixel/io/text_writer.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

// Restore stdout even when an assertion or a write operation throws.
class CaptureStdout {
public:
    CaptureStdout() : previous_(std::cout.rdbuf(buffer_.rdbuf())) {}

    ~CaptureStdout()
    {
        std::cout.clear();
        std::cout.rdbuf(previous_);
    }

    std::string bytes() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* previous_;
};

void resetsOnlyWhenRequested()
{
    CaptureStdout output;
    asciixel::AsciiFrame frame(3, 2);
    const std::string characters = "A  #. ";
    for (std::size_t i = 0; i < characters.size(); ++i) {
        frame.pixels[i].character = characters[i];
    }

    // Static image output must contain no cursor controls.
    asciixel::writeAsciiFrame(frame);
    if (output.bytes() != "A  \n#. \n") {
        throw std::runtime_error("Static stdout bytes changed");
    }

    // No previous frame means no movement; ANSI parameter zero would move once.
    asciixel::resetCursor(0);
    if (output.bytes() != "A  \n#. \n") {
        throw std::runtime_error("Zero rows must not move the cursor");
    }

    // Move relative to the preceding frame, then reuse the same text writer.
    asciixel::resetCursor(frame.height);
    asciixel::writeAsciiFrame(frame);
    if (output.bytes() != "A  \n#. \n\x1b[2FA  \n#. \n") {
        throw std::runtime_error("Relative cursor reset or frame output is incorrect");
    }
}

void reportsResetWriteFailure()
{
    CaptureStdout output;
    std::cout.setstate(std::ios::badbit);
    bool rejected = false;
    try {
        asciixel::resetCursor(2);
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("Cursor reset must report output failure");
    }
}

} // namespace

int main(int argc, char** argv) {
    resetsOnlyWhenRequested();
    reportsResetWriteFailure();
    if (argc != 2) throw std::runtime_error("Expected output path");
    const auto path = std::filesystem::u8path(argv[1]);
    std::filesystem::remove(path);
    asciixel::AsciiFrame frame(3, 2);
    const std::string chars = "A  #. ";
    for (std::size_t i = 0; i < chars.size(); ++i) frame.pixels[i].character = chars[i];
    asciixel::writeAsciiFile(frame, argv[1]);
    bool rejected = false;
    try { asciixel::writeAsciiFile(frame, argv[1]); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Existing output must be preserved");
    std::ifstream input(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes != "A  \n#. \n") throw std::runtime_error("TXT bytes differ: trailing spaces/LF/BOM");
    input.close();
    std::filesystem::remove(path);
}
