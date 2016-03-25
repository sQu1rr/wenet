#ifndef SQ_WENET_ADDRESS_HPP
#define SQ_WENET_ADDRESS_HPP

#include "belks/base.hpp"

#include <enet/enet.h>

namespace sq {

namespace wenet {

class Address {
public:
    class LookupException : public std::runtime_error {
    public: using std::runtime_error::runtime_error;
    };

public:
    Address() noexcept = default;

    explicit Address(uint16_t port) noexcept : Address(ENET_HOST_ANY, port) { }
    Address(uint32_t host, uint16_t port) noexcept;
    Address(cstring_span<> host, uint16_t port);

    explicit Address(const ENetAddress& address) noexcept;
    explicit Address(ENetAddress&& address) noexcept;

    Address& operator = (const ENetAddress& address) noexcept;
    Address& operator = (ENetAddress&& address) noexcept;

    void setHost(cstring_span<> host);
    void setHost(uint32_t host) noexcept { address_.host = host; }
    void setPort(uint16_t port) noexcept { address_.port = port; }

    uint32_t getHost() const noexcept { return address_.host; }
    uint16_t getPort() const noexcept { return address_.port; }
    std::string getHostname() const;
    std::string getIp() const;

    operator const ENetAddress* () const noexcept { return &address_; }

private:
    ENetAddress address_;
};

} // \wenet

} // \sq

#endif
