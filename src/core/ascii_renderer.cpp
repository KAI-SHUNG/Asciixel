#include "asciixel/core/ascii_renderer.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace asciixel {

/**
 * @brief Render white glyph coverage onto an opaque black bitmap.
 *
 * @param frame Nonempty character frame with consistent pixel storage.
 * @param charset Common cell layout and glyph bitmaps for all frame characters.
 *
 * @return Tightly packed grayscale bitmap within the output pixel limit.
 */
GrayBitmap renderAscii(const AsciiFrame& frame,
                       const RasterizedCharset& charset)
{
    // Validate dimensions before multiplying them into output sizes.
    const auto& layout = charset.layout;
    if (!frame.width || !frame.height || !layout.cell_width ||
        !layout.cell_height ||
        frame.width > maxBitmapPixels / layout.cell_width ||
        frame.height > maxBitmapPixels / layout.cell_height) {
        throw std::invalid_argument(
            "Invalid or excessive ASCII bitmap dimensions");
    }

    // Check the output budget and the input character count.
    const auto width  = frame.width * layout.cell_width;
    const auto height = frame.height * layout.cell_height;
    if (width > maxBitmapPixels / height ||
        frame.pixels.size() != frame.width * frame.height) {
        throw std::invalid_argument("Invalid or excessive ASCII frame size");
    }

    // Index glyphs by character and validate their owned coverage buffers.
    std::array<const RasterizedGlyph*, 256> lookup{};
    for (const auto& glyph : charset.glyphs) {
        auto& entry = lookup[static_cast<unsigned char>(glyph.character)];
        if (entry) {
            throw std::invalid_argument("Duplicate rasterized character");
        }
        entry = &glyph;

        if (glyph.bitmap_height &&
            glyph.bitmap_width > std::numeric_limits<std::size_t>::max() / glyph.bitmap_height) {
            throw std::invalid_argument("Glyph dimensions overflow");
        }
        if (glyph.alpha.size() != glyph.bitmap_width * glyph.bitmap_height) {
            throw std::invalid_argument("Invalid glyph coverage buffer");
        }
        if (glyph.alpha.empty()) {
            continue;
        }

        // Convert baseline-relative bearings to cell-local bounds.
        const auto x =
            static_cast<std::int64_t>(layout.baseline_x) + glyph.bitmap_left;
        const auto y =
            static_cast<std::int64_t>(layout.baseline_y) - glyph.bitmap_top;
        if (x < 0 || y < 0 ||
            static_cast<std::size_t>(x) > layout.cell_width ||
            static_cast<std::size_t>(y) > layout.cell_height ||
            glyph.bitmap_width > layout.cell_width - static_cast<std::size_t>(x) ||
            glyph.bitmap_height > layout.cell_height - static_cast<std::size_t>(y)) {
            throw std::invalid_argument("Glyph lies outside its cell");
        }
    }

    // Start with black cells; empty glyphs leave the background untouched.
    GrayBitmap result{
        width, height, std::vector<std::uint8_t>(width * height, 0)};
    for (std::size_t row = 0; row < frame.height; ++row) {
        for (std::size_t col = 0; col < frame.width; ++col) {
            const auto* glyph = lookup[static_cast<unsigned char>(
                frame.pixels[row * frame.width + col].character)];
            if (!glyph) {
                throw std::invalid_argument(
                    "Character missing from rasterized charset");
            }
            if (glyph->alpha.empty()) {
                continue;
            }

            // Position the glyph within its output cell using the baseline.
            const auto x = col * layout.cell_width +
                static_cast<std::size_t>(
                    static_cast<std::int64_t>(layout.baseline_x) +
                    glyph->bitmap_left);
            const auto y = row * layout.cell_height +
                static_cast<std::size_t>(
                    static_cast<std::int64_t>(layout.baseline_y) -
                    glyph->bitmap_top);

            // Copy coverage rows directly to preserve font antialiasing.
            for (std::size_t line = 0; line < glyph->bitmap_height; ++line) {
                std::copy_n(glyph->alpha.data() + line * glyph->bitmap_width,
                            glyph->bitmap_width,
                            result.pixels.data() + (y + line) * width + x);
            }
        }
    }

    return result;
}

} // namespace asciixel
