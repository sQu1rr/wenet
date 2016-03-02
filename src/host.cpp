#include "wenet/host.hpp"

namespace sq {

namespace wenet {

void Host::Deleter::operator () (ENetHost* host) const noexcept
{
    enet_host_destroy(host);
}

std::atomic<size_t> Host::objects_{0};

Host::Host(size_t peerCount, const ENetAddress* address)
{
    if (!objects_++) {
        if (enet_initialize()) {
            throw InitialisationException{"Cannot initialise ENET"};
        }
    }
    peers_.reserve(peerCount);
    host_.reset(enet_host_create(address, peerCount, 0u, 0u, 0u));
}

Host::Host(const Address& address, size_t peerCount) : Host(peerCount, address)
{
    address_ = address;
}

Host::~Host()
{
    if (!--objects_) {
        enet_deinitialize();
    }
}

Peer& Host::connect(const Address& address) noexcept
{
    return connect(address, getChannelLimit());
}

Peer& Host::connect(const Address& address, size_t channelCount,
                    uint32_t data) noexcept
{
    auto peer = enet_host_connect(host_.get(), address, channelCount, data);
    return createPeer(peer);
}

Host::Bandwidth Host::getBandwidthLimit() const noexcept
{
    return {host_->incomingBandwidth, host_->outgoingBandwidth};
}

void Host::setBandwidthLimit(const Bandwidth& bandwidth) noexcept
{
    enet_host_bandwidth_limit(host_.get(), bandwidth.incoming,
                              bandwidth.outgoing);
}

void Host::setChannelLimit(size_t limit) noexcept
{
    enet_host_channel_limit(host_.get(), limit);
}

void Host::broadcast(Packet&& packet, uint8_t channelId) const noexcept
{
    enet_host_broadcast(host_.get(), channelId, packet.releasePacket());
}

void Host::onConnect(EventCallback callback) noexcept
{
    cbConnect_ = std::move(callback);
}

void Host::onDisconnect(EventCallback callback) noexcept
{
    cbDisconnect_ = std::move(callback);
}

void Host::receive(Callback callback, bool once) noexcept
{
    do {
        auto result = enet_host_check_events(host_.get(), &event_);
        if (result > 0) parseEvent(callback);
        else if (result < 0) throw ReceiveEventException{"Cannot receive event"};
        else once = true;
    } while(!once);
}

void Host::service(Callback callback, bool once) noexcept
{
    service(time::ms{0}, callback, once);
}

void Host::service(time::ms timeout, Callback callback, bool once) noexcept
{
    do {
        auto result = enet_host_service(host_.get(), &event_, timeout.count());
        if (result > 0) parseEvent(callback);
        else if (result < 0) throw ReceiveEventException{"Cannot receive event"};
        else once = true;
    } while(!once);
}

void Host::flush()
{
    enet_host_flush(host_.get());
}

void Host::setCompression(std::nullptr_t) noexcept
{
    compressor_.reset();
}

void Host::setCompression(Compressor::Type)
{
    compressor_.reset();
    if (enet_host_compress_with_range_coder(host_.get())) {
        throw RangeCompressorInitException{"Cannot initialise compressor"};
    }
}

void Host::onChecksum(decltype(ENetHost::checksum) callback) const noexcept
{
    host_->checksum = callback;
}

void Host::onIntercept(decltype(ENetHost::intercept) callback) const noexcept
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

void Host::parseEvent(const Callback& callback)
{
    if (event_.type == ENET_EVENT_TYPE_CONNECT) {
        if (cbConnect_) cbConnect_(createPeer(event_.peer), event_.data);
    }
    else {
        auto& peer = getPeer(event_.peer);
        if (event_.type == ENET_EVENT_TYPE_RECEIVE) {
            callback(peer, {*event_.packet}, event_.channelID);
        }
        else {
            if (cbDisconnect_) cbDisconnect_(peer, event_.data);
            removePeer(peer);
        }
    }
}

Peer& Host::createPeer(ENetPeer* peer) noexcept
{
    if (!peer->data) peers_.emplace_back(*peer);
    return getPeer(peer);
}

Peer& Host::getPeer(ENetPeer* peer) noexcept
{
    return *reinterpret_cast<Peer*>(peer->data);
}

void Host::removePeer(const Peer& peer) noexcept
{
    auto it = std::find_if(peers_.begin(), peers_.end(),
            [&peer](const Peer& test) {
        return &test == &peer;
    });
    if (peers_.size() > 1u && &*it != &peers_.back()) {
        std::swap(*it, peers_.back());
    }
    peers_.pop_back();
}

} // \network

} // \sq
