#ifndef GLYPH_MATCHER_HPP
#define GLYPH_MATCHER_HPP

#include "asciixel/model/rasterized_charset.hpp"
#include "asciixel/model/ascii_frame.hpp"
#include "asciixel/model/sampled_frame.hpp"

namespace asciixel {

class GlyphMatcher {
private:
    static char matchCharacter(const LinearColor& color, const RasterizedCharset& charset,
                               double minimum_density, double density_range);

public:
    // The input frame contains sampled linear RGB colors.
    static AsciiFrame match(const SampledFrame& frame, const RasterizedCharset& charset);
};

} // namespace asciixel

#endif // GLYPH_MATCHER_HPP
