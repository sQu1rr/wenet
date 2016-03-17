#ifndef SQ_WENET_PACKET_HPP
#define SQ_WENET_PACKET_HPP

#include "belks/base.hpp"
#include "belks/util.hpp"

#include <enet/enet.h>

#include <memory>

namespace sq {

namespace wenet {

class Packet {
    struct Deleter { void operator () (ENetPacket* packet) const noexcept; };

public:
    enum class Flag : uint32_t {
        Reliable = ENET_PACKET_FLAG_RELIABLE,
        Unsequenced = ENET_PACKET_FLAG_UNSEQUENCED,
        Unmanaged = ENET_PACKET_FLAG_NO_ALLOCATE,
        Fragment = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
        Unreliable = ENET_PACKET_FLAG_SENT // just a random bit
    };
    using Flags = std::underlying_type<Flag>::type;

    class Exception : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };
    class UninitialisedException : public Exception {
    public: using Exception::Exception;
    };
    class UnmanagedException : public Exception {
    public: using Exception::Exception;
    };
    class FlagException : public Exception {
    public: using Exception::Exception;
    };
    
public:
    Packet() noexcept = default;
    Packet(span<const byte> data, Flag flag=Flag::Reliable);
    Packet(span<const byte> data, Flags flags);
    Packet(size_t size, Flag flag=Flag::Reliable);
    Packet(size_t size, Flags flags) noexcept;
    Packet(ENetPacket& packet, bool manage=true) noexcept;

    operator ENetPacket* () const noexcept { return packet_; }

    void operator = (span<const byte> data);

    Packet& operator << (span<const byte> data);

    void setFlags(Flags flags) const;
    void resize(size_t size) const;

    span<byte> getData() const noexcept;
    size_t getSize() const noexcept { return packet_->dataLength; }

    void releaseOwnership() noexcept { packetOwned_.release(); }

    bool isInit() const noexcept { return packet_; }
    bool isOwned() const noexcept { return packetOwned_.get(); }

private:
    Flags convertFlags(Flags flags) const;
    void create(span<const byte> data, uint32_t flags) noexcept;

    void throwIfLocked() const;

private:
    ENetPacket* packet_ = nullptr;
    std::unique_ptr<ENetPacket, Deleter> packetOwned_;
};

constexpr auto operator | (Packet::Flag flag1, Packet::Flag flag2) noexcept
{
    return belks::underlying_cast(flag1) | belks::underlying_cast(flag2);
}

constexpr auto operator | (Packet::Flags flags, Packet::Flag flag) noexcept
{
    return flags | belks::underlying_cast(flag);
}

constexpr auto operator | (Packet::Flag flag, Packet::Flags flags) noexcept
{
    return flags | belks::underlying_cast(flag);
}

constexpr auto operator & (Packet::Flag flag1, Packet::Flag flag2) noexcept
{
    return belks::underlying_cast(flag1) & belks::underlying_cast(flag2);
}

constexpr auto operator & (Packet::Flags flags, Packet::Flag flag) noexcept
{
    return flags & belks::underlying_cast(flag);
}

constexpr auto operator & (Packet::Flag flag, Packet::Flags flags) noexcept
{
    return flags & belks::underlying_cast(flag);
}

constexpr auto operator ~ (Packet::Flag flag) noexcept
{
    return ~belks::underlying_cast(flag);
}

} // \wenet

} // \sq

#endif
