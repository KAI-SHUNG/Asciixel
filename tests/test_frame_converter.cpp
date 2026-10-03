#include "asciixel/app/frame_converter.hpp"

#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <cstdint>
#include <type_traits>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void convertsSuccessiveFramesWithSharedCharset()
{
    asciixel::RasterizedCharset charset;
    charset.layout = {1, 2, 0, 1};
    asciixel::RasterizedGlyph space;
    space.character = ' ';
    space.density = 0.0f;
    asciixel::RasterizedGlyph solid;
    solid.character = '#';
    solid.density = 1.0f;
    charset.glyphs = {space, solid};

    asciixel::ImageFrame image(4, 4);
    for (std::size_t y = 0; y < 4; ++y) {
        for (std::size_t x = 0; x < 4; ++x) {
            const std::uint8_t level = x < 2 ? 0 : 255;
            image.at(x, y).color = {level, level, level};
        }
    }
    const auto first = asciixel::convertFrame(image, charset, {2});
    require(first.width == 2 && first.height == 1,
            "conversion must honor columns and font cell aspect ratio");
    require(first.pixels.size() == 2 && first.pixels[0].character == ' ' &&
                first.pixels[1].character == '#',
            "conversion must map dark and bright blocks to their glyphs");

    for (auto& pixel : image.pixels) pixel.color = {255, 255, 255};
    const auto second = asciixel::convertFrame(image, charset, {2});
    require(second.pixels.size() == 2 && second.pixels[0].character == '#' &&
                second.pixels[1].character == '#',
            "successive conversions must use the current image with the shared charset");
    require(first.pixels[0].character == ' ', "previous frame must remain unchanged");
}
// Catch loss of dark sRGB levels or gamma errors when converting byte images
// through the linear sample grid back into byte character colors.
void preservesAllByteColorsThroughConversion()
{
    asciixel::RasterizedCharset charset;
    charset.layout = {1, 1, 0, 0};
    asciixel::RasterizedGlyph dark, light;
    dark.character = ' '; dark.density = 0;
    light.character = '#'; light.density = 1;
    charset.glyphs = {dark, light};
    asciixel::ImageFrame image(256, 1);
    for (std::size_t i = 0; i < 256; ++i) {
        image.pixels[i].color = {static_cast<std::uint8_t>(i),
                                static_cast<std::uint8_t>(255 - i),
                                static_cast<std::uint8_t>((i + 127) % 256)};
    }
    const auto result = asciixel::convertFrame(image, charset, {256});
    for (std::size_t i = 0; i < 256; ++i) {
        const auto input = image.pixels[i].color;
        const auto output = result.pixels[i].color;
        require(output.r == input.r && output.g == input.g && output.b == input.b,
                "unmixed sRGB byte colors must survive conversion exactly");
        const auto decode = [](std::uint8_t channel) {
            const double value = channel / 255.0;
            return value <= 0.04045 ? value / 12.92
                : std::pow((value + 0.055) / 1.055, 2.4);
        };
        const double luminance = 0.2126 * decode(input.r) +
            0.7152 * decode(input.g) + 0.0722 * decode(input.b);
        require(result.pixels[i].character == (luminance > 0.5 ? '#' : ' '),
                "glyph luminance must use linear light, not encoded bytes");
    }
}

// Catch accidentally retaining full-resolution float storage or quantizing
// the sample before glyph matching at a luminance decision boundary.
void storesCompactColorsAndMatchesBeforeQuantization()
{
    require(std::is_same<decltype(asciixel::Color::r), std::uint8_t>::value &&
            sizeof(asciixel::ImagePixel) == 3 && sizeof(asciixel::AsciiPixel) == 4,
            "image and ASCII colors must use compact byte storage");
    asciixel::RasterizedCharset charset;
    charset.layout = {1, 1, 0, 0};
    asciixel::RasterizedGlyph dark, middle, light;
    dark.character = ' '; dark.density = 0.0f;
    middle.character = '+'; middle.density = 0.501f;
    light.character = '#'; light.density = 1.0f;
    charset.glyphs = {dark, middle, light};
    asciixel::ImageFrame image(2, 1);
    image.pixels[0].color = {0, 0, 0};
    image.pixels[1].color = {255, 255, 255};
    const auto result = asciixel::convertFrame(image, charset, {1});
    require(result.pixels[0].character == '+', "black-white average must match in linear light");
    require(result.pixels[0].color.r == 188 && result.pixels[0].color.g == 188 &&
            result.pixels[0].color.b == 188, "black-white average must encode as sRGB 188");

    middle.density = 0.001f;
    charset.glyphs = {dark, middle, light};
    const auto boundary = asciixel::convertFrame(image, charset, {1});
    require(boundary.pixels[0].character == '+',
            "sample color quantization must not change glyph selection");
}

} // namespace

int main()
{
    try {
        storesCompactColorsAndMatchesBeforeQuantization();
        preservesAllByteColorsThroughConversion();
        convertsSuccessiveFramesWithSharedCharset();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
