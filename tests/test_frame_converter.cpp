#include "asciixel/app/frame_converter.hpp"

#include <cstdio>
#include <stdexcept>

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
            const float level = x < 2 ? 0.0f : 1.0f;
            image.at(x, y).color = {level, level, level};
        }
    }
    const auto first = asciixel::convertFrame(image, charset, {2});
    require(first.width == 2 && first.height == 1,
            "conversion must honor columns and font cell aspect ratio");
    require(first.pixels.size() == 2 && first.pixels[0].character == ' ' &&
                first.pixels[1].character == '#',
            "conversion must map dark and bright blocks to their glyphs");

    for (auto& pixel : image.pixels) pixel.color = {1.0f, 1.0f, 1.0f};
    const auto second = asciixel::convertFrame(image, charset, {2});
    require(second.pixels.size() == 2 && second.pixels[0].character == '#' &&
                second.pixels[1].character == '#',
            "successive conversions must use the current image with the shared charset");
    require(first.pixels[0].character == ' ', "previous frame must remain unchanged");
}
} // namespace

int main()
{
    try {
        convertsSuccessiveFramesWithSharedCharset();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
