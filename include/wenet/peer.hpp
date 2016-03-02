#ifndef SQ_WENET_PEER_HPP
#define SQ_WENET_PEER_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include "wenet/units.hpp"
#include "wenet/address.hpp"
#include "wenet/packet.hpp"

namespace sq {

namespace wenet {

class Peer {
public:
    struct Throttle {
        time::ms interval;
        uint32_t acceleration;
        uint32_t deceleration;
    };

    struct Timeout {
        time::ms limit;
        time::ms minimum;
        time::ms maximum;
    };

    using Callback = std::function<void (const Packet&, uint8_t)>;

public:
    Peer(ENetPeer& peer) noexcept : peer_(peer) { peer_.data = this; }
    Peer(Peer&& peer) = default;
    ~Peer() { peer_.data = nullptr; }

    void operator = (Peer&& peer);

    operator ENetPeer* () const noexcept { return &peer_; }

    // Disconnect

    template <typename T>
    void disconnect(T data) const noexcept { disconnect(uint32_t(data)); }
    void disconnect(uint32_t data=0) const noexcept;

    template <typename T>
    void disconnectNow(T data) const noexcept { disconnectNow(uint32_t(data)); }
    void disconnectNow(uint32_t data=0) const noexcept;

    template <typename T>
    void disconnectLater(T data) const noexcept
        { disconnectLater(uint32_t(data)); }
    void disconnectLater(uint32_t data=0) const noexcept;

    void dropConnection() const noexcept;

    // Ping

    void ping() const noexcept;
    time::ms getPingInterval() const noexcept;
    void setPingInterval(time::ms interval) const noexcept;

    // Receive

    void receive(const Callback& callback) const noexcept;

    // Send

    void send(Packet&& packet, uint8_t channelId=0) const noexcept;

    // Throttle

    Throttle getThrottle() const noexcept;
    void setThrottle(const Throttle& throttle) noexcept;

    // Timeout
    
    Timeout getTimeout() const noexcept;
    void setTimeout(const Timeout& timeout) noexcept;

    // Info

    Address getAddress() const noexcept { return {peer_.address}; }

    speed::bs getIncomingBandwidth() const noexcept;
    speed::bs getOutgoingBandwidth() const noexcept;
    time::ms getRoundTripTime() const noexcept;
    uint32_t getPacketLoss() const noexcept;

private:
    ENetPeer& peer_;
};

} // \wenet

} // \sq

#endif
