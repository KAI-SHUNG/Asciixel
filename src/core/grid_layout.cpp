#include "asciixel/core/grid_layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace asciixel {

GridSize calculateGrid(std::size_t image_width, std::size_t image_height,
                       const SampleConfig& config, const GlyphLayout& layout)
{
    if (image_width == 0 || image_height == 0 || config.columns == 0 ||
        layout.cell_width == 0 || layout.cell_height == 0) {
        throw std::invalid_argument("image, sampling and cell dimensions must be greater than zero");
    }
    const std::size_t columns = std::min(config.columns, image_width);
    const long double rows_value = std::max(1.0L, std::round(
        static_cast<long double>(columns) / image_width * image_height *
        layout.cell_width / layout.cell_height));
    // An exclusive power-of-two bound remains exact even when long double == double.
    const long double size_limit = std::ldexp(1.0L, std::numeric_limits<std::size_t>::digits);
    if (!std::isfinite(rows_value) || rows_value >= size_limit) {
        throw std::invalid_argument("grid row count overflows");
    }
    const auto rows = static_cast<std::size_t>(rows_value);
    if (columns > std::numeric_limits<std::size_t>::max() / rows) {
        throw std::invalid_argument("grid cell count overflows");
    }
    return {columns, rows};
}

} // namespace asciixel
