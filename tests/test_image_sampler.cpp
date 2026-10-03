#include "asciixel/core/image_sampler.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using asciixel::Color;
using asciixel::ImageFrame;
using asciixel::ImagePixel;
using asciixel::ImageSampler;
using asciixel::LinearColor;
using asciixel::SampledFrame;

bool nearlyEqual(float left, float right)
{
    return std::fabs(left - right) < 0.0001f;
}

void require(bool condition)
{
    if (!condition) {
        throw std::runtime_error("unit test assertion failed");
    }
}

void assertColor(const LinearColor& color, float r, float g, float b)
{
    require(nearlyEqual(color.r, r));
    require(nearlyEqual(color.g, g));
    require(nearlyEqual(color.b, b));
}

void fillFrame(ImageFrame& frame, const Color& color)
{
    for (auto& pixel : frame.pixels) {
        pixel.color = color;
    }
}

void sampleConstantFrame()
{
    ImageFrame frame(4, 2);
    fillFrame(frame, {51, 102, 204});

    const SampledFrame sampled = ImageSampler::sample(frame, 2, 1);

    require(sampled.width == 2);
    require(sampled.height == 1);
    assertColor(sampled.at(0, 0).color, 0.0331048f, 0.132868f, 0.603827f);
    assertColor(sampled.at(1, 0).color, 0.0331048f, 0.132868f, 0.603827f);
}

void preservePixelsWhenDimensionsMatch()
{
    ImageFrame frame(2, 2);
    frame.at(0, 0) = ImagePixel{{255, 0, 0}};
    frame.at(1, 0) = ImagePixel{{0, 255, 0}};
    frame.at(0, 1) = ImagePixel{{0, 0, 255}};
    frame.at(1, 1) = ImagePixel{{255, 255, 255}};

    const SampledFrame sampled = ImageSampler::sample(frame, 2, 2);

    assertColor(sampled.at(0, 0).color, 1.0f, 0.0f, 0.0f);
    assertColor(sampled.at(1, 0).color, 0.0f, 1.0f, 0.0f);
    assertColor(sampled.at(0, 1).color, 0.0f, 0.0f, 1.0f);
    assertColor(sampled.at(1, 1).color, 1.0f, 1.0f, 1.0f);
}

void weightFractionalBoundaryWhenUpsampling()
{
    ImageFrame frame(2, 1);
    frame.at(0, 0) = ImagePixel{{255, 0, 0}};
    frame.at(1, 0) = ImagePixel{{0, 0, 255}};

    const SampledFrame sampled = ImageSampler::sample(frame, 3, 1);

    assertColor(sampled.at(0, 0).color, 1.0f, 0.0f, 0.0f);
    assertColor(sampled.at(1, 0).color, 0.5f, 0.0f, 0.5f);
    assertColor(sampled.at(2, 0).color, 0.0f, 0.0f, 1.0f);
}

void weightFractionalBoundaryWhenDownsampling()
{
    ImageFrame frame(3, 1);
    frame.at(0, 0) = ImagePixel{{0, 0, 0}};
    frame.at(1, 0) = ImagePixel{{255, 0, 0}};
    frame.at(2, 0) = ImagePixel{{0, 0, 0}};

    const SampledFrame sampled = ImageSampler::sample(frame, 2, 1);

    assertColor(sampled.at(0, 0).color, 1.0f / 3.0f, 0.0f, 0.0f);
    assertColor(sampled.at(1, 0).color, 1.0f / 3.0f, 0.0f, 0.0f);
}

void repeatSinglePixelWhenUpsampling()
{
    ImageFrame frame(1, 1);
    frame.at(0, 0) = ImagePixel{{51, 102, 204}};

    const SampledFrame sampled = ImageSampler::sample(frame, 3, 2);

    require(sampled.width == 3);
    require(sampled.height == 2);

    for (std::size_t y = 0; y < sampled.height; ++y) {
        for (std::size_t x = 0; x < sampled.width; ++x) {
            assertColor(sampled.at(x, y).color, 0.0331048f, 0.132868f, 0.603827f);
        }
    }
}

void weightBothAxesForTinyImage()
{
    ImageFrame frame(3, 3);
    fillFrame(frame, {0, 0, 0});
    frame.at(1, 1) = ImagePixel{{255, 0, 0}};

    const SampledFrame sampled = ImageSampler::sample(frame, 2, 2);

    for (std::size_t y = 0; y < sampled.height; ++y) {
        for (std::size_t x = 0; x < sampled.width; ++x) {
            assertColor(sampled.at(x, y).color, 1.0f / 9.0f, 0.0f, 0.0f);
        }
    }
}

void honorRequestedRowsWithoutCroppingSource()
{
    ImageFrame frame(2, 4);
    for (std::size_t y = 0; y < 4; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
            frame.at(x, y) = ImagePixel{{static_cast<std::uint8_t>(y < 2 ? 255 : 0),
                                         0,
                                         static_cast<std::uint8_t>(y < 2 ? 0 : 255)}};
        }
    }

    const SampledFrame sampled = ImageSampler::sample(frame, 2, 2);

    require(sampled.width == 2);
    require(sampled.height == 2);
    assertColor(sampled.at(0, 0).color, 1.0f, 0.0f, 0.0f);
    assertColor(sampled.at(1, 0).color, 1.0f, 0.0f, 0.0f);
    assertColor(sampled.at(0, 1).color, 0.0f, 0.0f, 1.0f);
    assertColor(sampled.at(1, 1).color, 0.0f, 0.0f, 1.0f);
}

void honorOddRequestedRows()
{
    ImageFrame frame(1, 2);
    frame.at(0, 0) = ImagePixel{{255, 0, 0}};
    frame.at(0, 1) = ImagePixel{{0, 0, 255}};

    const SampledFrame sampled = ImageSampler::sample(frame, 1, 3);

    require(sampled.height == 3);
    assertColor(sampled.at(0, 0).color, 1.0f, 0.0f, 0.0f);
    assertColor(sampled.at(0, 1).color, 0.5f, 0.0f, 0.5f);
    assertColor(sampled.at(0, 2).color, 0.0f, 0.0f, 1.0f);
}

void rejectZeroDimensions()
{
    const ImageFrame frame(2, 2);

    bool threw = false;
    try {
        ImageSampler::sample(frame, 0, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    require(threw);
}

void rejectEmptySource()
{
    const ImageFrame frame(0, 1);
    bool threw = false;
    try {
        ImageSampler::sample(frame, 1, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw);
}

void rejectOutputSizeOverflow()
{
    const ImageFrame frame(1, 1);
    bool threw = false;
    try {
        ImageSampler::sample(frame, std::numeric_limits<std::size_t>::max(), 2);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw);
}

} // namespace

int main()
{
    sampleConstantFrame();
    preservePixelsWhenDimensionsMatch();
    weightFractionalBoundaryWhenUpsampling();
    weightFractionalBoundaryWhenDownsampling();
    repeatSinglePixelWhenUpsampling();
    weightBothAxesForTinyImage();
    honorRequestedRowsWithoutCroppingSource();
    honorOddRequestedRows();
    rejectZeroDimensions();
    rejectEmptySource();
    rejectOutputSizeOverflow();
}
