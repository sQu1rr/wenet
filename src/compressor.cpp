#include "wenet/compressor.hpp"

#include <iostream>

namespace sq {

namespace wenet {

namespace compressor {

Zlib::Zlib()
{
    streamDef_.zalloc = streamInf_.zalloc = nullptr;
    streamDef_.zfree = streamInf_.zfree = nullptr;
    streamDef_.opaque = streamInf_.opaque = nullptr;

    if (deflateInit(&streamDef_, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw Compressor::InitialisationException{"Failed to initialise zlib"};
    }

    inflateInit(&streamInf_);
}

Zlib::~Zlib() noexcept
{
    deflateEnd(&streamDef_);
    inflateEnd(&streamInf_);
}

size_t Zlib::compress(BufferVector&& in, size_t size, span<byte> out) noexcept
{
    const auto outMax = out.size();
    deflateReset(&streamDef_);

    for (auto i = 0u; i < in.size(); ++i) {
        auto buffer = in[i];

        streamDef_.avail_in = buffer.size();
        streamDef_.next_in = const_cast<byte*>(&buffer[0]);
        streamDef_.avail_out = out.size();
        streamDef_.next_out = &out[0];

        const auto last = i == in.size() - 1;
        const auto expected = last ? Z_STREAM_END : Z_OK;
        const auto flush = last ? Z_FINISH : Z_NO_FLUSH;
        if (deflate(&streamDef_, flush) != expected) return 0;

        const auto bytes = out.size() - streamDef_.avail_out;
        out = {&out[0] + bytes, std::ptrdiff_t(streamDef_.avail_out)};
    }

    return outMax - streamDef_.avail_out;
}

size_t Zlib::decompress(span<const byte> in, span<byte> out) noexcept
{
    const auto outMax = out.size();
    inflateReset(&streamInf_);

    streamInf_.avail_in = in.size();
    streamInf_.next_in = const_cast<byte*>(&in[0]);
    streamInf_.avail_out = out.size();
    streamInf_.next_out = &out[0];

    if (inflate(&streamInf_, Z_NO_FLUSH) != Z_STREAM_END) return 0;

    return outMax - streamDef_.avail_out;
}

} // \compressor

} // \wenet

} // \sq
