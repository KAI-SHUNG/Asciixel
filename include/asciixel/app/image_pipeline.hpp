#ifndef IMAGE_PIPELINE_HPP
#define IMAGE_PIPELINE_HPP

#include "asciixel/app/frame_converter.hpp"
#include "asciixel/config/asciixel_config.hpp"
#include "asciixel/core/ascii_renderer.hpp"
#include "asciixel/core/charset_builder.hpp"
#include "asciixel/io/image_loader.hpp"
#include "asciixel/io/png_writer.hpp"
#include "asciixel/io/text_writer.hpp"
#include "asciixel/model/image_frame.hpp"
#include "asciixel/model/rasterized_charset.hpp"
#include <exception>
#include <stdexcept>

namespace asciixel {

/**
 * @brief Run one image conversion and dispatch its selected output.
 *
 * @param config Resolved image configuration, including output destination.
 *
 * @return No value; configuration and conversion failures propagate.
 */
void convertImage(const asciixel::Config& config)
{
    // Reject unsupported media before loading any conversion resources.
    asciixel::validateConfig(config);
    const auto* mediaConfig = std::get_if<asciixel::ImageConfig>(&config.media_config);
    if (!mediaConfig) {
        throw std::invalid_argument("Image conversion requires an image configuration");
    }

    // Prepare image pixels and font data for the synchronous frame conversion.
    const auto image   = asciixel::loadImage(config.input_path);
    const auto charset = asciixel::CharsetBuilder::buildCharset(config.charset);
    const auto frame   = asciixel::convertFrame(image, charset, config.sampling);

    // Render glyph bitmaps only when the selected output requires pixels.
    switch (mediaConfig->output) {
    case asciixel::ImageOutput::Terminal:
        asciixel::writeTextToStdout(frame);
        break;
    case asciixel::ImageOutput::Png:
        asciixel::writePng(asciixel::renderAscii(frame, charset),
                           *mediaConfig->output_path);
        break;
    case asciixel::ImageOutput::Txt:
        asciixel::writeTextFile(frame, *mediaConfig->output_path);
        break;
    }
}

} // namespace asciixel

#endif // IMAGE_PIPELINE_HPP