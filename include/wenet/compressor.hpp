#ifndef SQ_WENET_COMPRESSOR_HPP
#define SQ_WENET_COMPRESSOR_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include <zlib.h>

#include <functional>
#include <array>

namespace sq {

namespace wenet {

namespace wrapper {

using BufferVector = std::vector<span<const byte>>;

} // \wrapper


class Compressor {
public:
    class InitialisationException : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };

public:
    virtual size_t compress(wrapper::BufferVector&& in, size_t inLen,
                            span<byte> out) noexcept = 0;
    virtual size_t decompress(span<const byte> in, span<byte> out) noexcept = 0;
};

namespace compressor_detail {

template <typename T>
inline size_t compress(void* context, const ENetBuffer* buffers,
                       size_t bufferCount, size_t sourceLen, byte* dest,
                       size_t destLen)
{
    std::vector<span<const byte>> in; in.reserve(bufferCount);
    auto bufferSpan = span<const ENetBuffer>{
        buffers, std::ptrdiff_t(bufferCount)
    };
    for (auto& buffer : bufferSpan) {
        in.emplace_back(static_cast<byte*>(buffer.data), buffer.dataLength);
    }

    T& compressor = *reinterpret_cast<T*>(context);
    return compressor.compress(std::move(in), sourceLen,
                               {dest, std::ptrdiff_t(destLen)});
}

template <typename T>
inline size_t decompress(void* context, const byte* source, size_t sourceLen,
                         byte* dest, size_t destLen)
{
    T& compressor = *reinterpret_cast<T*>(context);
    return compressor.decompress({source, std::ptrdiff_t(sourceLen)},
                                 {dest, std::ptrdiff_t(destLen)});
}

} // \compressor_detail

namespace compressor {

using namespace wrapper;

class Range final : public virtual Compressor { };

class Zlib final : public virtual Compressor {
public:
    Zlib();
    ~Zlib() noexcept;

    size_t compress(BufferVector&& in, size_t inLen,
                    span<byte> out) noexcept override;
    size_t decompress(span<const byte> in, span<byte> out) noexcept override;

private:
    z_stream streamDef_;
    z_stream streamInf_;
};

} // \compressor

} // \wenet

} // \sq

#endif
