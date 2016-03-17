# Wenet

C++14 [ENet](http://enet.bespin.org) wrapper

# Tutorial

This tutorial is a modifying original
[ENet tutorial](http://enet.bespin.org/Tutorial.html)

## Initialisation

You should include the file &lt;wenet/wenet.hpp&gt; when using Wenet. Do not
include &lt;wenet.hpp&gt; without the directory prefix, as this may cause
file name conflicts on some systems.

No further steps are required, initialisation is handled automatically when host
is created and deinitialistaion happens after last host is destroyed

```cpp
#include <wenet/wenet.hpp>

// for the sake of tutorial:
using namespace sq;
using namespace sq::wenet;
using namespace std;
```

## Creating a Wenet server

Servers in Wenet are constructed with Host{} class. You must specify an address
on which to receive data and new connections, as well as the maximum
allowable numbers of connected peers.

```cpp
Host server{{1234u}, 32};
```

Host creation might throw Host::InitialisationException if either ENet or host
cannot be initialised.

## Creating a Wenet client

Clients in Wenet are similarly constructed with Host{} class, when no address
is specified to bind the host to. The peer count controls the maximum number of connections to other server hosts that may be simultaneously open.

```cpp
Host client{}; // peer count defaults to 1 if no number is given
```

## Host settings (both client and server)

You may optionally specify the incoming and outgoing bandwidth of the server
in bytes per second so that Wenet may try to statically manage bandwidth
resources among connected peers in addition to its dynamic throttling algorithm;
not specifying these two options will cause Wenet to rely entirely upon its
dynamic throttling algorithm to manage bandwidth.

```cpp
host.setBandwidth(/* incoming */, /* outgoing */);
```

You may also limit the maximum number of channels that will be used for
communication.

```cpp
host.setChannelLimit(/* number of channels */);
```

## Managing Wenet host

Wenet uses a polled event model to notify the programmer of significant events.
Wenet hosts are polled for events with host.service(), where an optional
timeout value in milliseconds may be specified to control how long
Wenet will poll; if a timeout of 0 is specified, host.service() will return
immediately if there are no events to dispatch. host.service() will return true
if an event was dispatched within the specified timeout.

Beware that most processing of the network with the Wenet stack is done inside
host.service(). Both hosts that make up the sides of a connection must
regularly call this function to ensure packets are actually sent and received.
A common symptom of not actively calling enet_host_service() on both ends is
that one side receives events while the other does not. The best way to schedule
this activity to ensure adequate service is, for example, to call
host.service() with a 0 timeout (meaning non-blocking) at the beginning of
every frame in a game loop.

Currently there are only three types of significant events in Wenet, and to
intercept them callbacks should be provided.

- onConnect - is called when either a new client host has connected to the
server host or when an attempt to establish a connection with a foreign host
has succeeded. Callback can receive Peer and optional data (uint32_t).

- onDisconnect - is called when a connected peer has either explicitly
disconnected or timed out. Callback can receive Peer and optional data.

- onReceive - is called when a packet is received from a connected peer.
Callback can receive Peer, Packet and channel id.

Any number of arguments can be omitted thanks to
[Convw](https://github.com/sQu1rr/convw)

```cpp
host.onConnect([](Peer&& peer, uint32_t data) {
    auto address = peer.getAddress();
    cout << "A new client connected from: " << address.getIp()
         << ":" << address.getPort() << " with data: " << data;
});
host.onDisconnect([](Peer&& peer, uint32_t data) {
    // user data TODO
    cout << "Disconnected data: " << data << endl;
});
host.onReceive([](Peer&& peer, Packet&& packet, uint8_t channelId) {
    // handle packet
});
host.service(1000_ms); // wait 1000 ms for an event
```

While onConnect and onDisconnect must be specified before servicing the host,
onReceive may also be given to host.service() function instead of defining it
beforehand. This, however affects the performance and thus not recommended.

By default host.service() will handle all pending events before returning, it
can be overriden with optional limit argument

```cpp
// only handle one event at a time
while (true) host.service([](Packet&&) {}, 1);
```

**Be aware**: Peer is invalidated on each call to host.service() and is not
guaranteed to exist thus should not be saved inbetween host.service() calls

## Sending a packet to a Wenet peer

Packets in Wenet are created with Packet{} class, where you may specify size
or intitial data.

Certain flags may also be supplied to Packet{} to control various packet
features:

- Packet::Flag::Reliable specifies that the packet must use reliable delivery.
This is the default flag. A reliable packet is guaranteed to be delivered,
and a number of retry attempts will be made until an acknowledgement is
received from the foreign host the packet is sent to. If a certain number of
retry attempts is reached without any acknowledgement, Wenet will assume the
peer has disconnected and forcefully reset the connection.

- Packet::Flag::Unreliable specifies that packet should be delivered using
unreliable delivery, so no retry attempts will be made nor acknowledgements
generated.

A packet may be resized (extended or truncated) with packet.resize(). Or by
adding additional data using operator <<. For obvious reasons this operator will
throw if tried to use on unmanaged packet.

A packet is sent to a foreign host with peer.send(). peer.send() accepts a
channel id over which to send the packet to a given peer. Once the packet is
handed over to Wenet with peer.send(), Wenet will handle its deallocation
automatically.

One may also use host.broadcast() to send a packet to all connected
peers on a given host over a specified channel id, as with peer.send().

Queued packets will be sent on a call to host.service(). Alternatively,
host.flush() will send out queued packets without dispatching any events.

```cpp
vector<byte> pack(cstring_span<> data) { return {data.begin(), data.end()}; }

string data = "packet";
Packet packet{pack(data)};

data = "packetfoo";
packet = pack(data); // will resize and copy

peer.send(packet);
host.flush();
```

## Disconnecting Wenet peer

Peers may be gently disconnected with peer.disconnect().
A disconnect request will be sent to the foreign host, and Wenet will wait
for an acknowledgement from the foreign host before finally disconnecting.
onDisconnect callback will be invoked once the disconnection succeeds.
Normally timeouts apply to the disconnect acknowledgement, and so if no
acknowledgement is received after a length of time the peer will be
forcefully disconnected.

peer.reset() will forcefully disconnect a peer. The foreign host will get
no notification of a disconnect and will time out on the foreign host.
No event is generated.

## Connecting to Wenet host

A connection to a foreign host is initiated with host.connect().
It accepts the address of a foreign host to connect to, and the number
of channels that should be allocated for communication. If N channels are
allocated for use, their channel ids will be numbered 0 through N-1.
If connection does not succeed a Host::InitialisationException is thrown.
When the connection attempt succeeds, onConnect will be invoked.
If the connection attempt times out or otherwise fails onDisconnect callback
will be called.

```cpp
auto hostname = "localhost"s;
auto peer = host.connect({hostname, 1238u});

host.onConnect([] { cout << "Success" << endl; });
host.onDisconnect([&peer] { peer.reset(); cout << "Fail" << endl; });
```

# Sample echo server and client

## Client

```cpp
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
            server.send({data});
        }
    }
}
```

## Server

```cpp
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
```
