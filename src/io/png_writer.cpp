#include "asciixel/io/png_writer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#endif

namespace asciixel {
namespace {

/**
 * @brief Release an encoder context owned by the RAII handle.
 *
 * @param p Encoder context to release; may be null.
 *
 * @return No value.
 */
void freeEncoder(AVCodecContext* p)
{
    avcodec_free_context(&p);
}

/**
 * @brief Release a frame and its referenced pixel buffers.
 *
 * @param p Frame to release; may be null.
 *
 * @return No value.
 */
void freeFrame(AVFrame* p)
{
    av_frame_free(&p);
}

/**
 * @brief Release an encoded packet and its data.
 *
 * @param p Packet to release; may be null.
 *
 * @return No value.
 */
void freePacket(AVPacket* p)
{
    av_packet_free(&p);
}

} // namespace

/**
 * @brief Encode a grayscale bitmap to a newly created PNG file.
 *
 * @param bitmap Tightly packed eight-bit grayscale pixels within size limits.
 * @param path UTF-8 destination path; existing files are never overwritten.
 *
 * @return No value; encoding or file errors throw an exception.
 */
void writePng(const GrayBitmap& bitmap, const std::string& path)
{
    // Check buffer consistency and dimensions before allocating codec resources.
    if (!bitmap.width || !bitmap.height ||
        bitmap.width > maxBitmapPixels / bitmap.height ||
        bitmap.width > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        bitmap.height > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        bitmap.pixels.size() != bitmap.width * bitmap.height) {
        throw std::invalid_argument("Invalid or excessive grayscale bitmap size");
    }

    // Find the encoder and retain all FFmpeg allocations under RAII ownership.
    const auto* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        throw std::runtime_error(
            "PNG encoder unavailable; rebuild FFmpeg with --enable-encoder=png");
    }
    std::unique_ptr<AVCodecContext, decltype(&freeEncoder)> encoder(
        avcodec_alloc_context3(codec), freeEncoder);
    std::unique_ptr<AVFrame, decltype(&freeFrame)> frame(
        av_frame_alloc(), freeFrame);
    std::unique_ptr<AVPacket, decltype(&freePacket)> packet(
        av_packet_alloc(), freePacket);
    if (!encoder || !frame || !packet) {
        throw std::runtime_error("Cannot allocate PNG encoder buffers");
    }

    // Configure a single-frame grayscale encoder.
    encoder->width     = static_cast<int>(bitmap.width);
    encoder->height    = static_cast<int>(bitmap.height);
    encoder->pix_fmt   = AV_PIX_FMT_GRAY8;
    encoder->time_base = {1, 1};
    if (avcodec_open2(encoder.get(), codec, nullptr) < 0)
        throw std::runtime_error("Cannot open PNG encoder");

    // Allocate codec-managed rows, which may include padding.
    frame->width  = encoder->width;
    frame->height = encoder->height;
    frame->format = encoder->pix_fmt;
    frame->pts    = 0;
    if (av_frame_get_buffer(frame.get(), 0) < 0)
        throw std::runtime_error("Cannot allocate PNG frame");

    // Copy source rows into the codec's stride-aware storage.
    for (std::size_t y = 0; y < bitmap.height; ++y) {
        std::copy_n(bitmap.pixels.data() + y * bitmap.width, bitmap.width,
                    frame->data[0] + y * static_cast<std::size_t>(frame->linesize[0]));
    }

    // Produce the complete PNG packet before creating the output file.
    if (avcodec_send_frame(encoder.get(), frame.get()) < 0 ||
        avcodec_receive_packet(encoder.get(), packet.get()) < 0) {
        throw std::runtime_error("Cannot encode PNG image");
    }

    // Create exclusively with native Unicode path support on Windows.
    const auto destination = std::filesystem::u8path(path);
#ifdef _WIN32
    // Older Windows CRTs do not support fopen's C11 'x' mode.
    const int descriptor = _wopen(destination.c_str(),
                                  _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
    if (descriptor < 0)
        throw std::runtime_error("Cannot create PNG (file may already exist): " + path);
    std::FILE* raw = _fdopen(descriptor, "wb");
    if (!raw) {
        _close(descriptor);
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw std::runtime_error("Cannot open PNG output stream: " + path);
    }
#else
    std::FILE* raw = std::fopen(destination.c_str(), "wbx");
#endif
    if (!raw) {
        throw std::runtime_error("Cannot create PNG (file may already exist): " + path);
    }

    // Check both writing and closing so delayed filesystem errors are reported.
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file(raw, std::fclose);
    const bool written =
        std::fwrite(packet->data, 1, packet->size, file.get()) == static_cast<std::size_t>(packet->size);
    const bool closed = std::fclose(file.release()) == 0;

    // Remove the newly created file when output is incomplete.
    if (!written || !closed) {
        std::error_code ignored;
        std::filesystem::remove(destination, ignored);
        throw std::runtime_error("Cannot write PNG: " + path);
    }
}

} // namespace asciixel
