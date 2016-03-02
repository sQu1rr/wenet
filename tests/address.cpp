#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "wenet/address.hpp"

namespace sq {

namespace wenet {

SCENARIO( "Specification Testing", "[wenet][address][specs]" ) {
    Address address; // empty constructor present

    WHEN( "Port constructor" ) {
        Address address(1234u);
        THEN( "Port is correctly set" ) {
            REQUIRE( address.getPort() == 1234u );
        }
    }
    WHEN( "Host & Port constructor" ) {
        Address address(4321u, 1234u);
        THEN( "Host & Port is correctly set" ) {
            REQUIRE( address.getHost() == 4321u );
            REQUIRE( address.getPort() == 1234u );
        }
    }

    const auto localhostName = "localhost"s;
    const auto localhostIp = "127.0.0.1"s;

    WHEN( "Named host constructor" ) {
        Address address(localhostName, 1234u);
        THEN( "Host & Port are correctly set" ) {
            REQUIRE( address.getHost() == 16777343 );
            REQUIRE( address.getPort() == 1234u );
            REQUIRE( address.getHostname() == localhostName );
            REQUIRE( address.getIp() == localhostIp );
        }
    }

    WHEN( "Host & Port are changed" ) {
        address.setHost(localhostIp);
        address.setPort(1234u);
        THEN( "Host & Port are correctly set" ) {
            REQUIRE( address.getHost() == 16777343 );
            REQUIRE( address.getPort() == 1234u );
        }
    }
}

SCENARIO( "Exception Testing", "[wenet][address][throw]" ) {
    Address address;
    const auto incorrectHost = "_"s;

    WHEN( "Supplying with incorrect hostname" ) {
        REQUIRE_THROWS_AS( address.setHost(incorrectHost),
                           Address::LookupException );
    }
    WHEN( "Supplying with incorrect hostname raw" ) {
        address.setHost(-1u);
        THEN( "Throws when trying to lookup host IP" ) {
            REQUIRE_THROWS_AS( address.getIp(), Address::LookupException );
        }
    }
}

} // \wenet

} // \sq
