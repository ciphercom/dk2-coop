#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>

namespace dk2 { struct CPlayer; struct GameAction; }

/** Controller ownership partitions the native Keeper stack without changing its capacity. */
namespace patch::network_hands {
inline void invariant(bool valid) { if (!valid) std::abort(); }
inline bool pickup(int kind) { return kind >= 57 && kind <= 60; }
/** Shared Keeper occupancy protects the possessed creature regardless of which Controller picks up. */
inline bool possessionBlocksPickup(bool coop, uint16_t possessed, uint16_t target) {
    return coop && possessed != 0 && possessed == target;
}
inline uint8_t originForSlot(int slot) { invariant(slot >= 0 && slot < 8); return uint8_t(slot + 1); }

/** Native UI calls may leave register debris above the 16-bit Keeper tag. */
inline bool isLocalKeeper(int tag, uint16_t localTag) { return uint16_t(tag) == localTag; }

/** Scene tags use 12 bits; drops use an 11-bit direction. Preserve every native payload bit. */
inline uint32_t tagPickup(uint32_t data, uint8_t origin) {
    invariant(origin >= 1 && origin <= 8 && !(data & 0xF000));
    return data | (uint32_t(origin) << 12);
}
inline uint32_t tagDrop(uint32_t data, uint8_t origin) {
    invariant(origin >= 1 && origin <= 8);
    return (data & 0x07FFFFFFu) | (uint32_t(origin) << 28);
}
inline uint8_t pickupOrigin(uint32_t data) { return uint8_t((data >> 12) & 15); }
inline uint8_t dropOrigin(uint32_t data) { return uint8_t(data >> 28); }

/** Completion can belong to the old queued action while a different Controller dispatches a new one. */
inline uint8_t completingOrigin(int type, uint16_t target, int queuedOrigin, uint16_t tag,
        int incomingKind, uint8_t incomingOrigin) {
    if (type >= 1 && type <= 3 && target == tag) {
        invariant(queuedOrigin >= 0 && queuedOrigin <= 8);
        return uint8_t(queuedOrigin);
    }
    return incomingKind == 58 ? incomingOrigin : uint8_t(0);
}

/** Stored after the native Keeper record so automatic rollback and peer world transfers include ownership. */
struct SavedOwners {
    uint32_t version = 0x31444E48; // HND1
    std::array<uint8_t, 64> slots{};
};
static_assert(sizeof(SavedOwners) == 68);

/** Only successful native insertion assigns ownership; failed competing pickups cannot steal it. */
class Ownership {
    std::array<uint8_t, 4096> owners_{};
public:
    void reset() { owners_.fill(0); }
    uint8_t owner(uint16_t tag) const { invariant(tag && tag < owners_.size()); return owners_[tag]; }
    void accepted(uint16_t tag, uint8_t origin) {
        invariant(tag && tag < owners_.size() && origin <= 8);
        owners_[tag] = origin;
    }
    // Origin zero is a native/scripted pickup, available to either Controller as before.
    bool visible(uint16_t tag, uint8_t origin) const { return owner(tag) == 0 || owner(tag) == origin; }
    int count(const uint16_t *tags, unsigned size, uint8_t origin) const {
        invariant(size <= 64);
        int result = 0;
        for (unsigned i = 0; i < size; ++i) if (visible(tags[i], origin)) ++result;
        return result;
    }
    int latest(const uint16_t *tags, unsigned size, uint8_t origin) const {
        invariant(size <= 64 && origin <= 8);
        for (int i = int(size) - 1; i >= 0; --i) if (visible(tags[i], origin)) return i;
        return -1;
    }
    uint16_t at(const uint16_t *tags, unsigned size, uint8_t origin, unsigned index) const {
        invariant(size <= 64);
        for (unsigned i = 0; i < size; ++i) if (visible(tags[i], origin) && index-- == 0) return tags[i];
        return 0;
    }
    SavedOwners save(const uint16_t *tags, unsigned size) const {
        invariant(size <= 64);
        SavedOwners saved;
        for (unsigned i = 0; i < size; ++i) saved.slots[i] = owner(tags[i]);
        return saved;
    }
    /** Validate the complete external record before applying it. */
    bool load(const uint16_t *tags, unsigned size, const SavedOwners &saved) {
        if (saved.version != 0x31444E48 || size > 64) return false;
        for (unsigned i = 0; i < size; ++i)
            if (!tags[i] || tags[i] >= 4096 || saved.slots[i] > 8) return false;
        for (unsigned i = 0; i < size; ++i) accepted(tags[i], saved.slots[i]);
        return true;
    }
    uint32_t checksum(uint32_t native, const uint16_t *tags, unsigned size) const {
        invariant(size <= 64);
        for (unsigned i = 0; i < size; ++i) native = native * 16777619u ^ owner(tags[i]);
        return native;
    }
    /** Move the chosen entry to the native pop position while retaining everyone else's order. */
    static void promote(uint16_t *tags, unsigned size, unsigned index) {
        invariant(size <= 64 && index < size);
        const auto tag = tags[index];
        for (unsigned i = index + 1; i < size; ++i) tags[i - 1] = tags[i];
        tags[size - 1] = tag;
    }
};

bool enabled();
void resetSession();
void tagAction(dk2::GameAction &action);
uint8_t owner(uint16_t tag);
bool localOwns(uint16_t tag);
uint16_t localLatest(dk2::CPlayer &keeper);
int dropIndex(dk2::CPlayer &keeper);
}
