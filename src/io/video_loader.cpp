#include "asciixel/io/video_loader.hpp"
#include "asciixel/io/frame_normalizer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
}

#include <stdexcept>

namespace asciixel {
namespace {

/**
 * @brief Turn an FFmpeg failure into an operation-specific runtime error.
 *
 * @param result FFmpeg result code; nonnegative values indicate success.
 * @param operation Description of the operation that produced the result.
 */
void check(int result, const std::string& operation)
{
    if (result >= 0) {
        return;
    }

    char message[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(result, message, sizeof(message));
    throw std::runtime_error(operation + ": " + message);
}

} // namespace

struct VideoLoader::Impl {
    AVFormatContext* input = nullptr;
    AVCodecContext* decoder = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    int stream = -1;
    bool pending_packet = false;
    bool input_eof = false;
    bool draining = false;
    bool finished = false;
    Color background;
    std::string path;

    // Own all native resources here, including during constructor failures.
    ~Impl()
    {
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&decoder);
        avformat_close_input(&input);
    }

    /**
     * @brief Copy decoded pixels and source presentation timing into owned data.
     *
     * @return Linear RGB frame with optional source times in microseconds.
     */
    VideoFrame copyFrame() const
    {
        const AVRational time_base = input->streams[stream]->time_base;
        VideoFrame result{
            normalizeFrame(*frame, background), std::nullopt, std::nullopt};

        // Keep absent timestamps absent; playback will resolve its own timeline.
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            result.timestamp_us = av_rescale_q(
                frame->best_effort_timestamp, time_base, AVRational{1, 1000000});
        }
        if (frame->duration > 0) {
            result.duration_us = av_rescale_q(
                frame->duration, time_base, AVRational{1, 1000000});
        }

        return result;
    }
};

/**
 * @brief Open a local media file and prepare its best video stream for decoding.
 *
 * @param path UTF-8 input path accepted by the FFmpeg file protocol.
 * @param background Linear RGB background for transparent source pixels.
 */
VideoLoader::VideoLoader(const std::string& path, Color background)
    : impl_(std::make_unique<Impl>())
{
    impl_->background = background;
    impl_->path = path;

    // Probe streams before selecting a video decoder; audio streams are ignored.
    check(avformat_open_input(&impl_->input, path.c_str(), nullptr, nullptr),
          "Cannot open video " + path);
    check(avformat_find_stream_info(impl_->input, nullptr),
          "Cannot read video " + path);
    impl_->stream = av_find_best_stream(impl_->input, AVMEDIA_TYPE_VIDEO,
                                       -1, -1, nullptr, 0);
    check(impl_->stream, "Video stream not found in " + path);
    const auto* parameters = impl_->input->streams[impl_->stream]->codecpar;
    const auto* codec = avcodec_find_decoder(parameters->codec_id);
    if (!codec) {
        throw std::runtime_error("Video decoder not found: " + path);
    }

    // Copy stream parameters and retain their time base for decoded frame times.
    impl_->decoder = avcodec_alloc_context3(codec);
    if (!impl_->decoder) {
        throw std::runtime_error("Cannot allocate video decoder");
    }
    check(avcodec_parameters_to_context(impl_->decoder, parameters),
          "Cannot configure video decoder");
    impl_->decoder->pkt_timebase =
        impl_->input->streams[impl_->stream]->time_base;
    check(avcodec_open2(impl_->decoder, codec, nullptr),
          "Cannot open video decoder");

    // These buffers are reused; nextFrame copies pixels before returning.
    impl_->packet = av_packet_alloc();
    impl_->frame = av_frame_alloc();
    if (!impl_->packet || !impl_->frame) {
        throw std::runtime_error("Cannot allocate video buffers");
    }
}

VideoLoader::~VideoLoader() = default;

/**
 * @brief Decode the next display-order frame, including buffered frames at EOF.
 *
 * @return Owned frame, or nullopt once the decoder reaches its own EOF.
 */
std::optional<VideoFrame> VideoLoader::nextFrame()
{
    auto& state = *impl_;
    if (state.finished) {
        return std::nullopt;
    }

    for (;;) {
        // Always receive available frames before feeding more compressed data.
        const int received = avcodec_receive_frame(state.decoder, state.frame);
        if (received == 0) {
            auto result = state.copyFrame();
            av_frame_unref(state.frame);
            return result;
        }
        if (received == AVERROR_EOF) {
            state.finished = true;
            return std::nullopt;
        }
        if (received != AVERROR(EAGAIN)) {
            check(received, "Cannot decode video " + state.path);
        }
        if (state.draining) {
            throw std::runtime_error("Video decoder requested input after draining");
        }

        // Skip other streams and retain an unsent target packet until accepted.
        while (!state.pending_packet && !state.input_eof) {
            const int read = av_read_frame(state.input, state.packet);
            if (read == AVERROR_EOF) {
                state.input_eof = true;
                break;
            }
            check(read, "Cannot read video packet " + state.path);
            if (state.packet->stream_index == state.stream) {
                state.pending_packet = true;
            }
            else {
                av_packet_unref(state.packet);
            }
        }

        // A single null packet starts draining only after demuxing reaches EOF.
        const int sent = avcodec_send_packet(
            state.decoder, state.pending_packet ? state.packet : nullptr);

        // Both ends returning EAGAIN without progress violates FFmpeg's contract.
        if (sent == AVERROR(EAGAIN)) {
            throw std::runtime_error("Video decoder made no progress");
        }
        check(sent, "Cannot send video packet " + state.path);

        // Release compressed data only after the decoder accepts the packet.
        if (state.pending_packet) {
            av_packet_unref(state.packet);
            state.pending_packet = false;
        }
        else {
            state.draining = true;
        }
    }
}

} // namespace asciixel
