#include "wenet/wenet.hpp"

#include <iostream>

using namespace sq;
using namespace std;
using namespace sq::wenet;

string descClient(const Peer& peer)
{
    auto addr = peer.getAddress();
    auto name = "["s + addr.getIp() + ":" + to_string(addr.getPort()) + "]";
    return name + "#" + std::to_string(peer.getId()) + " ";
}

string unpack(span<const byte> data) { return {data.begin(), data.end()}; }

int main()
{
    Host server{{1238u}, 32};

    server.onConnect([](Peer& peer) {
        cout << descClient(peer) << "Connected" << endl;
        peer.setTimeout({1000_ms, 0_ms, 1000_ms});
        peer.setPingInterval(500_ms);
    });

    server.onDisconnect([](size_t peerId) {
        cout << "#" << peerId << " Disconnected" << endl;
    });

    server.setCompression<sq::wenet::compressor::Zlib>();

    bool work = true;
    server.onReceive([&work](Peer& peer, Packet&& packet) {
        auto data = unpack(packet.getData());
        cout << descClient(peer) << data << endl;
        peer.send(packet);
        if (data == "quit"s) work = false;
    });

    while (work) server.service(1000_ms);
}
