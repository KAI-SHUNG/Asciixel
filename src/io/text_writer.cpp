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
void validateFrame(const AsciiFrame& frame)
{
    if (!frame.width || !frame.height ||
        frame.width > std::numeric_limits<std::size_t>::max() / frame.height ||
        frame.pixels.size() != frame.width * frame.height)
        throw std::invalid_argument("Invalid ASCII frame dimensions");
    for (const auto& pixel : frame.pixels) {
        const auto ch = static_cast<unsigned char>(pixel.character);
        if (ch < 32 || ch > 126)
            throw std::invalid_argument("Frame must contain printable ASCII");
    }
}

template<typename Write>
void writeRows(const AsciiFrame& frame, Write write)
{
    std::string row(frame.width, ' ');
    for (std::size_t y = 0; y < frame.height; ++y) {
        for (std::size_t x = 0; x < frame.width; ++x)
            row[x] = frame.pixels[y * frame.width + x].character;
        write(row.data(), row.size());
        write("\n", 1);
    }
}
} // namespace

void writeAsciiFrame(const AsciiFrame& frame)
{
    validateFrame(frame);
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        throw std::runtime_error("Cannot set stdout to binary mode");
#endif
    writeRows(frame, [](const char* bytes, std::size_t size) {
        std::cout.write(bytes, static_cast<std::streamsize>(size));
    });
    if (!std::cout) throw std::runtime_error("Cannot write ASCII frame");
}

void writeAsciiFile(const AsciiFrame& frame, const std::string& path)
{
    validateFrame(frame);
    const auto destination = std::filesystem::u8path(path);
#ifdef _WIN32
    const int descriptor = _wopen(destination.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
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
    if (!raw) throw std::runtime_error("Cannot create TXT (file may already exist): " + path);
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file(raw, std::fclose);
    try {
        writeRows(frame, [&](const char* bytes, std::size_t size) {
            if (std::fwrite(bytes, 1, size, file.get()) != size)
                throw std::runtime_error("Cannot write TXT: " + path);
        });
        if (std::fclose(file.release()) != 0)
            throw std::runtime_error("Cannot close TXT: " + path);
    } catch (...) {
        file.reset();
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw;
    }
}
} // namespace asciixel
