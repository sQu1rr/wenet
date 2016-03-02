#ifndef SQ_WENET_COMPRESSOR_HPP
#define SQ_WENET_COMPRESSOR_HPP

#include "belks/base.hpp"

#include <enet/enet.h>
#include <zlib.h>

#include <functional>

namespace sq {

namespace wenet {

using BufferVector = std::vector<span<const byte>>;

struct Compressor {
    enum class Type { RangeCompressor };

    size_t compress(const BufferVector& in, size_t inLen, span<byte> out);
    size_t decompress(span<const byte> in, span<byte> out);
};

namespace compressor_detail {

template <typename T>
size_t compress(void* context, const ENetBuffer* buffers, size_t bufferCount,
                size_t sourceLen, byte* dest, size_t destLen)
{
    std::vector<span<const byte>> in; in.reserve(bufferCount);
    auto bufferView = span<const ENetBuffer>{
        buffers, static_cast<std::ptrdiff_t>(bufferCount)
    };
    for (auto& buffer : bufferView) {
        in.emplace_back(static_cast<byte*>(buffer.data), buffer.dataLength);
    }

    T& compressor = *reinterpret_cast<T*>(context);
    return compressor.compress(in, sourceLen,
                               {dest, static_cast<std::ptrdiff_t>(destLen)});
}

template <typename T>
size_t decompress(void* context, const byte* source, size_t sourceLen,
                  byte* dest, size_t destLen)
{
    T& compressor = *reinterpret_cast<T*>(context);
    return compressor.decompress(
        {source, static_cast<std::ptrdiff_t>(sourceLen)},
        {dest, static_cast<std::ptrdiff_t>(destLen)}
    );
}

} // \compressor_detail

class CompressorZlib : public Compressor {
public:
    size_t compress(const BufferVector& in, size_t inLen, span<byte> out)
    {
        std::vector<byte> data; data.reserve(inLen);
        for (auto& buffer : in)  {
            std::copy(buffer.begin(), buffer.end(), std::back_inserter(data));
        }

        auto destLen = size_t(out.size());
        if (::compress(&out[0], &destLen, &data[0], data.size()) != Z_OK) {
            return 0;
        }
        return destLen;
    }

    size_t decompress(span<const byte> in, span<byte> out)
    {
        auto destLen = size_t(out.size());
        if (uncompress(&out[0], &destLen, &in[0], in.size()) != Z_OK) return 0;
        return destLen;
    }
};

} // \wenet

} // \sq

#endif
