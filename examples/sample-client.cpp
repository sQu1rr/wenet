#include "wenet/wenet.hpp"

#include <iostream>

using namespace sq;
using namespace std;
using namespace sq::wenet;

vector<byte> pack(cstring_span<> data) { return {data.begin(), data.end()}; }
string unpack(span<const byte> data) { return {data.begin(), data.end()}; }

int main()
{
    const auto hostname = "localhost"s;
    const auto port = 1238u;

    // Create Client
    Host client{};
    client.setCompression<sq::wenet::compressor::Zlib>();

    // Connect
    client.onConnect([] { cout << "Connected" << endl; });

    // Disconnect
    client.onDisconnect([] { cout << "Disconnected" << endl; });

    // Receive
    bool work = true;
    client.onReceive([&work](Packet&& packet) {
        auto data = unpack(packet.getData());
        if (data == "quit"s) work = false;
        cout << "[server] " << data << endl;
    });

    // Connect to server
    auto server = client.connect({hostname, port});
    server.setTimeout({1000_ms, 0_ms, 1000_ms});
    server.setPingInterval(500_ms);

    while (work) {
        // Work
        client.service(100_ms);

        // Ask for input
        if (work) {
            string str;
            std::cin >> str;
            auto data = pack(str);
            server.send({data});
        }
        // since input wait stops "ping events" server will disconnect client
        // after 1s timeout
    }
}
