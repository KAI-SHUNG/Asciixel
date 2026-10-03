#include "asciixel/io/text_writer.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

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

void writesToAnyStream()
{
    std::ostringstream output;
    CaptureStdout stdout_output;
    asciixel::AsciiFrame frame(3, 2);
    const std::string characters = "A  #. ";
    for (std::size_t i = 0; i < characters.size(); ++i)
        frame.pixels[i].character = characters[i];
    asciixel::writeText(frame, output);
    if (output.str() != "A  \n#. \n" || !stdout_output.bytes().empty())
        throw std::runtime_error("Shared text writer must target only the supplied stream");
    output.setstate(std::ios::badbit);
    bool rejected = false;
    try { asciixel::writeText(frame, output); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Stream write failure must propagate");
}

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    {
        CaptureStdout output;
        const int previous = _setmode(_fileno(stdout), _O_TEXT);
        asciixel::AsciiFrame frame(1, 1);
        frame.pixels[0].character = 'A';
        asciixel::writeTextToStdout(frame);
        const int after = _setmode(_fileno(stdout), previous);
        if (after != _O_TEXT) throw std::runtime_error("Static output must restore stdout mode");
    }
#endif
    writesToAnyStream();
    if (argc != 2) throw std::runtime_error("Expected output path");
    const auto path = std::filesystem::u8path(argv[1]);
    std::filesystem::remove(path);
    asciixel::AsciiFrame frame(3, 2);
    const std::string chars = "A  #. ";
    for (std::size_t i = 0; i < chars.size(); ++i) frame.pixels[i].character = chars[i];
    asciixel::writeTextFile(frame, argv[1]);
    bool rejected = false;
    try { asciixel::writeTextFile(frame, argv[1]); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Existing output must be preserved");
    std::ifstream input(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes != "A  \n#. \n") throw std::runtime_error("TXT bytes differ: trailing spaces/LF/BOM");
    input.close();
    std::filesystem::remove(path);
}
