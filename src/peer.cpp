#include "wenet/peer.hpp"

#include "wenet/host.hpp"

namespace sq {

namespace wenet {

Peer& Peer::operator = (ENetPeer& peer) noexcept
{
    peer_ = &peer;
    address_ = peer.address;
    return *this;
}

// Disconnect

void Peer::disconnect(uint32_t data) const noexcept
{
    enet_peer_disconnect(peer_, data);
}

void Peer::disconnectNow(uint32_t data) const noexcept
{
    enet_peer_disconnect_now(peer_, data);
    host_->removePeer(*this);
}

void Peer::disconnectLater(uint32_t data) const noexcept
{
    enet_peer_disconnect_later(peer_, data);
}

void Peer::drop() const noexcept
{
    enet_peer_reset(peer_);
    host_->removePeer(*this);
}

// Ping

void Peer::ping() const noexcept
{
    enet_peer_ping(peer_);
}

time::ms Peer::getPingInterval() const noexcept
{
    return time::ms{peer_->pingInterval};
}

void Peer::setPingInterval(time::ms interval) const noexcept
{
    enet_peer_ping_interval(peer_, interval.count());
}

// Receive

void Peer::receive(Callback callback) const noexcept
{
    uint8_t channelId;
    auto packet = enet_peer_receive(peer_, &channelId);
    callback(Packet{*packet}, channelId);
}

// Send

void Peer::send(Packet& packet, uint8_t channelId) const noexcept
{
    if (packet.isOwned()) packet.releaseOwnership();
    enet_peer_send(peer_, channelId, packet);
}

void Peer::send(Packet&& packet, uint8_t channelId) const noexcept
{
    packet.releaseOwnership();
    enet_peer_send(peer_, channelId, packet);
}

// Throttle

Peer::Throttle Peer::getThrottle() const noexcept
{
    return Throttle{
        time::ms{peer_->packetThrottleInterval},
        peer_->packetThrottleAcceleration,
        peer_->packetThrottleDeceleration
    };
}

void Peer::setThrottle(const Throttle& throttle) noexcept
{
    enet_peer_throttle_configure(peer_, throttle.interval.count(),
                                 throttle.acceleration, throttle.deceleration);
}

// Timeout

Peer::Timeout Peer::getTimeout() const noexcept
{
    return Timeout{
        time::ms{peer_->timeoutLimit},
        time::ms{peer_->timeoutMinimum},
        time::ms{peer_->timeoutMaximum}
    };
}

void Peer::setTimeout(const Timeout& timeout) noexcept
{
    enet_peer_timeout(peer_, timeout.limit.count(), timeout.minimum.count(),
                      timeout.maximum.count());
}

// Info

Peer::State Peer::getState() const noexcept
{
    return static_cast<State>(peer_->state);
}

speed::bs Peer::getIncomingBandwidth() const noexcept
{
    return peer_->incomingBandwidth;
}

speed::bs Peer::getOutgoingBandwidth() const noexcept
{
    return peer_->outgoingBandwidth;
}

time::ms Peer::getRoundTripTime() const noexcept
{
    return time::ms{peer_->roundTripTime};
}

uint32_t Peer::getPacketLoss() const noexcept
{
    return peer_->packetLoss;
}

} // \wenet

} // \sq
