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

string unpack(span<const byte> data) {
    return {data.begin(), data.end()};
}

int main()
{
    Host server{{1238u}, 32};

    server.onConnect([](Peer& peer) {
        cout << descClient(peer) << "Connected" << endl;
    });

    server.onDisconnect([](Peer& peer) {
        cout << descClient(peer) << "Disconnected" << endl;
    });

    bool work = true;
    while (work) {
        server.service(time::ms{1000}, [&work](Peer& peer, Packet&& packet) {
            auto data = unpack(packet.getData());
            cout << descClient(peer) << data << endl;
            peer.send(std::move(packet));
            if (data == "quit"s) work = false;
        });
    }
}