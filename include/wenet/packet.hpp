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
    Packet(span<const byte> data, Flag flag=Flag::Reliable)
        : Packet(data, belks::underlying_cast(flag)) { }
    Packet(span<const byte> data, Flags flags);
    Packet(size_t size, Flag flag=Flag::Reliable)
        : Packet(size, belks::underlying_cast(flag)) { }
    Packet(size_t size, Flags flags) noexcept;
    Packet(ENetPacket& packet) noexcept;

    void operator = (span<const byte> data);
    void operator = (Packet&& packet) noexcept;

    Packet& operator << (span<const byte> data);

    void setFlags(Flags flags) const;
    void resize(size_t size) const;

    span<byte> getData() const noexcept;
    size_t getSize() const noexcept { return packet_->dataLength; }

    ENetPacket* getPacket() const noexcept { return packet_.get(); }
    ENetPacket* releasePacket() noexcept { return packet_.release(); }

private:
    Flags convertFlags(Flags flags) const;
    void create(span<const byte> data, uint32_t flags) noexcept;

private:
    std::unique_ptr<ENetPacket, Deleter> packet_;
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
