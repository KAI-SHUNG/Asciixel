#include "asciixel/io/text_writer.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#endif

namespace asciixel {
namespace {

/**
 * @brief Check character-frame storage and printable ASCII contents.
 *
 * @param frame Frame to validate before serialization.
 *
 * @return No value; inconsistent dimensions or invalid characters throw.
 */
void validateFrame(const AsciiFrame& frame)
{
    // Reject empty or inconsistent storage before iterating over characters.
    if (!frame.width || !frame.height ||
        frame.width > std::numeric_limits<std::size_t>::max() / frame.height ||
        frame.pixels.size() != frame.width * frame.height)
        throw std::invalid_argument("Invalid ASCII frame dimensions");

    // ASCII output excludes control characters and multibyte encodings.
    for (const auto& pixel : frame.pixels) {
        const auto ch = static_cast<unsigned char>(pixel.character);
        if (ch < 32 || ch > 126)
            throw std::invalid_argument("Frame must contain printable ASCII");
    }
}

/**
 * @brief Serialize rows with trailing spaces preserved and LF terminators.
 *
 * @tparam Write Callable accepting a byte pointer and byte count.
 * @param frame Validated character frame.
 * @param write Synchronous byte consumer; must not retain the supplied pointer.
 *
 * @return No value; exceptions from the byte consumer propagate.
 */
template<typename Write>
void writeRows(const AsciiFrame& frame, Write write)
{
    // Reuse one row buffer while emitting every character, including spaces.
    std::string row(frame.width, ' ');
    for (std::size_t y = 0; y < frame.height; ++y) {
        for (std::size_t x = 0; x < frame.width; ++x)
            row[x] = frame.pixels[y * frame.width + x].character;

        // Terminate every row, including the final one, with a single LF.
        write(row.data(), row.size());
        write("\n", 1);
    }
}
} // namespace

// Shared by static stdout, video playback and arbitrary output streams.
// Keeps one row buffer; never constructs or copies a complete text frame.
void writeText(const AsciiFrame& frame, std::ostream& output)
{
    validateFrame(frame);
    writeRows(frame, [&](const char* bytes, std::size_t size) {
        output.write(bytes, static_cast<std::streamsize>(size));
    });
    if (!output) throw std::runtime_error("Cannot write ASCII text");
}

void writeTextToStdout(const AsciiFrame& frame)
{
#ifdef _WIN32
    // Scope byte-mode changes to static output. Video owns its mode per session.
    struct StdoutMode {
        int previous;
        StdoutMode() : previous(_setmode(_fileno(stdout), _O_BINARY))
        {
            if (previous == -1) throw std::runtime_error("Cannot set stdout to binary mode");
        }
        ~StdoutMode() { _setmode(_fileno(stdout), previous); }
    } mode;
#endif
    writeText(frame, std::cout);
#ifdef _WIN32
    // Flush while byte mode is still active, including redirected stdout.
    std::cout.flush();
    if (!std::cout) throw std::runtime_error("Cannot flush ASCII text");
#endif
}

/**
 * @brief Save a character frame as UTF-8-compatible ASCII without a BOM.
 *
 * @param frame Valid printable ASCII frame; trailing spaces are preserved.
 * @param path UTF-8 destination path; existing files are never overwritten.
 *
 * @return No value; validation or file errors throw an exception.
 */
void writeTextFile(const AsciiFrame& frame, const std::string& path)
{
    // Validate contents before attempting to create the destination.
    validateFrame(frame);
    const auto destination = std::filesystem::u8path(path);

    // Use exclusive creation and native Unicode paths on Windows.
#ifdef _WIN32
    const int descriptor = _wopen(destination.c_str(),
                                  _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
    if (descriptor < 0)
        throw std::runtime_error("Cannot create TXT (file may already exist): " + path);
    std::FILE* raw = _fdopen(descriptor, "wb");
    if (!raw) {
        _close(descriptor);
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw std::runtime_error("Cannot open TXT output stream: " + path);
    }
#else
    std::FILE* raw = std::fopen(destination.c_str(), "wbx");
#endif
    if (!raw) {
        throw std::runtime_error("Cannot create TXT (file may already exist): " + path);
    }

    // Own the stream while writing rows and checking the final close.
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file(raw, std::fclose);
    try {
        writeRows(frame, [&](const char* bytes, std::size_t size) {
            if (std::fwrite(bytes, 1, size, file.get()) != size)
                throw std::runtime_error("Cannot write TXT: " + path);
        });
        if (std::fclose(file.release()) != 0)
            throw std::runtime_error("Cannot close TXT: " + path);
    } catch (...) {
        // Close the stream before removing a partially written file.
        file.reset();
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw;
    }
}
} // namespace asciixel
