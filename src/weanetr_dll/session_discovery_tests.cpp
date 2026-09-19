#include "session_discovery.h"
#include <cassert>
#include <cstring>
#include <cwchar>

/** Discovery must retain reachable endpoints without changing the host's session metadata. */
int main() {
    net::MLDPLAY_SESSIONDESC advertised;
    advertised.guidInstance.Data1 = 1;
    advertised.guidApplication.Data1 = 2;
    advertised.flags = 0x200;
    advertised.currentPlayers = 1;
    advertised.totalMaxPlayers = 2;
    advertised.fileHashsum = 1234;
    advertised.sock = {0x3412, 123, 0x1401A8C0}; // 192.168.1.20, advertised port 0x1234
    std::wcscpy(advertised.gameName, L"Co-op campaign");

    // Both the address and port may be translated by the router.
    const net::MySocket publicSource = {0x7856, 456, 0x0A7100CB}; // 203.0.113.10
    const auto discovered = net::discoveredSession(advertised, publicSource);
    assert(discovered.sock.ipv4 == publicSource.ipv4);
    assert(discovered.sock.portBe == publicSource.portBe);
    auto restored = discovered;
    restored.sock.ipv4 = advertised.sock.ipv4;
    restored.sock.portBe = advertised.sock.portBe;
    assert(std::memcmp(&restored, &advertised, sizeof(advertised)) == 0);

    // LAN discovery and refreshed metadata must keep the same session identity.
    const auto lan = net::discoveredSession(advertised, advertised.sock);
    assert(lan.sock.ipv4 == advertised.sock.ipv4 && lan.sock.portBe == advertised.sock.portBe);
    assert(net::sameDiscoveredSession(lan, discovered));
    auto another = discovered;
    another.guidInstance.Data1 = 3;
    another.sock.portBe = 0x7956;
    assert(!net::sameDiscoveredSession(discovered, another));
    another.sock.portBe = discovered.sock.portBe;
    assert(net::sameDiscoveredSession(discovered, another));
}
