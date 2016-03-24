#include "wenet/wenet.hpp"

#include <array>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>
#include <atomic>
#include <iomanip>

#include "belks/io.hpp"

using namespace sq;
using namespace sq::belks;
using namespace sq::wenet;

constexpr auto port = 1238u;

template <typename T>
class ServerBase {
public:
    ServerBase(uint n, uint t) : size_(n), times_(t) { }

    void stop() { work_ = false; }

    void work()
    {
        using namespace std::chrono;

        in_ = 0;
        count_ = 0;
        const auto time = high_resolution_clock::now();
        while (work_) static_cast<T*>(this)->service();
        const auto now = high_resolution_clock::now();
        const auto diff = duration_cast<milliseconds>(now - time).count();

        if (diff) in_ /= diff;
    }


    void print()
    {
        const auto n = size_;
        const auto loss = (float(times_) * n - count_) / (times_ * n) * 100;
        std::cout << std::setw(12) << sizeToString<SizeType::b>(in_) + "/s"
                  << std::setw(12) << sizeToString<SizeType::b>(in_ / n) + "/s"
                  << std::setw(12) << std::to_string(loss) + "%";
    }

protected:
    uint in_ = 0;
    uint count_ = 0;

    uint size_;
    uint times_;

    bool work_ = true;
};

template <typename T>
class ClientBase {
public:
    ClientBase(uint s, uint t) : size_(s), times_(t) { }

    void work()
    {
        std::vector<byte> v; v.reserve(size_);
        for (auto i = 0u; i < times_; ++i) {
            static_cast<T*>(this)->service();

            v.clear(); v.resize(size_, i);
            static_cast<T*>(this)->send(v);
        }
        static_cast<T*>(this)->disconnect();
    }

protected:
    uint size_;
    uint times_;
};

class WenetServer : public ServerBase<WenetServer> {
public:
    WenetServer(uint n, uint t) : ServerBase(n, t), host_({port}, n)
    {
        host_.onReceive([this](Peer& peer, Packet&& packet) {
            in_ += packet.getSize();
            count_++;
            peer.send(std::move(packet));
        });
    }

    void service() { host_.service(); }

private:
    Host host_;
};

class WenetClient : public ClientBase<WenetClient> {
public:
    WenetClient(uint s, uint t) : ClientBase(s, t), peer_(host_)
    {
        peer_ = *host_.connect({"localhost", port});
    }

    void service() { host_.service(); }
    void send(const std::vector<byte>& v) { peer_.send({v}); }
    void disconnect() { peer_.disconnect(); host_.flush(); }

private:
    Host host_;
    Peer peer_;
};


class EnetServer : public ServerBase<EnetServer> {
public:
    EnetServer(uint n, uint t) : ServerBase(n, t)
    {
        enet_initialize();

        ENetAddress address;

        address.host = ENET_HOST_ANY;
        address.port = 1238;

        host_ = enet_host_create(&address, n, 0, 0, 0);
    }

    ~EnetServer() { enet_host_destroy(host_); enet_deinitialize(); }

    void service()
    {
        ENetEvent event;
        while (enet_host_service(host_, &event, 0)) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                in_ += event.packet->dataLength;
                count_++;
                enet_peer_send(event.peer, 0, event.packet);
            }
        }
    }

private:
    ENetHost* host_ = nullptr;
};

class EnetClient : public ClientBase<EnetClient> {
public:
    EnetClient(uint s, uint t) : ClientBase(s, t)
    {
        host_ = enet_host_create(nullptr, 1, 0, 0, 0);

        ENetAddress address;
        enet_address_set_host(&address, "localhost");
        address.port = 1238;

        peer_ = enet_host_connect(host_, &address, 0, 0);
    }

    ~EnetClient() { enet_peer_reset(peer_); enet_host_destroy(host_); }

    void service()
    {
        ENetEvent event;
        while (enet_host_service(host_, &event, 0));
    }

    void send(const std::vector<byte>& v)
    {
        ENetPacket* packet = enet_packet_create(&v[0], size_,
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer_, 0, packet);
    }

    void disconnect()
    {
        enet_peer_disconnect(peer_, 0);
        enet_host_flush(host_);
    }

private:
    ENetHost* host_;
    ENetPeer* peer_;
};


template <typename Server, typename Client>
void test(uint n, uint s, uint t)
{
    Server server{n, t};
    auto serverThread = std::thread([&server] { server.work(); });
    
    std::vector<std::thread> clientThreads;
    for (auto i = 0u; i < n; ++i) {
        clientThreads.emplace_back([s, t] {
            Client client{s, t};
            client.work();
        });
    }
    
    for (auto& thread : clientThreads) thread.join();

    server.stop();
    serverThread.join();
    server.print();
}

int main()
{
    constexpr auto N = 1u << 9;
    constexpr auto T = N << 5;
    constexpr auto S = 1024u * 32u;

    std::cout << std::setw(12) << "Clients"
              << std::setw(12) << "Speed"
              << std::setw(12) << "Single"
              << std::setw(12) << "Loss"
              << "   |   "
              << std::setw(12) << "Speed"
              << std::setw(12) << "Single"
              << std::setw(12) << "Loss"
              << std::endl;

    for (auto i = 1u, d = 1u; i <= N; i <<= 1, d++) {
        std::cout << std::setw(12) << i;
        test<WenetServer, WenetClient>(i, S / d, T / d);
        std::cout << "   |   ";
        test<EnetServer, EnetClient>(i, S / d, T / d);
        std::cout << std::endl;
    }
}
