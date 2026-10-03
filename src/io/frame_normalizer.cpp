#include "asciixel/io/frame_normalizer.hpp"
#include "asciixel/core/color_conversion.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace asciixel {
namespace {

/**
 * @brief Replace deprecated JPEG pixel format tags without changing pixels.
 *
 * @param format Decoder-provided pixel format.
 *
 * @return Equivalent YUV tag, or the original tag if no replacement is needed.
 */
AVPixelFormat normalizeJpegFormat(AVPixelFormat format)
{
    switch (format) {
    case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
    case AV_PIX_FMT_YUVJ411P: return AV_PIX_FMT_YUV411P;
    case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
    case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
    case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
    default: return format;
    }
}

} // namespace

/**
 * @brief Convert decoded pixels to sRGB8 and composite transparency in linear light.
 *
 * @param source Decoded frame with valid dimensions, planes and strides.
 * @param background sRGB8 background used for alpha compositing.
 *
 * @return Owned sRGB8 image with the source dimensions.
 */
ImageFrame normalizeFrame(const AVFrame& source, Color background)
{
    // Validate the pixel format and retain JPEG's full-range interpretation.
    const auto original_format = static_cast<AVPixelFormat>(source.format);
    if (!av_pix_fmt_desc_get(original_format)) {
        throw std::runtime_error("Cannot convert image pixels");
    }

    const AVPixelFormat input_format = normalizeJpegFormat(original_format);
    const bool full_range = input_format != original_format ||
                            source.color_range == AVCOL_RANGE_JPEG;

    // Convert the source format to packed RGBA without resizing.
    std::unique_ptr<SwsContext, decltype(&sws_freeContext)> scaler(
        sws_getContext(source.width, source.height, input_format,
                       source.width, source.height, AV_PIX_FMT_RGBA,
                       SWS_BILINEAR, nullptr, nullptr, nullptr),
        sws_freeContext);
    if (!scaler) {
        throw std::runtime_error("Cannot convert image pixels");
    }

    // Override the source range for JPEG while preserving other scaler settings.
    if (full_range) {
        int *source_table = nullptr, *destination_table = nullptr;
        int source_range = 0, destination_range = 0;
        int brightness = 0, contrast = 0, saturation = 0;
        if (sws_getColorspaceDetails(scaler.get(), &source_table, &source_range,
                                     &destination_table, &destination_range,
                                     &brightness, &contrast, &saturation) < 0 ||
            sws_setColorspaceDetails(scaler.get(), source_table, 1,
                                     destination_table, destination_range,
                                     brightness, contrast, saturation) < 0) {
            throw std::runtime_error("Cannot set full-range image colorspace");
        }
    }

    // Allocate packed storage and convert all source rows.
    const std::size_t width = static_cast<std::size_t>(source.width);
    const std::size_t height = static_cast<std::size_t>(source.height);
    std::vector<std::uint8_t> rgba(width * height * 4);
    std::uint8_t* output[] = {rgba.data(), nullptr, nullptr, nullptr};
    int stride[] = {source.width * 4, 0, 0, 0};
    if (sws_scale(scaler.get(), source.data, source.linesize, 0, source.height,
                  output, stride) != source.height) {
        throw std::runtime_error("Cannot convert image pixels");
    }

    // Opaque video pixels remain bytes; only transparent pixels need arithmetic.
    ImageFrame image(width, height);
    const double background_r = srgbToLinear(background.r);
    const double background_g = srgbToLinear(background.g);
    const double background_b = srgbToLinear(background.b);
    for (std::size_t i = 0; i < image.pixels.size(); ++i) {
        const auto* pixel = rgba.data() + i * 4;
        if (pixel[3] == 255) {
            image.pixels[i].color = {pixel[0], pixel[1], pixel[2]};
        }
        else if (pixel[3] == 0) {
            image.pixels[i].color = background;
        }
        else {
            const double alpha = pixel[3] / 255.0;
            image.pixels[i].color = {
                linearToSrgb(alpha * srgbToLinear(pixel[0]) + (1.0 - alpha) * background_r),
                linearToSrgb(alpha * srgbToLinear(pixel[1]) + (1.0 - alpha) * background_g),
                linearToSrgb(alpha * srgbToLinear(pixel[2]) + (1.0 - alpha) * background_b),
            };
        }
    }

    return image;
}

} // namespace asciixel
