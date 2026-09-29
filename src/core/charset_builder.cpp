#include "asciixel/core/charset_builder.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace asciixel {
namespace {

using LibraryHandle =
    std::unique_ptr<FT_LibraryRec_, decltype(&FT_Done_FreeType)>;
using FaceHandle = std::unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)>;

struct Font {
    // Destroy the face before its owning FreeType library.
    LibraryHandle library;
    FaceHandle face;
};

/**
 * @brief Open the first font face and set its rasterization size.
 *
 * @param font_path Font file path accepted by FreeType.
 * @param pixel_size Requested glyph height in pixels.
 *
 * @return Owned library and face handles with the pixel size configured.
 */
Font loadFont(const std::string& font_path, unsigned pixel_size)
{
    // Initialize FreeType under RAII ownership.
    FT_Library raw_library = nullptr;
    if (FT_Init_FreeType(&raw_library)) {
        throw std::runtime_error("FreeType initialization failed");
    }
    LibraryHandle library(raw_library, FT_Done_FreeType);

    // Open face zero while keeping its library alive.
    FT_Face raw_face = nullptr;
    if (FT_New_Face(library.get(), font_path.c_str(), 0, &raw_face)) {
        throw std::runtime_error("Cannot load font: " + font_path);
    }
    FaceHandle face(raw_face, FT_Done_Face);

    // Configure the pixel size used for metrics and glyph rasterization.
    if (FT_Set_Pixel_Sizes(face.get(), 0, pixel_size)) {
        throw std::runtime_error("Cannot set font size");
    }

    return {std::move(library), std::move(face)};
}

/**
 * @brief Rasterize one character and copy its grayscale coverage.
 *
 * @param face Non-null face with a configured pixel size.
 *             Its glyph slot is updated during rasterization.
 * @param ch Printable ASCII character present in the font.
 *
 * @return Owned glyph bitmap and bearings; density is calculated later.
 */
RasterizedGlyph copyGlyph(FT_Face face, unsigned char ch)
{
    // Reject missing characters before rasterizing the glyph.
    if (!FT_Get_Char_Index(face, ch) ||
        FT_Load_Char(face, ch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) {
        throw std::runtime_error(
            "Cannot render ASCII character: " + std::to_string(ch));
    }

    // Preserve bitmap dimensions and baseline-relative positioning.
    const FT_Bitmap& bitmap = face->glyph->bitmap;
    RasterizedGlyph glyph;
    glyph.character = static_cast<char>(ch);
    glyph.bitmap_width = bitmap.width;
    glyph.bitmap_height = bitmap.rows;
    glyph.bitmap_left = face->glyph->bitmap_left;
    glyph.bitmap_top = face->glyph->bitmap_top;
    if (bitmap.width == 0 || bitmap.rows == 0) {
        return glyph;
    }

    // Validate grayscale storage before allocating the owned coverage buffer.
    if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.num_grays < 2) {
        throw std::runtime_error("Expected grayscale glyph bitmap");
    }
    if (glyph.bitmap_width > std::numeric_limits<std::size_t>::max() / glyph.bitmap_height) {
        throw std::runtime_error("Glyph bitmap dimensions overflow");
    }

    // Follow FreeType's row pitch and normalize coverage to eight bits.
    glyph.alpha.resize(glyph.bitmap_width * glyph.bitmap_height);
    for (std::size_t y = 0; y < glyph.bitmap_height; ++y) {
        const auto* row =
            bitmap.buffer + static_cast<std::ptrdiff_t>(y) * bitmap.pitch;
        for (std::size_t x = 0; x < glyph.bitmap_width; ++x) {
            glyph.alpha[y * glyph.bitmap_width + x] =
                static_cast<std::uint8_t>(
                    (row[x] * 255u + (bitmap.num_grays - 1) / 2) /
                    (bitmap.num_grays - 1));
        }
    }

    return glyph;
}

} // namespace

/**
 * @brief Build glyph bitmaps, a shared cell layout and sorted coverage values.
 *
 * @param config Font path, pixel size and unique printable ASCII candidates.
 *
 * @return Owned charset sorted by density, then by character code.
 */
RasterizedCharset CharsetBuilder::buildCharset(const CharsetConfig& config)
{
    // Validate the request and prepare the font face once for all candidates.
    config.validate();
    Font font = loadFont(config.font_path, config.pixel_size);
    FT_Face face = font.face.get();
    RasterizedCharset charset;

    // Seed common cell bounds with the font's vertical metrics.
    std::int64_t left = 0;
    std::int64_t right = 0;
    std::int64_t top = std::max<std::int64_t>(0,
        std::ceil(face->size->metrics.ascender / 64.0));
    std::int64_t bottom = std::min<std::int64_t>(0,
        std::floor(face->size->metrics.descender / 64.0));
    const auto line_height = static_cast<std::int64_t>(
        std::ceil(face->size->metrics.height / 64.0));
    FT_Pos advance = 0;

    // Rasterize each candidate once and expand the common cell to fit.
    for (unsigned char ch : config.candidates) {
        auto glyph = copyGlyph(face, ch);

        // Require a shared positive advance for a consistent character grid.
        const FT_Pos glyph_advance = face->glyph->advance.x;
        if (charset.glyphs.empty()) {
            advance = glyph_advance;
        }
        if (advance <= 0 || glyph_advance != advance) {
            throw std::runtime_error(
                "Candidate characters must have the same positive advance");
        }

        // Include both advance width and visible overhangs in the cell bounds.
        right = std::max(right,
            static_cast<std::int64_t>(std::ceil(advance / 64.0)));
        if (!glyph.alpha.empty()) {
            left = std::min(left, static_cast<std::int64_t>(glyph.bitmap_left));
            right = std::max(right,
                static_cast<std::int64_t>(glyph.bitmap_left) + static_cast<std::int64_t>(glyph.bitmap_width));
            top = std::max(top, static_cast<std::int64_t>(glyph.bitmap_top));
            bottom = std::min(bottom,
                static_cast<std::int64_t>(glyph.bitmap_top) - static_cast<std::int64_t>(glyph.bitmap_height));
        }
        charset.glyphs.push_back(std::move(glyph));
    }

    // Finalize positive cell dimensions and a baseline shared by all glyphs.
    const auto width = right - left;
    const auto height = std::max(line_height, top - bottom);
    if (width <= 0 || height <= 0 ||
        width > std::numeric_limits<int>::max() ||
        height > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Invalid font cell size");
    }
    charset.layout = {
        static_cast<std::size_t>(width), static_cast<std::size_t>(height),
        static_cast<int>(-left), static_cast<int>(top)};

    // Measure coverage against the full common cell, including empty margins.
    const double area = static_cast<double>(width) * height;
    for (auto& glyph : charset.glyphs) {
        double sum = 0;
        for (auto alpha : glyph.alpha) {
            sum += alpha;
        }
        glyph.density = static_cast<float>(sum / (255.0 * area));
    }

    // Sort deterministically, using character codes to resolve equal coverage.
    std::sort(charset.glyphs.begin(), charset.glyphs.end(),
              [](const RasterizedGlyph& a, const RasterizedGlyph& b) {
                  return a.density == b.density ? a.character < b.character
                                                : a.density < b.density;
              });

    return charset;
}

} // namespace asciixel
