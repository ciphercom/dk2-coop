#include "network_hands.h"
#include <cstdio>

namespace {
void require(bool valid, const char *message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}
}

/** Observable hand contents and drop order must agree after interleaved synchronized commands. */
int main() {
    using namespace patch::network_hands;
    require(isLocalKeeper(1, 1) && isLocalKeeper(0x12340001, 1) && isLocalKeeper(int(0xFFFF0001u), 1),
        "3D preview queries must identify the local Keeper despite unused register bits");
    require(!isLocalKeeper(0x12340002, 1), "normalizing a preview query must not match another Keeper");
    Ownership state;
    const auto a = originForSlot(0), b = originForSlot(1);
    uint16_t tags[64] = {11, 12, 13, 14};
    state.accepted(11, a); state.accepted(12, b);
    state.accepted(13, a); state.accepted(14, b);
    require(state.count(tags, 4, a) == 2 && state.count(tags, 4, b) == 2, "each hand shows only its own pickups");
    require(state.at(tags, 4, a, 0) == 11 && state.at(tags, 4, a, 1) == 13 && !state.at(tags, 4, a, 2),
        "display indexing must exclude the partner's entries");
    unsigned size = 4;
    auto drop = [&](uint8_t origin) {
        const int index = state.latest(tags, size, origin);
        if (index < 0) return uint16_t(0);
        Ownership::promote(tags, size, unsigned(index));
        return tags[--size];
    };
    require(drop(a) == 13 && drop(a) == 11, "A must drop in LIFO order even while B owns the global top");
    require(!drop(a) && size == 2 && tags[0] == 12 && tags[1] == 14, "empty A must leave B's contents untouched");
    require(drop(b) == 14 && drop(b) == 12 && !drop(b), "B's LIFO order must survive A's interior removals");

    // Rejected requests do not call accepted(); a later reuse must replace stale ownership.
    state.accepted(11, a);
    require(!state.visible(11, b), "a competing failed pickup must not be visible to B");
    state.accepted(11, b);
    require(state.visible(11, b) && !state.visible(11, a), "a dropped and repicked creature must change hands");
    state.accepted(12, 0);
    require(state.visible(12, a) && state.visible(12, b), "native scripted pickups retain shared access");
    for (unsigned i = 0; i < 64; ++i) { tags[i] = uint16_t(i + 1); state.accepted(tags[i], i % 2 ? a : b); }
    require(state.count(tags, 64, a) == 32 && state.count(tags, 64, b) == 32, "partitioning preserves the full native capacity");
    const auto saved = state.save(tags, 64);
    const auto checksum = state.checksum(42, tags, 64);
    Ownership remote;
    require(remote.load(tags, 64, saved) && remote.latest(tags, 64, a) == 63 && remote.latest(tags, 64, b) == 62,
        "network world transfer must restore both hand orders");
    require(remote.checksum(42, tags, 64) == checksum, "transferred ownership must reproduce the shared checksum");
    remote.accepted(64, b);
    require(remote.checksum(42, tags, 64) != checksum, "ownership divergence must be detected before a wrong drop");
    require(remote.load(tags, 64, saved) && remote.checksum(42, tags, 64) == checksum, "rollback must restore the earlier owners");
    auto invalid = saved;
    invalid.slots[63] = 9;
    require(!remote.load(tags, 64, invalid) && remote.checksum(42, tags, 64) == checksum, "invalid owner data must fail without partial mutation");
    invalid = saved; invalid.version = 0;
    require(!remote.load(tags, 64, invalid), "an incompatible ownership format must be rejected");
    require(completingOrigin(1, 11, a, 11, 58, b) == a,
        "finishing A's delayed pickup during B's command must retain A's origin");
    require(completingOrigin(1, 11, a, 12, 58, b) == b,
        "B's direct batch pickup must use B's origin even with A's other pickup pending");
    require(completingOrigin(0, 0, 0, 11, 0, 0) == 0, "native pickups must not invent a Controller");
    state.reset();
    require(state.owner(11) == 0, "a new session must not inherit prior owners");
    for (int slot = 0; slot < 8; ++slot) {
        const auto origin = originForSlot(slot);
        const uint32_t batch = 0x0FFF0ABCu;
        const auto tagged = tagPickup(batch, origin);
        require(pickupOrigin(tagged) == origin && (tagged & ~0xF000u) == batch,
            "batch pickup origin must preserve both packed creature tags");
        const auto dropData = tagDrop(0xFFFF0FFFu, origin);
        require(dropOrigin(dropData) == origin && (dropData & 0xFFFF) == 4095 && ((dropData >> 16) & 0x7FF) == 2047,
            "drop origin must preserve the target tag and native masked direction");
    }
    std::puts("network_hands_tests passed");
}
