#include "wenet/packet.hpp"

#include <algorithm>

namespace sq {

namespace wenet {

Packet::Flags convertFlags(Packet::Flags flags)
{
    using Exception = Packet::Exception;
    if (!flags) throw Exception{"Flags cannot be empty"};

    if (flags & Packet::Flag::Reliable) {
        if (flags & Packet::Flag::Unsequenced) {
            throw Exception{"Unsequenced packet cannot be reliable"};
        }
        if (flags & Packet::Flag::Fragment) {
            throw Exception{"Fragmented packets override reliability flag"};
        }
        if (flags & Packet::Flag::Unreliable) {
            throw Exception{"Packet is either reliable or unreliable"};
        }
    }

    return flags & ~Packet::Flag::Unreliable; // unreliable is a dummy flag
}

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

Packet::Packet(ENetPacket& packet, bool manage) noexcept
    : packet_(&packet), packetOwned_(manage ? &packet : nullptr) { }

void Packet::operator = (span<const byte> data) const
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

const Packet& Packet::operator << (span<const byte> data) const
{
    throwIfLocked();
    if (packet_->flags & Flag::Unmanaged) {
        throw FlagException{"Cannot modify unmanaged packet"};
    }

    const auto size = getSize();
    enet_packet_resize(packet_, size + data.size());
    std::copy(data.begin(), data.end(), packet_->data + size);
    return *this;
}

void Packet::onDestroy(const FreeCallback& callback) const noexcept
{
    if (packet_->userData) {
        delete reinterpret_cast<FreeCallback*>(packet_->userData);
        packet_->userData = nullptr;
        packet_->freeCallback = nullptr;
    }
    if (callback) {
        packet_->userData = new FreeCallback(callback);
        packet_->freeCallback = [](ENetPacket* packet) {
            const auto callback = reinterpret_cast<FreeCallback*>(packet->userData);
            (*callback)({*packet, false});
            delete reinterpret_cast<FreeCallback*>(packet->userData);
        };
    }
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

void Packet::create(span<const byte> data, uint32_t flags) noexcept
{
    packetOwned_.reset(enet_packet_create(&data[0], data.size(), flags));
    packet_ = packetOwned_.get();
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
