#include "asciixel/io/image_loader.hpp"
#include "asciixel/io/frame_normalizer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <memory>
#include <stdexcept>

namespace asciixel {
namespace {

/**
 * @brief Close an input context owned by the RAII handle.
 *
 * @param context Input context to release; may be null.
 *
 * @return No value.
 */
void closeInput(AVFormatContext* context)
{
    avformat_close_input(&context);
}

/**
 * @brief Release a decoder context owned by the RAII handle.
 *
 * @param context Decoder context to release; may be null.
 *
 * @return No value.
 */
void freeDecoder(AVCodecContext* context)
{
    avcodec_free_context(&context);
}

/**
 * @brief Release a packet and its referenced data.
 *
 * @param packet Packet to release; may be null.
 *
 * @return No value.
 */
void freePacket(AVPacket* packet)
{
    av_packet_free(&packet);
}

/**
 * @brief Release a decoded frame and its referenced buffers.
 *
 * @param frame Frame to release; may be null.
 *
 * @return No value.
 */
void freeFrame(AVFrame* frame)
{
    av_frame_free(&frame);
}

using Input   = std::unique_ptr<AVFormatContext, decltype(&closeInput)>;
using Decoder = std::unique_ptr<AVCodecContext, decltype(&freeDecoder)>;
using Packet  = std::unique_ptr<AVPacket, decltype(&freePacket)>;
using Frame   = std::unique_ptr<AVFrame, decltype(&freeFrame)>;

/**
 * @brief Open an image source and discover its streams.
 *
 * @param path Input path accepted by FFmpeg.
 *
 * @return Owned input context with stream information populated.
 */
Input openImage(const std::string& path)
{
    // Open the source before transferring ownership to the RAII handle.
    AVFormatContext* context = nullptr;
    if (avformat_open_input(&context, path.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Cannot open image: " + path);
    }
    Input input(context, closeInput);

    // Probe stream metadata needed to select and configure the decoder.
    if (avformat_find_stream_info(input.get(), nullptr) < 0) {
        throw std::runtime_error("Cannot read image: " + path);
    }

    return input;
}

/**
 * @brief Decode an image using one packet from an opened source.
 *
 * @param input Non-null input context with discovered streams.
 *
 * @return Owned decoded frame; unsupported or incomplete decoding throws.
 */
Frame decodeImage(AVFormatContext* input)
{
    // Select the image stream and locate its decoder.
    const int stream = av_find_best_stream(input, AVMEDIA_TYPE_VIDEO,
                                           -1, -1, nullptr, 0);
    if (stream < 0) {
        throw std::runtime_error("Image stream not found");
    }

    const AVCodecParameters* parameters = input->streams[stream]->codecpar;
    const AVCodec*           codec      = avcodec_find_decoder(parameters->codec_id);
    if (!codec) {
        throw std::runtime_error("Image decoder not found");
    }

    // Configure decoder ownership and copy the selected stream parameters.
    Decoder decoder(avcodec_alloc_context3(codec), freeDecoder);
    if (!decoder ||
        avcodec_parameters_to_context(decoder.get(), parameters) < 0 ||
        avcodec_open2(decoder.get(), codec, nullptr) < 0) {
        throw std::runtime_error("Cannot open image decoder");
    }

    // Allocate packet and frame handles before requesting decoded pixels.
    Packet packet(av_packet_alloc(), freePacket);
    Frame  frame(av_frame_alloc(), freeFrame);
    if (!packet || !frame) {
        throw std::runtime_error("Cannot allocate image buffer");
    }

    // The static-image path expects one packet to produce one frame.
    if (av_read_frame(input, packet.get()) < 0 ||
        avcodec_send_packet(decoder.get(), packet.get()) < 0 ||
        avcodec_receive_frame(decoder.get(), frame.get()) < 0) {
        throw std::runtime_error("Cannot decode image");
    }

    return frame;
}

} // namespace

/**
 * @brief Load an image and normalize its decoded pixels to sRGB8.
 *
 * @param path Input image path accepted by FFmpeg.
 * @param background sRGB8 color used beneath transparent pixels.
 *
 * @return Owned sRGB8 image with transparency composited.
 */
ImageFrame loadImage(const std::string& path, Color background)
{
    // Keep decoder resources local to the load operation.
    Input input = openImage(path);
    Frame frame = decodeImage(input.get());

    // Copy normalized pixels into the application's independent data model.
    return normalizeFrame(*frame, background);
}

} // namespace asciixel
