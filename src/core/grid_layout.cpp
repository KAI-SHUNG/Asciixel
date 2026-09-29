#include "asciixel/core/grid_layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace asciixel {

/**
 * @brief Calculate an aspect-corrected character grid without adding columns.
 *
 * @param image_width Source image width in pixels; must be positive.
 * @param image_height Source image height in pixels; must be positive.
 * @param config Requested column count; must be positive.
 * @param layout Font cell dimensions used for aspect correction.
 *
 * @return Grid dimensions with at least one row.
 */
GridSize calculateGrid(std::size_t image_width, std::size_t image_height,
                       const SampleConfig& config, const GlyphLayout& layout)
{
    // Reject empty image, grid or font cell dimensions.
    if (image_width == 0 || image_height == 0 || config.columns == 0 ||
        layout.cell_width == 0 || layout.cell_height == 0) {
        throw std::invalid_argument(
            "image, sampling and cell dimensions must be greater than zero");
    }

    // Limit columns to the source width and correct rows for the cell aspect.
    const std::size_t columns = std::min(config.columns, image_width);
    const long double rows_value = std::max(1.0L,
        std::round(static_cast<long double>(columns) / image_width * image_height * layout.cell_width / layout.cell_height));

    // Check rows before conversion; a power-of-two bound stays exact even
    // when long double has the same precision as double.
    const long double size_limit =
        std::ldexp(1.0L, std::numeric_limits<std::size_t>::digits);
    if (!std::isfinite(rows_value) || rows_value >= size_limit) {
        throw std::invalid_argument("grid row count overflows");
    }

    // Ensure the total cell count fits in size_t.
    const auto rows = static_cast<std::size_t>(rows_value);
    if (columns > std::numeric_limits<std::size_t>::max() / rows) {
        throw std::invalid_argument("grid cell count overflows");
    }

    return {columns, rows};
}

} // namespace asciixel
