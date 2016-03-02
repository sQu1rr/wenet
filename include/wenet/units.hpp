#ifndef SQ_WENET_TIME_HPP
#define SQ_WENET_TIME_HPP

#include "belks/base.hpp"

#include <chrono>

namespace sq {

namespace wenet {

namespace time {

using ms = std::chrono::duration<uint32_t, std::milli>; // TODO literals

} // \time

namespace speed {

using bs = uint32_t; // bytes per second TODO literals

} // \speed

} // \wenet

} // \sq

#endif
