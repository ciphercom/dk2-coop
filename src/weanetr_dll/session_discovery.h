#pragma once

#include <WinSock2.h>
#include "structs.h"

namespace net {

/** Keep session metadata, but join through the endpoint reachable by this client. */
inline MLDPLAY_SESSIONDESC discoveredSession(const MLDPLAY_SESSIONDESC &advertised, const MySocket &source) {
    auto discovered = advertised;
    discovered.sock.ipv4 = source.ipv4;
    discovered.sock.portBe = source.portBe;
    return discovered;
}

/** A router can expose multiple game sessions at one IP through different ports. */
inline bool sameDiscoveredSession(const MLDPLAY_SESSIONDESC &left, const MLDPLAY_SESSIONDESC &right) {
    return left.guidInstance == right.guidInstance ||
        (left.sock.ipv4 == right.sock.ipv4 && left.sock.portBe == right.sock.portBe);
}

}
