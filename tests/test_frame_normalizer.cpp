#include "asciixel/io/frame_normalizer.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
}

#include <cmath>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireByte(std::uint8_t actual, std::uint8_t expected, const char* message)
{
    require(actual == expected, message);
}

void preservesDimensionsAndSrgbBytes()
{
    std::uint8_t pixels[] = {255, 0, 0, 255, 128, 128, 128, 255};
    AVFrame source{};
    source.width = 2;
    source.height = 1;
    source.format = AV_PIX_FMT_RGBA;
    source.data[0] = pixels;
    source.linesize[0] = sizeof(pixels);

    const auto result = asciixel::normalizeFrame(source, {0, 0, 0});
    require(result.width == 2 && result.height == 1, "frame dimensions changed");
    require(result.pixels.size() == 2, "frame pixel count changed");
    requireByte(result.at(0, 0).color.r, 255, "red channel changed");
    requireByte(result.at(0, 0).color.g, 0, "green channel changed");
    requireByte(result.at(0, 0).color.b, 0, "blue channel changed");
    requireByte(result.at(1, 0).color.r, 128, "sRGB red byte changed");
    requireByte(result.at(1, 0).color.g, 128, "sRGB green byte changed");
    requireByte(result.at(1, 0).color.b, 128, "sRGB blue byte changed");
}

void compositesTransparentPixelsInLinearLight()
{
    std::uint8_t pixels[] = {0, 0, 255, 128, 255, 0, 0, 0};
    AVFrame source{};
    source.width = 2;
    source.height = 1;
    source.format = AV_PIX_FMT_RGBA;
    source.data[0] = pixels;
    source.linesize[0] = sizeof(pixels);

    const auto result = asciixel::normalizeFrame(source, {124, 170, 203});
    requireByte(result.at(0, 0).color.r, 89, "half-transparent red blend is wrong");
    requireByte(result.at(0, 0).color.g, 124, "half-transparent green blend is wrong");
    requireByte(result.at(0, 0).color.b, 231, "half-transparent blue blend is wrong");
    requireByte(result.at(1, 0).color.r, 124, "transparent red should equal background");
    requireByte(result.at(1, 0).color.g, 170, "transparent green should equal background");
    requireByte(result.at(1, 0).color.b, 203, "transparent blue should equal background");
}

// Preserve opaque bytes exactly and check linear-light compositing against
// an independent double-precision reference for every possible byte input.
void preservesAllSrgbByteValues()
{
    std::array<std::uint8_t, 256 * 2 * 4> pixels{};
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t channel = 0; channel < 256; ++channel) {
            const auto offset = (row * 256 + channel) * 4;
            pixels[offset] = static_cast<std::uint8_t>(channel);
            pixels[offset + 1] = static_cast<std::uint8_t>(255 - channel);
            pixels[offset + 2] = static_cast<std::uint8_t>((channel + 127) % 256);
            pixels[offset + 3] = row == 0 ? 255 : 128;
        }
    }
    AVFrame source{};
    source.width = 256;
    source.height = 2;
    source.format = AV_PIX_FMT_RGBA;
    source.data[0] = pixels.data();
    source.linesize[0] = 256 * 4;
    const auto result = asciixel::normalizeFrame(source, {124, 170, 203});
    const std::array<double, 3> background{124, 170, 203};
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t channel = 0; channel < 256; ++channel) {
            const auto offset = (row * 256 + channel) * 4;
            const auto color = result.at(channel, row).color;
            const std::array<unsigned int, 3> actual{color.r, color.g, color.b};
            for (std::size_t component = 0; component < 3; ++component) {
                const double encoded = pixels[offset + component] / 255.0;
                const double linear = encoded <= 0.04045 ? encoded / 12.92
                    : std::pow((encoded + 0.055) / 1.055, 2.4);
                const double alpha = pixels[offset + 3] / 255.0;
                const double bg = background[component] / 255.0;
                const double bg_linear = bg <= 0.04045 ? bg / 12.92
                    : std::pow((bg + 0.055) / 1.055, 2.4);
                const double blend = alpha * linear + (1.0 - alpha) * bg_linear;
                const double encoded_blend = blend <= 0.0031308 ? blend * 12.92
                    : 1.055 * std::pow(blend, 1.0 / 2.4) - 0.055;
                const auto expected = std::lround(encoded_blend * 255.0);
                require(actual[component] == expected,
                        "sRGB byte conversion or alpha compositing changed");
            }
        }
    }
}

void rejectsUnsupportedPixelFormat()
{
    AVFrame source{};
    source.width = 1;
    source.height = 1;
    source.format = AV_PIX_FMT_NONE;

    bool threw = false;
    try {
        asciixel::normalizeFrame(source, {0, 0, 0});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "unsupported pixel format should raise runtime_error");
}

bool sawDeprecatedPixelFormatWarning = false;

void collectWarnings(void*, int level, const char* format, va_list args)
{
    if (level > AV_LOG_WARNING) {
        return;
    }
    char message[256];
    std::vsnprintf(message, sizeof(message), format, args);
    if (std::string(message).find("deprecated pixel format used") != std::string::npos) {
        sawDeprecatedPixelFormatWarning = true;
    }
}

void convertsFullRangeJpegWithoutDeprecatedFormatWarning()
{
    std::uint8_t y[] = {16};
    std::uint8_t u[] = {128};
    std::uint8_t v[] = {128};
    AVFrame source{};
    source.width = 1;
    source.height = 1;
    source.format = AV_PIX_FMT_YUVJ444P;
    source.color_range = AVCOL_RANGE_JPEG;
    source.data[0] = y;
    source.data[1] = u;
    source.data[2] = v;
    source.linesize[0] = source.linesize[1] = source.linesize[2] = 1;

    av_log_set_callback(collectWarnings);
    const auto result = asciixel::normalizeFrame(source, {0, 0, 0});
    av_log_set_callback(av_log_default_callback);

    requireByte(result.at(0, 0).color.r, 16, "full-range JPEG luma was converted as limited-range");
    require(!sawDeprecatedPixelFormatWarning, "deprecated JPEG pixel format warning was emitted");
}

} // namespace

int main()
{
    try {
        preservesDimensionsAndSrgbBytes();
        compositesTransparentPixelsInLinearLight();
        preservesAllSrgbByteValues();
        rejectsUnsupportedPixelFormat();
        convertsFullRangeJpegWithoutDeprecatedFormatWarning();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}

