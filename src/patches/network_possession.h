#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>

namespace dk2 { struct MyGameSession; }
namespace patch::network_possession {
/** A marker in action55's unused data3 preserves the Controller across the shared command stream. */
inline uint32_t originForSlot(int slot) { if (slot < 0 || slot >= 8) std::abort(); return 0x504F5300u | (slot + 1); }
inline bool validOrigin(uint32_t origin) { return origin >= 0x504F5301u && origin <= 0x504F5308u; }

/** Panel slots are one-based and paged; only the visible possession entry is locked while occupied. */
inline bool buttonBlocked(bool enabled, uint16_t creature, uint32_t slot,
        uint32_t page, uint32_t pageSize, uint32_t possessionIndex) {
    return enabled && creature && slot && slot <= pageSize && possessionIndex &&
        page * pageSize + slot == possessionIndex;
}

/** Local presentation follows the accepted native shot, never an ambiguous pending creature target. */
class Presentation {
    std::array<uint32_t, 4096> shotOrigins_{};
    uint16_t keeper_ = 0, creature_ = 0;
    bool releasePending_ = false;
public:
    void reset() { *this = Presentation{}; }
    /** Reloaded in-flight shots have no saved origin; established possession is reconciled separately. */
    void clearPendingShots() { shotOrigins_.fill(0); }
    /** Replace a stale deferred possession before native path completion can dereference its creature. */
    bool releaseCamera(uint32_t mode, uint32_t deferredMode = 0, uint32_t savedMode = 0) {
        if (!releasePending_) return false;
        releasePending_ = false;
        const auto returnMode = mode == 18 ? (deferredMode ? deferredMode : savedMode) : mode;
        return returnMode == 1 || returnMode == 2;
    }
    /** Every allocation overwrites the old tag, including unrelated/native shots. */
    void allocatedShot(uint16_t tag, uint32_t origin) {
        if (!tag || tag >= shotOrigins_.size()) std::abort();
        shotOrigins_[tag] = validOrigin(origin) ? origin : 0;
    }
    uint32_t consumeShot(uint16_t tag) {
        if (!tag || tag >= shotOrigins_.size()) std::abort();
        const auto origin = shotOrigins_[tag];
        shotOrigins_[tag] = 0;
        return origin;
    }
    bool enter(bool enabled, uint32_t origin, uint32_t localOrigin,
            uint16_t keeper, uint16_t localKeeper, uint16_t creature) {
        if (!enabled || keeper != localKeeper) return true;
        if (!validOrigin(origin) || origin != localOrigin) return false;
        if (!creature) std::abort();
        keeper_ = keeper; creature_ = creature;
        releasePending_ = false;
        return true;
    }
    bool owns(uint16_t keeper, uint16_t creature) const {
        return creature_ && keeper_ == keeper && creature_ == creature;
    }
    /** Exit/death can occur without a local release action; return whether local cleanup is needed. */
    bool observe(uint16_t keeper, uint16_t creature) {
        if (!creature_ || owns(keeper, creature)) return false;
        keeper_ = creature_ = 0;
        releasePending_ = true;
        return true;
    }
};
bool enabled();
void resetSession();
void afterWorldReload();
void afterWorldTick(dk2::MyGameSession &session);
bool relativeMouse(bool nativePossessed);
}
