#include "asciixel/io/frame_normalizer.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace asciixel {
namespace {

float toLinear(std::uint8_t channel)
{
    const float value = channel / 255.0f;
    return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
}

// Replace deprecated YUVJ format tags with their equivalent YUV formats.
// Pixel data is unchanged; the JPEG full range is configured separately below.
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

ImageFrame normalizeFrame(const AVFrame& source, Color background)
{
    const auto original_format = static_cast<AVPixelFormat>(source.format);
    if (!av_pix_fmt_desc_get(original_format)) {
        throw std::runtime_error("Cannot convert image pixels");
    }

    const AVPixelFormat input_format = normalizeJpegFormat(original_format);
    const bool full_range = input_format != original_format || source.color_range == AVCOL_RANGE_JPEG;

    std::unique_ptr<SwsContext, decltype(&sws_freeContext)> scaler(
        sws_getContext(source.width, source.height, input_format,
                       source.width, source.height, AV_PIX_FMT_RGBA,
                       SWS_BILINEAR, nullptr, nullptr, nullptr),
        sws_freeContext);
    if (!scaler) {
        throw std::runtime_error("Cannot convert image pixels");
    }
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

    const std::size_t width = static_cast<std::size_t>(source.width);
    const std::size_t height = static_cast<std::size_t>(source.height);
    std::vector<std::uint8_t> rgba(width * height * 4);
    std::uint8_t* output[] = {rgba.data(), nullptr, nullptr, nullptr};
    int stride[] = {source.width * 4, 0, 0, 0};
    if (sws_scale(scaler.get(), source.data, source.linesize, 0, source.height,
                  output, stride) != source.height) {
        throw std::runtime_error("Cannot convert image pixels");
    }

    ImageFrame image(width, height);
    for (std::size_t i = 0; i < image.pixels.size(); ++i) {
        const auto* pixel = rgba.data() + i * 4;
        const float alpha = pixel[3] / 255.0f;
        image.pixels[i].color = {
            alpha * toLinear(pixel[0]) + (1.0f - alpha) * background.r,
            alpha * toLinear(pixel[1]) + (1.0f - alpha) * background.g,
            alpha * toLinear(pixel[2]) + (1.0f - alpha) * background.b,
        };
    }
    return image;
}

} // namespace asciixel
