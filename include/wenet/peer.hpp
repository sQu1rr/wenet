#ifndef SQ_WENET_PEER_HPP
#define SQ_WENET_PEER_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include "wenet/units.hpp"
#include "wenet/address.hpp"
#include "wenet/packet.hpp"
#include "convw/convw.hpp"

namespace sq {

namespace wenet {

class Host;

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

    enum class State {
        Disconnected = ENET_PEER_STATE_DISCONNECTED,
        Connecting = ENET_PEER_STATE_CONNECTING,
        AcknowledgingConnect = ENET_PEER_STATE_ACKNOWLEDGING_CONNECT,
        ConnectionPending = ENET_PEER_STATE_CONNECTION_PENDING,
        ConnectionSucceeded = ENET_PEER_STATE_CONNECTION_SUCCEEDED,
        Connected = ENET_PEER_STATE_CONNECTED,
        DisconnectLater = ENET_PEER_STATE_DISCONNECT_LATER,
        Disconnecting = ENET_PEER_STATE_DISCONNECTING,
        AcknowledgingDisconnect = ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT,
        Zombie = ENET_PEER_STATE_ZOMBIE 
    };

    using Callback = convw::Convw<void (const Packet&, uint8_t)>;

public:
    Peer(Host& host) noexcept : host_(&host) { }
    Peer(Host& host, ENetPeer& peer) noexcept
        : host_(&host), peer_(&peer), address_(peer.address) { }

    Peer& operator = (ENetPeer& peer) noexcept;

    operator ENetPeer* () const noexcept { return peer_; }

    size_t getId() const noexcept { return size_t(this); }

    // Disconnect

    void disconnect(uint32_t data=0) const noexcept;
    void disconnectNow(uint32_t data=0) const noexcept;
    void disconnectLater(uint32_t data=0) const noexcept;
    void drop() const noexcept;

    // Ping

    void ping() const noexcept;
    time::ms getPingInterval() const noexcept;
    void setPingInterval(time::ms interval) const noexcept;

    // Receive

    void receive(Callback callback) const noexcept;

    // Send

    void send(Packet& packet, uint8_t channelId=0) const noexcept;
    void send(Packet&& packet, uint8_t channelId=0) const noexcept;

    // Throttle

    Throttle getThrottle() const noexcept;
    void setThrottle(const Throttle& throttle) noexcept;

    // Timeout
    
    Timeout getTimeout() const noexcept;
    void setTimeout(const Timeout& timeout) noexcept;

    // Info

    const Address& getAddress() const noexcept { return address_; }

    State getState() const noexcept;

    speed::bs getIncomingBandwidth() const noexcept;
    speed::bs getOutgoingBandwidth() const noexcept;
    time::ms getRoundTripTime() const noexcept;
    uint32_t getPacketLoss() const noexcept;

private:
    Host* host_;
    ENetPeer* peer_;
    Address address_;
};

} // \wenet

} // \sq

#endif
