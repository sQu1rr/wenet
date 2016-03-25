#include "wenet/address.hpp"

namespace sq {

namespace wenet {

// http://pubs.opengroup.org/onlinepubs/7908799/xns/gethostname.html
constexpr auto MAX_HOSTNAME = 255;
constexpr auto MAX_IP = 15; // IPv4

Address::Address(uint32_t host, uint16_t port) noexcept
{
    setPort(port);
    setHost(host);
}

Address::Address(cstring_span<> host, uint16_t port)
{
    setPort(port);
    setHost(host);
}

Address::Address(const ENetAddress& address) noexcept : address_(address) { }
Address::Address(ENetAddress&& address) noexcept : address_(address) { }

Address& Address::operator = (const ENetAddress& address) noexcept
{
    address_ = address;
    return *this;
}

Address& Address::operator = (ENetAddress&& address) noexcept
{
    address_ = address;
    return *this;
}

void Address::setHost(cstring_span<> hostname)
{
    if (enet_address_set_host(&address_, gsl::to_string(hostname).c_str())) {
        throw LookupException{"Failed to lookup hostname"};
    }
}

std::string Address::getHostname() const
{
    char buffer[MAX_HOSTNAME + 1];
    if (enet_address_get_host(&address_, buffer, MAX_HOSTNAME)) {
        throw LookupException{"Impossible to lookup hostname"};
    }
    return {buffer};
}

std::string Address::getIp() const
{
    char buffer[MAX_IP + 1];
    if (enet_address_get_host_ip(&address_, buffer, MAX_IP)) {
        throw LookupException{"Failed to get IP"};
    }
    return {buffer};
}

} // \wenet

} // \sq
