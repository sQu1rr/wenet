#ifndef SQ_WENET_HOST_HPP
#define SQ_WENET_HOST_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include <memory>
#include <functional>
#include <vector>
#include <atomic>
#include <unordered_map>

#include "wenet/peer.hpp"
#include "wenet/units.hpp"
#include "wenet/packet.hpp"
#include "wenet/address.hpp"
#include "wenet/compressor.hpp"
#include "convw/convw.hpp"

namespace sq {

namespace wenet {

class Host {
    struct Deleter { void operator () (ENetHost* host) const noexcept; };

    struct Bandwidth {
        speed::bs incoming;
        speed::bs outgoing;
    };

public:
    using Callback = convw::Convw<void (Peer&, Packet&&, uint8_t)>;
    using ConnectCallback = convw::Convw<void (Peer&, uint32_t)>;
    using DisconnectCallback = convw::Convw<void (size_t, uint32_t)>;

    class Exception : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };
    class RangeCompressorInitException : public Exception {
    public: using Exception::Exception;
    };
    class ReceiveEventException : public Exception {
    public: using Exception::Exception;
    };
    class InitException : public Exception {
    public: using Exception::Exception;
    };

public:
    Host(size_t peerCount=1, const ENetAddress* address=nullptr);
    Host(const Address& address, size_t peerCount);

    operator ENetHost* () const noexcept { return host_.get(); }

    Peer& connect(const Address& address) noexcept;
    Peer& connect(const Address& address, size_t channelCount,
                  uint32_t data=0) noexcept;

    Bandwidth getBandwidth() const noexcept;
    void setBandwidth(const Bandwidth& bandwidth) noexcept;

    size_t getChannelLimit() const noexcept { return host_->channelLimit; }
    void setChannelLimit(size_t limit) noexcept;

    void broadcast(Packet& packet, uint8_t channelId=0) const noexcept;
    void broadcast(Packet&& packet, uint8_t channelId=0) const noexcept;

    void onReceive(Callback callback) noexcept;
    void onConnect(ConnectCallback callback) noexcept;
    void onDisconnect(DisconnectCallback callback) noexcept;

    bool receive(int limit=0) noexcept;
    bool service(int limit=0) noexcept;
    bool service(time::ms timeout, int limit=0) noexcept;

    void flush();

    template <typename Comp> void setCompression();
    void disableCompression() noexcept;

    // Unwrapped callbacks

    void _onChecksum(decltype(ENetHost::checksum) callback) const noexcept;
    void _onIntercept(decltype(ENetHost::intercept) callback) const noexcept;

    // Peers

    size_t getPeerCount() const noexcept { return peers_.size(); }
    size_t getPeerLimit() const noexcept { return host_->peerCount; }
    span<Peer> getPeers() noexcept { return gsl::as_span(peers_); }

    // 1.3.9
#if ENET_VERSION_CREATE(1, 3, 9) <= ENET_VERSION

    size_t getDuplicatePeers() const noexcept;
    void setDuplicatePeers(size_t value) const noexcept;

#endif

    // 1.3.12
#if ENET_VERSION_CREATE(1, 3, 12) <= ENET_VERSION

    size_t getMaximumPacketSize() const noexcept;
    void setMaximumPacketSize(size_t value) const noexcept;
    size_t getMaximumWaitingData() const noexcept;
    void setMaximumWaitingData(size_t value) const noexcept;

#endif

    // Info

    const Address& getAddress() const noexcept { return address_; }

    uint32_t getTotalReceivedData() const noexcept;
    uint32_t getTotalReceivedPackets() const noexcept;
    uint32_t getTotalSentData() const noexcept;
    uint32_t getTotalSentPackets() const noexcept;

    void removePeer(const Peer& peer) noexcept;

private:
    void parseEvent(ENetEvent& event);

    Peer& getPeer(ENetPeer& peer) noexcept;
    Peer& createPeer(ENetPeer& peer) noexcept;
    void removePeer(ENetPeer& peer) noexcept;

private:
    Address address_;
    Callback cbReceive_;
    ConnectCallback cbConnect_;
    DisconnectCallback cbDisconnect_;
    std::unique_ptr<ENetHost, Deleter> host_;
    std::unique_ptr<Compressor> compressor_;
    std::vector<Peer> peers_;

    static std::atomic<size_t> objects_;
};

template <typename Comp>
void Host::setCompression()
{
    static_assert(std::is_base_of<Compressor, Comp>::value, "Wrong class type");
    if (std::is_same<Comp, compressor::Range>::value) {
        compressor_.reset();
        if (enet_host_compress_with_range_coder(host_.get())) {
            throw RangeCompressorInitException{"Cannot initialise compressor"};
        }
    }
    else {
        compressor_ = std::make_unique<Comp>();
        ENetCompressor compressor{
            compressor_.get(),
            compressor_detail::compress<Comp>,
            compressor_detail::decompress<Comp>,
            nullptr
        };
        enet_host_compress(host_.get(), &compressor);
    }
}

} // \wenet

} // \sq

#endif
