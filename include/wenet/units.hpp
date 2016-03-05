#ifndef SQ_WENET_TIME_HPP
#define SQ_WENET_TIME_HPP

#include "belks/base.hpp"

#include <chrono>

namespace sq {

namespace wenet {

namespace time {

using ms = std::chrono::duration<uint32_t, std::milli>;

} // \time

namespace speed {

using bs = uint32_t;

} // \speed

constexpr time::ms operator "" _ms(unsigned long long ms) {
    return time::ms{ms};
}

} // \wenet

} // \sq

#endif
