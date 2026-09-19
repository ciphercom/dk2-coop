#pragma once
#include <cstdint>
#include <cstdlib>

namespace dk2 { struct MyGameSession; }
namespace patch::network_gem_ending {

/** Native ending selection is driven by shared authored flags. */
inline bool usesAlternativeEnding(uint32_t flags) { return (flags & 0x18) != 0; }
/** Campaign cinematics belong only to the shared local Keeper. */
inline bool eligibleWinner(uint16_t winner, uint16_t localKeeper) {
    return winner == localKeeper;
}
/** A native ending can complete without creating a gem creature. */
struct Started {
    uint16_t creature;
    bool alternative;
    Started(uint16_t creature, bool alternative = false) : creature(creature), alternative(alternative) {}
};

/** One shared victory cinematic; Controller-local camera/input state never selects its owner. */
class Ending {
    uint16_t keeper_ = 0, creature_ = 0;
    uint32_t lastTick_ = 0;
    bool attempted_ = false, complete_ = false, hasTick_ = false, alternative_ = false;
public:
    bool alternativeFor(uint16_t keeper) const { return alternative_ && keeper == keeper_; }
    bool completeAlternative(uint16_t keeper, bool nativeComplete) {
        if (!alternativeFor(keeper) || !nativeComplete || complete_) return false;
        complete_ = true;
        return true;
    }
    uint16_t alternativeKeeper() const { return alternative_ ? keeper_ : 0; }
    void reset() { *this = Ending{}; }
    bool observeTick(uint32_t tick) {
        const bool rewound = hasTick_ && tick < lastTick_;
        if (rewound) reset();
        lastTick_ = tick;
        hasTick_ = true;
        return rewound;
    }
    bool activeFor(uint16_t keeper) const { return (creature_ || alternative_) && keeper == keeper_; }
    bool completeFor(uint16_t keeper) const { return activeFor(keeper) && complete_; }
    bool matches(uint16_t keeper, uint16_t creature) const { return creature_ && activeFor(keeper) && creature == creature_; }
    bool complete(uint16_t keeper, uint16_t creature, bool nativeComplete) {
        if (!matches(keeper, creature) || !nativeComplete || complete_) return false;
        complete_ = true;
        return true;
    }

    template<class Keeper, class Original, class Start>
    int16_t transition(bool enabled, Keeper &keeper, int status, Original original, Start start) {
        const bool candidate = enabled && !attempted_ && keeper.status == 0 && status == 2;
        // Native winner propagation can recurse into another Keeper; reserve the first shared winner.
        if (candidate) attempted_ = true;
        const int16_t result = original();
        if (candidate) {
            if (keeper.status == 2) {
                keeper_ = keeper.f0_tagId;
                // A zero creature is a fallback unless the authored ending explicitly needs no gem.
                const Started started = start();
                if (started.alternative && started.creature) std::abort();
                creature_ = started.creature;
                alternative_ = started.alternative;
            } else attempted_ = false;
        }
        return result;
    }
};

/** Native frozen wait behavior, or the unmodified original ready body; no camera fields are changed. */
template<class Original, class Ready>
int wait(bool enabled, bool matches, bool pending, uint32_t &age, Original original, Ready ready) {
    if (!enabled || !matches) return original();
    if (pending) { --age; return 1; }
    return ready();
}

bool enabled();
bool activeForKeeper(uint16_t keeperTag);
bool physicalCompleteForKeeper(uint16_t keeperTag);
bool alternativeForKeeper(uint16_t keeperTag);
void resetSession();
void beforeWorldTick(dk2::MyGameSession &session);
}
