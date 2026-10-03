#include "asciixel/core/glyph_matcher.hpp"
#include "asciixel/core/color_conversion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace asciixel {

/**
 * @brief Select the glyph whose normalized density is nearest to luminance.
 *
 * @param color Finite linear RGB color with channels in [0, 1].
 * @param charset Nonempty charset with validated glyph densities.
 * @param minimum_density Lowest density in the charset.
 * @param density_range Difference between highest and lowest densities.
 *
 * @return Matching ASCII character; equal distances favor the smaller code.
 */
char GlyphMatcher::matchCharacter(const LinearColor& color,
                                 const RasterizedCharset& charset,
                                 double minimum_density, double density_range)
{
    // Reject colors outside the linear RGB contract.
    if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
        !std::isfinite(color.b) ||
        color.r < 0.0f || color.r > 1.0f || color.g < 0.0f || color.g > 1.0f ||
        color.b < 0.0f || color.b > 1.0f) {
        throw std::invalid_argument(
            "match color must be finite linear RGB in [0, 1]");
    }

    // Convert linear RGB to luminance and search the stretched density range.
    const double brightness =
        0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    const RasterizedGlyph* best = nullptr;
    double best_distance = std::numeric_limits<double>::infinity();
    for (const RasterizedGlyph& glyph : charset.glyphs) {
        const double level = density_range > 0 ?
            (glyph.density - minimum_density) / density_range : 0.0;
        const double distance = std::abs(brightness - level);

        // Break ties by character code so candidate order cannot affect output.
        if (distance < best_distance ||
            (distance == best_distance &&
             static_cast<unsigned char>(glyph.character) < static_cast<unsigned char>(best->character))) {
            best = &glyph;
            best_distance = distance;
        }
    }

    return best->character;
}

/**
 * @brief Map sampled colors to glyphs using normalized font coverage.
 *
 * @param frame Nonempty sampled frame containing linear RGB colors.
 * @param charset Nonempty glyph set with finite densities in [0, 1].
 *
 * @return Character frame with the sample dimensions and sRGB8 colors.
 */
AsciiFrame GlyphMatcher::match(const SampledFrame& frame,
                              const RasterizedCharset& charset)
{
    // Validate frame storage and ensure at least one candidate is available.
    if (frame.width == 0 || frame.height == 0 ||
        frame.width > std::numeric_limits<std::size_t>::max() / frame.height ||
        frame.pixels.size() != frame.width * frame.height) {
        throw std::invalid_argument(
            "invalid match frame dimensions or pixel count");
    }
    if (charset.glyphs.empty()) {
        throw std::invalid_argument("match charset must not be empty");
    }

    // Validate coverage and derive the normalization range once per frame.
    double minimum = 1.0;
    double maximum = 0.0;
    for (const RasterizedGlyph& glyph : charset.glyphs) {
        if (!std::isfinite(glyph.density) || glyph.density < 0.0f ||
            glyph.density > 1.0f) {
            throw std::invalid_argument(
                "match glyph density must be finite and in [0, 1]");
        }
        minimum = std::min(minimum, static_cast<double>(glyph.density));
        maximum = std::max(maximum, static_cast<double>(glyph.density));
    }

    // Match before quantizing to sRGB8 so rounding cannot change the glyph.
    AsciiFrame result(frame.width, frame.height);
    for (std::size_t y = 0; y < frame.height; ++y) {
        for (std::size_t x = 0; x < frame.width; ++x) {
            const LinearColor& color = frame.at(x, y).color;
            result.at(x, y) = {
                matchCharacter(color, charset, minimum, maximum - minimum),
                toSrgb(color)};
        }
    }

    return result;
}

} // namespace asciixel
