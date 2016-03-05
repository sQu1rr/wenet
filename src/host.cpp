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
    host_.reset(enet_host_create(address, peerCount, 0u, 0u, 0u));
    if (!host_) throw InitialisationException{"Cannot initialise host"};
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

Peer Host::connect(const Address& address) noexcept
{
    return connect(address, getChannelLimit());
}

Peer Host::connect(const Address& address, size_t channelCount,
                    uint32_t data) noexcept
{
    auto peer = enet_host_connect(host_.get(), address, channelCount, data);
    if (!peer) throw InitialisationException{"Cannot connect to peer"};
    return {*peer};
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

void Host::onReceive(Callback callback) noexcept
{
    cbReceive_ = std::move(callback);
}

void Host::onConnect(EventCallback callback) noexcept
{
    cbConnect_ = std::move(callback);
}

void Host::onDisconnect(EventCallback callback) noexcept
{
    cbDisconnect_ = std::move(callback);
}

bool Host::receive(int limit) noexcept
{
    do {
        auto result = enet_host_check_events(host_.get(), &event_);
        if (result > 0) parseEvent();
        else if (result < 0) throw ReceiveEventException{"Cannot receive event"};
        else return false;
    } while(--limit);
    return true;
}

bool Host::receive(Callback callback, int limit) noexcept
{
    onReceive(callback);
    return receive(limit);
}

bool Host::service(int limit) noexcept
{
    return service(time::ms{0}, limit);
}

bool Host::service(Callback callback, int limit) noexcept
{
    onReceive(callback);
    return service(limit);
}

bool Host::service(time::ms timeout, int limit) noexcept
{
    do {
        auto result = enet_host_service(host_.get(), &event_, timeout.count());
        if (result > 0) parseEvent();
        else if (result < 0) throw ReceiveEventException{"Cannot receive event"};
        else return false;
    } while(--limit);
    return true;
}

bool Host::service(time::ms timeout, Callback callback, int limit) noexcept
{
    onReceive(callback);
    return service(timeout, limit);
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

std::vector<Peer> Host::getPeers() const noexcept
{
    std::vector<Peer> peers; peers.reserve(peerCount());
    for (auto i = 0u; i < peerCount(); ++i) {
        peers.emplace_back(host_->peers[i]);
    }
    return peers;
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

void Host::parseEvent()
{
    if (event_.type == ENET_EVENT_TYPE_CONNECT) {
        if (cbConnect_) cbConnect_({*event_.peer}, event_.data);
    }
    else {
        if (event_.type == ENET_EVENT_TYPE_RECEIVE) {
            cbReceive_({*event_.peer}, {*event_.packet}, event_.channelID);
        }
        else {
            if (cbDisconnect_) cbDisconnect_({*event_.peer}, event_.data);
        }
    }
}

} // \network

} // \sq
