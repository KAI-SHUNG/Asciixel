#include "asciixel/core/image_sampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace asciixel {

/**
 * @brief Average linear RGB over a block using pixel overlap areas.
 *
 * @param frame Valid source image in linear RGB.
 * @param x_start Inclusive left edge in source pixel coordinates.
 * @param y_start Inclusive top edge in source pixel coordinates.
 * @param x_end Exclusive right edge in source pixel coordinates.
 * @param y_end Exclusive bottom edge in source pixel coordinates.
 *
 * @return Area-weighted color of a positive-area block inside the image.
 */
SampledPixel ImageSampler::sampleBlock(const ImageFrame& frame,
                                     double            x_start,
                                     double            y_start,
                                     double            x_end,
                                     double            y_end)
{
    // Accumulate weighted channels and the covered source area.
    double red        = 0.0;
    double green      = 0.0;
    double blue       = 0.0;
    double total_area = 0.0;

    // Clip the final pixel indices to the source bounds.
    const std::size_t last_y =
        std::min(frame.height, static_cast<std::size_t>(std::ceil(y_end)));
    const std::size_t last_x =
        std::min(frame.width, static_cast<std::size_t>(std::ceil(x_end)));

    // Weight each intersecting pixel by its overlap with the sample block.
    for (std::size_t y = static_cast<std::size_t>(std::floor(y_start));
         y < last_y; ++y) {
        const double overlap_y =
            std::min(y_end, static_cast<double>(y + 1)) -
            std::max(y_start, static_cast<double>(y));
        for (std::size_t x = static_cast<std::size_t>(std::floor(x_start));
             x < last_x; ++x) {
            const double overlap_x =
                std::min(x_end, static_cast<double>(x + 1)) -
                std::max(x_start, static_cast<double>(x));
            const double area      = overlap_x * overlap_y;
            if (area <= 0.0) {
                continue;
            }

            const Color color = frame.at(x, y).color;
            red += static_cast<double>(color.r) * area;
            green += static_cast<double>(color.g) * area;
            blue += static_cast<double>(color.b) * area;
            total_area += area;
        }
    }

    // Normalize the accumulated channels by the actual covered area.
    return SampledPixel{{static_cast<float>(red / total_area),
                         static_cast<float>(green / total_area),
                         static_cast<float>(blue / total_area)}};
}

/**
 * @brief Resample an image into a caller-selected character grid.
 *
 * @param frame Source image with dimensions matching its pixel buffer.
 * @param new_width Positive number of sample columns.
 * @param new_height Positive number of sample rows.
 *
 * @return Grid of area-weighted linear RGB colors.
 */
SampledFrame ImageSampler::sample(const ImageFrame& frame,
                                 std::size_t new_width,
                                 std::size_t new_height)
{
    // Validate source storage and destination dimensions before allocation.
    if (frame.width == 0 || frame.height == 0 ||
        new_width == 0 || new_height == 0) {
        throw std::invalid_argument(
            "sample dimensions must be greater than zero");
    }
    if (frame.width > std::numeric_limits<std::size_t>::max() / frame.height ||
        frame.pixels.size() != frame.width * frame.height ||
        new_width > std::numeric_limits<std::size_t>::max() / new_height) {
        throw std::invalid_argument("invalid sample dimensions or pixel count");
    }

    // Allocate the grid and map each cell to a continuous source rectangle.
    SampledFrame sampled_frame(new_width, new_height);
    const double x_step =
        static_cast<double>(frame.width) / static_cast<double>(new_width);
    const double y_step =
        static_cast<double>(frame.height) / static_cast<double>(new_height);

    // Sample each cell independently, including fractional pixel boundaries.
    for (std::size_t y = 0; y < new_height; ++y) {
        for (std::size_t x = 0; x < new_width; ++x) {
            const double x_start   = static_cast<double>(x) * x_step;
            const double y_start   = static_cast<double>(y) * y_step;
            const double x_end     = static_cast<double>(x + 1) * x_step;
            const double y_end     = static_cast<double>(y + 1) * y_step;
            sampled_frame.at(x, y) = sampleBlock(frame, x_start, y_start,
                                                x_end, y_end);
        }
    }

    return sampled_frame;
}

} // namespace asciixel
