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

void closeInput(AVFormatContext* context)
{
    avformat_close_input(&context);
}

void freeDecoder(AVCodecContext* context)
{
    avcodec_free_context(&context);
}

void freePacket(AVPacket* packet)
{
    av_packet_free(&packet);
}

void freeFrame(AVFrame* frame)
{
    av_frame_free(&frame);
}

using Input   = std::unique_ptr<AVFormatContext, decltype(&closeInput)>;
using Decoder = std::unique_ptr<AVCodecContext, decltype(&freeDecoder)>;
using Packet  = std::unique_ptr<AVPacket, decltype(&freePacket)>;
using Frame   = std::unique_ptr<AVFrame, decltype(&freeFrame)>;

Input openImage(const std::string& path)
{
    AVFormatContext* context = nullptr;
    if (avformat_open_input(&context, path.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Cannot open image: " + path);
    }
    Input input(context, closeInput);
    if (avformat_find_stream_info(input.get(), nullptr) < 0) {
        throw std::runtime_error("Cannot read image: " + path);
    }
    return input;
}

Frame decodeImage(AVFormatContext* input)
{
    const int stream = av_find_best_stream(input, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream < 0) {
        throw std::runtime_error("Image stream not found");
    }

    const AVCodecParameters* parameters = input->streams[stream]->codecpar;
    const AVCodec*           codec      = avcodec_find_decoder(parameters->codec_id);
    if (!codec) {
        throw std::runtime_error("Image decoder not found");
    }
    Decoder decoder(avcodec_alloc_context3(codec), freeDecoder);
    if (!decoder || avcodec_parameters_to_context(decoder.get(), parameters) < 0 || avcodec_open2(decoder.get(), codec, nullptr) < 0) {
        throw std::runtime_error("Cannot open image decoder");
    }

    Packet packet(av_packet_alloc(), freePacket);
    Frame  frame(av_frame_alloc(), freeFrame);
    if (!packet || !frame) {
        throw std::runtime_error("Cannot allocate image buffer");
    }
    if (av_read_frame(input, packet.get()) < 0 || avcodec_send_packet(decoder.get(), packet.get()) < 0 || avcodec_receive_frame(decoder.get(), frame.get()) < 0) {
        throw std::runtime_error("Cannot decode image");
    }
    return frame;
}

} // namespace

ImageFrame loadImage(const std::string& path, Color background)
{
    Input input = openImage(path);
    Frame frame = decodeImage(input.get());
    return normalizeFrame(*frame, background);
}

} // namespace asciixel
