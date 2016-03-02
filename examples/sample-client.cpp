#include "wenet/wenet.hpp"

#include <iostream>

using namespace sq;
using namespace std;
using namespace sq::wenet;

vector<byte> pack(cstring_span<> data) {
    return {data.begin(), data.end()};
}

string unpack(span<const byte> data) {
    return {data.begin(), data.end()};
}

int main()
{
    Host client{};

    client.onConnect([] {
        cout << "Connected" << endl;
    });

    client.onDisconnect([] {
        cout << "Disconnected" << endl;
    });

    auto hostname = "localhost"s;
    auto& server = client.connect({hostname, 1238u});

    bool work = true;
    while (work) {
        client.service(time::ms{100}, [&work](Packet&& packet) {
            auto data = unpack(packet.getData());
            if (data == "quit"s) work = false;
            cout << "[server] " << data << endl;
        });
        string str;
        std::cin >> str;
        auto data = pack(str);
        server.send({data});
    }
}