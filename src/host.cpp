#include "wenet/host.hpp"

namespace sq {

namespace wenet {

void Host::Deleter::operator () (ENetHost* host) const noexcept
{
    hosts_.erase(host);
    enet_host_destroy(host);

    if (!--objects_) enet_deinitialize();
}

std::atomic<size_t> Host::objects_{0};
std::unordered_map<ENetHost*, Host*> Host::hosts_;

Host::Host(size_t peerCount, const ENetAddress* address)
{
    if (!objects_++) {
        if (enet_initialize()) {
            throw InitialisationException{"Cannot initialise ENET"};
        }
    }
    host_.reset(enet_host_create(address, peerCount, 0u, 0u, 0u));
    if (!host_) throw InitialisationException{"Cannot initialise host"};

    hosts_[host_.get()] = this;
}

Host::Host(const Address& address, size_t peerCount) : Host(peerCount, address)
{
    address_ = address;
}

Peer Host::connect(const Address& address) noexcept
{
    return connect(address, getChannelLimit());
}

Peer Host::connect(const Address& address, size_t channelCount,
                    uint32_t data) noexcept
{
    auto peer = enet_host_connect(host_.get(), address, channelCount, data);
    if (!peer) throw InitialisationException{"Cannot connect to peer"};
    return createPeer(*peer);
}

Host::Bandwidth Host::getBandwidth() const noexcept
{
    return {host_->incomingBandwidth, host_->outgoingBandwidth};
}

void Host::setBandwidth(const Bandwidth& bandwidth) noexcept
{
    enet_host_bandwidth_limit(host_.get(), bandwidth.incoming,
                              bandwidth.outgoing);
}

void Host::setChannelLimit(size_t limit) noexcept
{
    enet_host_channel_limit(host_.get(), limit);
}

void Host::broadcast(Packet& packet, uint8_t channelId) const noexcept
{
    if (packet.isOwned()) packet.releaseOwnership();
    enet_host_broadcast(host_.get(), channelId, packet);
}

void Host::broadcast(Packet&& packet, uint8_t channelId) const noexcept
{
    packet.releaseOwnership();
    enet_host_broadcast(host_.get(), channelId, packet);
}

void Host::onReceive(Callback callback) noexcept
{
    cbReceive_ = std::move(callback);
}

void Host::onConnect(ConnectCallback callback) noexcept
{
    cbConnect_ = std::move(callback);
}

void Host::onDisconnect(DisconnectCallback callback) noexcept
{
    cbDisconnect_ = std::move(callback);
}

bool Host::receive(int limit) noexcept
{
    ENetEvent event;
    do {
        auto result = enet_host_check_events(host_.get(), &event);
        if (result > 0) parseEvent(event);
        else if (result < 0) throw ReceiveEventException{"Cannot receive"};
        else return false;
    } while(--limit);
    return true;
}

bool Host::service(int limit) noexcept
{
    return service(time::ms{0}, limit);
}

bool Host::service(time::ms timeout, int limit) noexcept
{
    ENetEvent event;
    do {
        auto result = enet_host_service(host_.get(), &event, timeout.count());
        if (result > 0) parseEvent(event);
        else if (result < 0) throw ReceiveEventException{"Cannot receive"};
        else return false;
    } while(--limit);
    return true;
}

void Host::flush()
{
    enet_host_flush(host_.get());
}

void Host::disableCompression() noexcept
{
    compressor_.reset();
    enet_host_compress(host_.get(), nullptr);
}

void Host::_onChecksum(decltype(ENetHost::checksum) callback) const noexcept
{
    host_->checksum = callback;
}

void Host::_onIntercept(decltype(ENetHost::intercept) callback) const noexcept
{
    host_->intercept = callback;
}

#if ENET_VERSION_CREATE(1, 3, 9) <= ENET_VERSION

size_t getDuplicatePeers() const noexcept
{
    return host_->duplicatePeers;
}

void setDuplicatePeers(size_t value) const noexcept
{
    host_->duplicatePeers = value;
}

#endif

#if ENET_VERSION_CREATE(1, 3, 12) <= ENET_VERSION

size_t getMaximumPacketSize() const noexcept
{
    return host_->maximumPacketSize;
}

void setMaximumPacketSize(size_t value) const noexcept
{
    host_->maximumPacketSize = value;
}

size_t getMaximumWaitingData() const noexcept
{
    return host_->maximumWaitingData;
}

void setMaximumWaitingData(size_t value) const noexcept
{
    host_->maximumWaitingData = value;
}

#endif

uint32_t Host::getTotalReceivedData() const noexcept
{
    return host_->totalReceivedData;
}

uint32_t Host::getTotalReceivedPackets() const noexcept
{
    return host_->totalReceivedPackets;
}

uint32_t Host::getTotalSentData() const noexcept
{
    return host_->totalSentData;
}

uint32_t Host::getTotalSentPackets() const noexcept
{
    return host_->totalSentPackets;
}

void Host::staticRemovePeer(const Peer& peer) noexcept
{
    const auto ptr = static_cast<ENetPeer*>(peer);
    retrieve(ptr->host).removePeer(*ptr);
}

void Host::parseEvent(ENetEvent& event)
{
    auto peer = event.peer;
    if (event.type == ENET_EVENT_TYPE_CONNECT) {
        // Connect
        if (cbConnect_) cbConnect_(createPeer(*peer), event.data);
    }
    else {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            // Receive
            cbReceive_(getPeer(*peer), {*event.packet}, event.channelID);
        }
        else {
            // Disconnect
            if (cbDisconnect_) {
                cbDisconnect_(getPeer(*peer).getId(), event.data);
            }
            removePeer(*peer);
        }
    }
}

Peer& Host::getPeer(ENetPeer& peer) noexcept
{
    return peers_[size_t(peer.data)];
}

Peer& Host::createPeer(ENetPeer& peer) noexcept
{
    peer.data = reinterpret_cast<void*>(peers_.size());
    peers_.emplace_back(peer);
    return peers_.back();
}

void Host::removePeer(ENetPeer& peer) noexcept
{
    auto index = size_t(peer.data);

    std::swap(peers_[index], peers_.back());

    peers_.pop_back();
    peer.data = nullptr;
}

} // \network

} // \sq
