#include "wenet/packet.hpp"

#include <algorithm>

namespace sq {

namespace wenet {

// ENetPacket Deleter

void Packet::Deleter::operator () (ENetPacket* packet) const noexcept
{
    enet_packet_destroy(packet);
}

Packet::Packet(span<const byte> data, Flag flag)
    : Packet(data, belks::underlying_cast(flag)) { }

Packet::Packet(span<const byte> data, Flags flags)
{
    create(data, convertFlags(flags));
}

Packet::Packet(size_t size, Flag flag)
    : Packet(size, belks::underlying_cast(flag)) { }

Packet::Packet(size_t size, Flags flags) noexcept
    : Packet(*enet_packet_create(nullptr, size, convertFlags(flags))) { }

Packet::Packet(ENetPacket& packet) noexcept
    : packet_(&packet), packetOwned_(&packet) { }

void Packet::operator = (span<const byte> data)
{
    throwIfLocked();

    if (packet_->flags & Flag::Unmanaged) {
        // const cast is ok here because enet should not modify data
        // and the type being non-const seems like an oversight
        packet_->data = const_cast<byte*>(&data[0]);
    }
    else {
        if (size_t(data.size()) != packet_->dataLength) {
            enet_packet_resize(packet_, data.size());
        }
        std::copy(data.begin(), data.end(), packet_->data);
    }
}

Packet& Packet::operator << (span<const byte> data)
{
    throwIfLocked();
    if (packet_->flags & Flag::Unmanaged) {
        throw FlagException{"Cannot modify unmanaged packet"};
    }
    
    auto size = getSize();
    enet_packet_resize(packet_, size + data.size());
    std::copy(data.begin(), data.end(), packet_->data + size);
    return *this;
}

void Packet::setFlags(Flags flags) const
{
    throwIfLocked();

    if (flags & Flag::Unmanaged) {
        throw FlagException{"Cannot modify unmanaged flag"};
    }
    packet_->flags = convertFlags(flags);
}

void Packet::resize(size_t size) const
{
    throwIfLocked();

    if (packet_->flags & Flag::Unmanaged) packet_->dataLength = size;
    else enet_packet_resize(packet_, size);
}

span<byte> Packet::getData() const noexcept
{
    return {packet_->data, std::ptrdiff_t(packet_->dataLength)};
}

Packet::Flags Packet::convertFlags(Flags flags) const
{
    if (!flags) {
        throw FlagException{"Flags cannot be empty"};
    }
    if (flags & Flag::Unsequenced && flags & Flag::Reliable) {
        throw FlagException{"Unsequenced packet cannot be reliable"};
    }
    if (flags & Flag::Fragment && flags & Flag::Reliable) {
        throw FlagException{"Fragmented packets override reliability flag"};
    }
    if (flags & Flag::Unreliable && flags & Flag::Reliable) {
        throw FlagException{"Packet is either reliable or unreliable"};
    }
    return flags & ~Flag::Unreliable; // unreliable is a dummy flag
}

void Packet::create(span<const byte> data, uint32_t flags) noexcept
{
    const bool noAlloc = flags & ENET_PACKET_FLAG_NO_ALLOCATE;
    const byte* initialData = noAlloc ? &data[0] : nullptr;

    packetOwned_.reset(enet_packet_create(initialData, data.size(), flags));
    packet_ = packetOwned_.get();

    if (!noAlloc) std::copy(data.begin(), data.end(), packet_->data);
}

void Packet::throwIfLocked() const
{
    if (!packetOwned_.get()) {
        if (!packet_) {
            throw UninitialisedException{"Packet is not initialised yet"};
        }
        else throw UninitialisedException{"Packet cannot be modified"};
    }
}

} // \network

} // \sq
