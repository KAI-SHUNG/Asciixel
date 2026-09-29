#include "asciixel/app/frame_converter.hpp"
#include "asciixel/core/glyph_matcher.hpp"
#include "asciixel/core/grid_layout.hpp"
#include "asciixel/core/image_sampler.hpp"

namespace asciixel {

AsciiFrame convertFrame(const ImageFrame& image, const RasterizedCharset& charset,
                        const SampleConfig& sampling)
{
    const auto grid = calculateGrid(image.width, image.height, sampling, charset.layout);
    const auto sampled = ImageSampler::sample(image, grid.columns, grid.rows);
    return GlyphMatcher::match(sampled, charset);
}

} // namespace asciixel
