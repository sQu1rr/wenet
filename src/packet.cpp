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

Packet::Packet(span<const byte> data, bool unreliable, bool allocate) noexcept
{
    uint flags = unreliable ? 0u : ENET_PACKET_FLAG_RELIABLE;
    if (!allocate) flags |= ENET_PACKET_FLAG_NO_ALLOCATE;
    create(data, flags);
}


Packet::Packet(Packet&& packet) noexcept
    : packet_(std::move(packet.packet_)) { }

void Packet::operator = (span<const byte> data)
{
    if (!packet_) {
        throw UninitialisedException{"Packet is not initialised yet"};
    }
    if (packet_->flags & ENET_PACKET_FLAG_NO_ALLOCATE) {
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
    if (packet_->flags & ENET_PACKET_FLAG_NO_ALLOCATE) {
        throw UnmanagedException{"Packet data is manually managed"};
    }
    auto size = getSize();
    enet_packet_resize(packet_.get(), size + data.size());
    std::copy(data.begin(), data.end(), packet_->data + size);
    return *this;
}

span<byte> Packet::getData() const noexcept
{
    return {packet_->data, static_cast<std::ptrdiff_t>(packet_->dataLength)};
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
