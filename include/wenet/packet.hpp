#ifndef SQ_WENET_PACKET_HPP
#define SQ_WENET_PACKET_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include <memory>

namespace sq {

namespace wenet {

class Packet {
    struct Deleter { void operator () (ENetPacket* packet) const noexcept; };

    class Exception : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };
    class UninitialisedException : public Exception {
    public: using Exception::Exception;
    };
    class UnmanagedException : public Exception {
    public: using Exception::Exception;
    };

public:
    Packet() noexcept = default;
    Packet(span<const byte> data, bool unreliable=false,
           bool allocate=true) noexcept;
    Packet(ENetPacket& packet) noexcept;
    Packet(Packet&& packet) noexcept;

    void operator = (span<const byte> data);
    void operator = (Packet&& packet) noexcept;

    Packet& operator << (span<const byte> data);

    span<byte> getData() const noexcept;
    size_t getSize() const noexcept { return packet_->dataLength; }

    ENetPacket* getPacket() const noexcept { return packet_.get(); }
    ENetPacket* releasePacket() noexcept { return packet_.release(); }

private:
    void create(span<const byte> data, uint32_t flags) noexcept;

private:
    std::unique_ptr<ENetPacket, Deleter> packet_;
};

} // \wenet

} // \sq

#endif
