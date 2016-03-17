#include "wenet/wenet.hpp"

#include <iostream>

using namespace sq;
using namespace std;
using namespace sq::wenet;

string descClient(const Peer& peer)
{
    auto addr = peer.getAddress();
    return "["s + addr.getIp() + ":" + to_string(addr.getPort()) + "] "s;
}

string unpack(span<const byte> data) { return {data.begin(), data.end()}; }

int main()
{
    Host server{{1238u}, 32};

    server.onConnect([](Peer&& peer) {
        cout << descClient(peer) << "Connected" << endl;
        peer.setTimeout({1000_ms, 0_ms, 1000_ms});
        peer.setPingInterval(500_ms);
    });

    server.onDisconnect([](Peer&& peer) {
        cout << descClient(peer) << "Disconnected" << endl;
    });

    server.setCompression<sq::wenet::compressor::Zlib>();

    bool work = true;
    while (work) {
        server.service(1000_ms, [&work](Peer&& peer, Packet&& packet) {
            auto data = unpack(packet.getData());
            cout << descClient(peer) << data << endl;
            peer.send(packet);
            if (data == "quit"s) work = false;
        });
    }
}
