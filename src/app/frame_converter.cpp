#include "asciixel/app/frame_converter.hpp"
#include "asciixel/core/glyph_matcher.hpp"
#include "asciixel/core/grid_layout.hpp"
#include "asciixel/core/image_sampler.hpp"

namespace asciixel {

/**
 * @brief Convert one linear RGB image into a character frame synchronously.
 *
 * @param image Source image with valid dimensions and pixel storage.
 * @param charset Caller-prepared glyphs and shared cell layout.
 * @param config Requested character column count.
 *
 * @return Character frame with an aspect-corrected grid.
 */
AsciiFrame convertFrame(const ImageFrame& image, const RasterizedCharset& charset,
                        const SampleConfig& config)
{
    // Derive the grid from source dimensions and font cell proportions.
    const auto grid = calculateGrid(image.width, image.height,
                                    config, charset.layout);

    // Average source colors over each grid cell.
    const auto sampled = ImageSampler::sample(image, grid.columns, grid.rows);

    // Match sampled luminance against the prepared glyph densities.
    return GlyphMatcher::match(sampled, charset);
}

} // namespace asciixel
