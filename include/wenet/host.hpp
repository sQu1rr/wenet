#ifndef SQ_WENET_HOST_HPP
#define SQ_WENET_HOST_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

#include <memory>
#include <functional>
#include <vector>
#include <atomic>

#include "wenet/peer.hpp"
#include "wenet/units.hpp"
#include "wenet/packet.hpp"
#include "wenet/address.hpp"
#include "wenet/compressor.hpp"
#include "convw/convw.hpp"

namespace sq {

namespace wenet {

using convw::Convw;

class Host {
    struct Deleter { void operator () (ENetHost* host) const noexcept; };

    struct Bandwidth {
        speed::bs incoming;
        speed::bs outgoing;
    };

public:
    using Callback = Convw<void (Peer&, Packet&&, uint8_t)>;
    using EventCallback = Convw<void (Peer&, uint32_t)>;

    class Exception : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };
    class RangeCompressorInitException : public Exception {
    public: using Exception::Exception;
    };
    class ReceiveEventException : public Exception {
    public: using Exception::Exception;
    };
    class InitialisationException : public Exception {
    public: using Exception::Exception;
    };

public:
    Host(size_t peerCount=1, const ENetAddress* address=nullptr);
    Host(const Address& address, size_t peerCount);
    ~Host();

    Peer& connect(const Address& address) noexcept;

    template <typename T>
    Peer& connect(const Address& address, size_t channelCount, T data) noexcept
        { return connect(address, channelCount, static_cast<uint32_t>(data)); }

    Peer& connect(const Address& address, size_t channelCount,
                  uint32_t data=0) noexcept;

    Bandwidth getBandwidthLimit() const noexcept;
    void setBandwidthLimit(const Bandwidth& bandwidth) noexcept;

    size_t getChannelLimit() const noexcept { return host_->channelLimit; }
    void setChannelLimit(size_t limit) noexcept;

    void broadcast(Packet&& packet, uint8_t channelId=0) const noexcept;

    void onConnect(EventCallback callback) noexcept;
    void onDisconnect(EventCallback callback) noexcept;

    void receive(Callback callback, bool once=false) noexcept;
    void service(Callback callback, bool once=false) noexcept;
    void service(time::ms timeout, Callback callback, bool once=false) noexcept;

    void flush();

    template <typename Compressor>
    void setCompression();
    void setCompression(std::nullptr_t) noexcept;
    void setCompression(Compressor::Type);

    // Checksum TODO wrapper

    void onChecksum(decltype(ENetHost::checksum) callback) const noexcept;

    // Intercept TODO wrapper

    void onIntercept(decltype(ENetHost::intercept) callback) const noexcept;

    // Peers

    size_t peerCount() const noexcept { return peers_.size(); }
    const std::vector<Peer>& getPeers() const noexcept { return peers_; }

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

    const Address &getAddress() const noexcept { return address_; }

    uint32_t getTotalReceivedData() const noexcept;
    uint32_t getTotalReceivedPackets() const noexcept;
    uint32_t getTotalSentData() const noexcept;
    uint32_t getTotalSentPackets() const noexcept;

private:
    void parseEvent(const Callback& callback);
    Peer& createPeer(ENetPeer* peer) noexcept;
    Peer& getPeer(ENetPeer* peer) noexcept;
    void removePeer(const Peer& peer) noexcept;

private:
    Address address_;
    std::vector<Peer> peers_;
    EventCallback cbConnect_;
    EventCallback cbDisconnect_;
    std::unique_ptr<ENetHost, Deleter> host_;
    std::unique_ptr<Compressor> compressor_;
    ENetEvent event_;

    static std::atomic<size_t> objects_;
};

template <typename Comp>
void Host::setCompression()
{
    static_assert(std::is_base_of<Compressor, Comp>::value, "Wrong class type");
    compressor_ = std::make_unique<Comp>();
    ENetCompressor compressor{
        compressor_.get(),
        compressor_detail::compress<Comp>,
        compressor_detail::decompress<Comp>,
        nullptr
    };
    enet_host_compress(host_.get(), &compressor);
}

} // \wenet

} // \sq

#endif