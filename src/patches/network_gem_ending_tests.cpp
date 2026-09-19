#include "network_gem_ending.h"
#include "scripted_camera_timeline.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
void require(bool valid, const char *message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}
struct Keeper { uint16_t f0_tagId; int status; };
}

/** Verify shared transitions and native wait contracts independently of binary frame layout. */
int main() {
    using namespace patch::network_gem_ending;
    // Authored alternate endings have no gem creature or gem-interface completion flags.
    require(!usesAlternativeEnding(0x04) &&
        usesAlternativeEnding(0x08) && usesAlternativeEnding(0x10),
        "only active campaign level flags must select the native alternative ending");
    Ending localEnding;
    Keeper enemy{62, 0}, shared{61, 0};
    int localStarts = 0, localOriginals = 0;
    require(localEnding.transition(eligibleWinner(enemy.f0_tagId, shared.f0_tagId), enemy, 2,
        [&] { ++localOriginals; enemy.status = 2; return -9; },
        [&] { ++localStarts; return Started{0, true}; }) == -9 &&
        localOriginals == 1 && localStarts == 0 && !localEnding.activeFor(62),
        "another Keeper's victory must forward without reserving or starting the shared ending");
    localEnding.transition(eligibleWinner(shared.f0_tagId, shared.f0_tagId), shared, 2,
        [&] { ++localOriginals; shared.status = 2; return 1; },
        [&] { ++localStarts; return Started{0, true}; });
    require(localOriginals == 2 && localStarts == 1 && localEnding.alternativeFor(61),
        "a prior enemy victory must not prevent the shared Keeper's later ending");
    Ending alternative;
    Keeper alternateKeeper{61, 0};
    alternative.transition(true, alternateKeeper, 2,
        [&] { alternateKeeper.status = 2; return 7; }, [] { return Started{0, true}; });
    require(alternative.activeFor(61) && alternative.alternativeFor(61) &&
        !alternative.matches(61, 0) && !alternative.completeFor(61),
        "alternative ending must track its Keeper without inventing a gem creature");
    require(!alternative.completeAlternative(62, true) && !alternative.completeAlternative(61, false),
        "alternative ending must wait for its own native completion deadline");
    require(alternative.completeAlternative(61, true) && alternative.completeFor(61),
        "alternative ending must release when the native deadline clears");
    alternative.reset();
    require(!alternative.alternativeFor(61), "reset must clear alternative ending ownership");
    Ending ending;
    Keeper keeper{19, 0};
    std::string calls;
    auto win = [&] { calls += 'O'; keeper.status = 2; return int16_t(-7); };
    auto start = [&] { calls += 'S'; return uint16_t(411); };
    require(ending.transition(true, keeper, 2, win, start) == -7 && calls == "OS",
        "shared victory must call original once before starting, preserving its result");
    require(ending.activeFor(19) && !ending.activeFor(3) && ending.matches(19, 411) &&
        !ending.matches(19, 412) && !ending.matches(20, 411), "ending must bind the shared Keeper and native creature");
    calls.clear();
    ending.transition(true, keeper, 2, win, start);
    require(calls == "O", "repeated victory must not restart the original cinematic");
    Keeper second{20, 0};
    ending.transition(true, second, 2, [&] { second.status = 2; return 1; }, start);
    require(calls == "O" && !ending.activeFor(20), "one terminal cinematic must not restart for another Keeper");
    require(!ending.complete(20, 411, true) && !ending.complete(19, 412, true) &&
        !ending.complete(19, 411, false), "unrelated or incomplete states must not release the ending");
    require(ending.complete(19, 411, true) && ending.completeFor(19) && ending.activeFor(19),
        "physical completion must preserve ending scope for local presentation gating");
    require(!ending.complete(19, 411, true), "completion must be observed once");
    Ending nested;
    Keeper outer{51, 0}, inner{52, 0};
    int nestedStarts = 0, nestedOriginals = 0;
    const int16_t outerResult = nested.transition(true, outer, 2, [&] {
        ++nestedOriginals;
        require(nested.transition(true, inner, 2, [&] { ++nestedOriginals; inner.status = 2; return -11; },
            [&] { ++nestedStarts; return uint16_t(512); }) == -11,
            "nested status propagation must preserve its native return value");
        outer.status = 2;
        return -12;
    }, [&] { ++nestedStarts; return uint16_t(511); });
    require(outerResult == -12 && nestedOriginals == 2 && nestedStarts == 1 &&
        nested.matches(51, 511) && !nested.activeFor(52),
        "the first shared victory must retain cinematic ownership across recursive native propagation");
    Ending rejected;
    Keeper retry{53, 0};
    rejected.transition(true, retry, 2, [] { return 0; }, [] { return uint16_t(531); });
    rejected.transition(true, retry, 2, [&] { retry.status = 2; return 1; }, [] { return uint16_t(532); });
    require(rejected.matches(53, 532), "a rejected native transition must release its reservation for a later victory");
    for (int excluded = 0; excluded < 4; ++excluded) {
        Ending ignored;
        Keeper other{42, excluded == 1 ? 3 : 0};
        int originals = 0, starts = 0;
        ignored.transition(excluded != 0, other, excluded == 2 ? 3 : 2,
            [&] { ++originals; other.status = excluded == 3 ? 0 : 2; return 13; },
            [&] { ++starts; return uint16_t(9); });
        require(originals == 1 && starts == 0 && !ignored.activeFor(42),
            "disabled, terminal, loss, or rejected transitions must forward without starting");
    }
    Ending fallback;
    Keeper noGem{31, 0};
    int fallbackStarts = 0;
    fallback.transition(true, noGem, 2, [&] { noGem.status = 2; return 1; },
        [&] { ++fallbackStarts; return uint16_t(0); });
    noGem.status = 0;
    fallback.transition(true, noGem, 2, [&] { noGem.status = 2; return 1; },
        [&] { ++fallbackStarts; return uint16_t(99); });
    require(!fallback.activeFor(31) && fallbackStarts == 1, "native no-gem/failure fallback must not stay pending or retry");
    require(!ending.observeTick(100) && !ending.observeTick(101), "normal ticks must retain the ending");
    require(ending.observeTick(99) && !ending.activeFor(19), "observed tick rewind must clear stale ending ownership");
    keeper.status = 0;
    calls.clear();
    ending.transition(true, keeper, 2, win, start);
    require(calls == "OS", "rewind must allow a subsequent fresh shared transition");
    ending.reset();
    require(!ending.activeFor(19) && !ending.completeFor(19), "session reset must clear physical completion and ownership");
    for (bool enabled : {false, true}) for (bool matching : {false, true}) {
        uint32_t age = 8;
        std::string trace;
        const int result = wait(enabled, matching, true, age,
            [&] { trace += 'O'; return 23; }, [&] { trace += 'R'; return 31; });
        if (enabled && matching)
            require(result == 1 && age == 7 && trace.empty(), "pending wait must reproduce native age decrement and return1");
        else require(result == 23 && age == 8 && trace == "O", "excluded wait must forward the original exactly once");
    }
    uint32_t age = 9;
    require(wait(true, true, false, age, [] { return -1; }, [&] { age = 22; return 37; }) == 37 && age == 22,
        "ready native continuation must retain its state changes and return value");
    // E6 observes movement only; an authored rotation must not prolong this native movement wait.
    patch::scripted_camera::MotionWaits motion;
    motion.dispatch(true, 4, 3000, 10, 4, false, [] { return 1; });
    require(motion.movementPending(), "a shared move must make E6 pending");
    motion.advance(12, false);
    require(!motion.movementPending() && !motion.condition(true, 72, 0, 3, 0),
        "E6 must release after movement while the separate rotation wait is still pending");
    return 0;
}
