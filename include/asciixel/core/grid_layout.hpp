#ifndef GRID_LAYOUT_HPP
#define GRID_LAYOUT_HPP

#include "asciixel/config/asciixel_config.hpp"
#include "asciixel/model/rasterized_charset.hpp"

namespace asciixel {

struct GridSize {
    std::size_t columns;
    std::size_t rows;
};

// Small images retain their source column count. Rows use the actual cell aspect.
GridSize calculateGrid(std::size_t image_width, std::size_t image_height,
                       const SampleConfig& config, const GlyphLayout& layout);

} // namespace asciixel
#endif
