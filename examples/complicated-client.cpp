#include "wenet/wenet.hpp"

#include <iostream>

using namespace sq;
using namespace std;
using namespace sq::wenet;

vector<byte> pack(cstring_span<> data) { return {data.begin(), data.end()}; }
string unpack(span<const byte> data) { return {data.begin(), data.end()}; }

int main()
{
    Host client{};

    client.onConnect([] { cout << "Connected" << endl; });
    client.onDisconnect([] { cout << "Disconnected" << endl; });

    client.setCompression<sq::wenet::compressor::Zlib>();

    auto hostname = "localhost"s;
    auto server = client.connect({hostname, 1238u});
    server.setTimeout({1000_ms, 0_ms, 1000_ms});
    server.setPingInterval(500_ms);

    bool work = true;
    while (work) {
        client.service(100_ms, [&work](Packet&& packet) {
            auto data = unpack(packet.getData());
            if (data == "quit"s) work = false;
            cout << "[server] " << data << endl;
        });
        if (work) {
            string str;
            std::cin >> str;
            auto data = pack(str);
            Packet packet{data};
            packet.onDestroy([&hostname](Packet&& packet) {
                cout << hostname << " destroying packet" << endl;
            });
            server.send({data});
        }
    }
}
