#include "wenet/packet.hpp"

#include <algorithm>

namespace sq {

namespace wenet {

// ENetPacket Deleter

void Packet::Deleter::operator () (ENetPacket* packet) const noexcept
{
    enet_packet_destroy(packet);
}

Packet::Packet(ENetPacket& packet) noexcept : packet_(&packet) { }

Packet::Packet(span<const byte> data, Flags flags)
{
    create(data, convertFlags(flags));
}

Packet::Packet(Packet&& packet) noexcept
    : packet_(std::move(packet.packet_)) { }

void Packet::operator = (span<const byte> data)
{
    if (!packet_) {
        throw UninitialisedException{"Packet is not initialised yet"};
    }
    if (packet_->flags & Flag::Unmanaged) {
        throw UnmanagedException{"Packet data is manually managed"};
    }
    if (static_cast<size_t>(data.size()) != packet_->dataLength) {
        enet_packet_resize(packet_.get(), data.size());
    }
    std::copy(data.begin(), data.end(), packet_->data);
}

void Packet::operator = (Packet&& packet) noexcept
{
    packet_ = std::move(packet.packet_);
}

Packet& Packet::operator << (span<const byte> data)
{
    if (!packet_) {
        throw UninitialisedException{"Packet is not initialised yet"};
    }
    auto size = getSize();
    resize(size + data.size());
    std::copy(data.begin(), data.end(), packet_->data + size);
    return *this;
}

void Packet::setFlags(Flags flags) const
{
    if (!packet_) {
        throw UninitialisedException{"Packet is not initialised yet"};
    }
    packet_->flags = convertFlags(flags);
}

void Packet::resize(size_t size) const
{
    if (packet_->flags & Flag::Unmanaged) {
        throw UnmanagedException{"Packet data is manually managed"};
    }
    enet_packet_resize(packet_.get(), size);
}

span<byte> Packet::getData() const noexcept
{
    return {packet_->data, static_cast<std::ptrdiff_t>(packet_->dataLength)};
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
    if (packet_ && flags & Flag::Unmanaged) {
        throw FlagException{"Cannot modify unmanaged flag"};
    }
    return flags & ~Flag::Unreliable; // unreliable is a dummy flag
}

void Packet::create(span<const byte> data, uint32_t flags) noexcept
{
    const bool noAlloc = flags & ENET_PACKET_FLAG_NO_ALLOCATE;
    const byte* initialData = noAlloc ? &data[0] : nullptr;

    packet_.reset(enet_packet_create(initialData, data.size(), flags));

    if (!noAlloc) std::copy(data.begin(), data.end(), packet_->data);
}

} // \network

} // \sq
