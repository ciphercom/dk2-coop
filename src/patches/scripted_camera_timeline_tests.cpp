#include "scripted_camera_timeline.h"
#include "scripted_camera_playback.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {
/** Contract failures must remain fatal in every test build. */
void require(bool valid, const char *message) {
    if (!valid) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

/** Original-engine boundary double: expose the fields the engine owns. */
struct EngineCamera {
    uint32_t _mode = 3;
    int numPoints = 32;
    int fEC6 = 0;
    int fECA = 0x3f000000;
    int savedMode = 7;
    int completions = 0;

    char load() {
        if (_mode == 18) return 18; // The real rejected load also returns nonzero.
        _mode = 18;
        fEC6 = 0;
        fECA = 0x3f000000;
        return 0; // Accepted network load may return zero.
    }
    int finish() { _mode = savedMode; ++completions; fEC6 = 0; return 123; }
};

/** Different render rates cannot change the original camera-complete predicate. */
void playbackContract() {
    using patch::scripted_camera::Playback;
    Playback fast, slow;
    EngineCamera a, b;
    require(fast.load(true, a, 50, 10, [&](bool) { return a.load(); }) == 0, "load result changed");
    slow.load(true, b, 50, 10, [&](bool) { return b.load(); });
    require(fast.load(true, a, 52, 10, [&](bool) { return a.load(); }) == 18, "rejected load result changed");
    a.fEC6 = 40; // A slow world can allow the render clock to overshoot the path.
    fast.complete(true, a, [&] { return a.finish(); });
    require(a._mode == 18 && a.completions == 0, "render advanced the campaign predicate early");
    require(a.fEC6 == 30 && a.fECA == 0x3f800000, "deferred endpoint permits out-of-bounds next-point rendering");
    slow.complete(true, b, [&] { return b.finish(); }); // Early local skip must not end the wait.
    require(b.fEC6 == 0 && b.fECA == 0x3f000000, "early skip jumped the local camera to the endpoint");
    fast.beforeTick(true, a, 60, [&] { return a.finish(); });
    slow.beforeTick(true, b, 60, [&] { return b.finish(); });
    require(a._mode == 18 && b._mode == 18, "campaign predicate became true before deadline");
    fast.beforeTick(true, a, 61, [&] { return a.finish(); });
    slow.beforeTick(true, b, 61, [&] { return b.finish(); });
    require(a._mode == 7 && b._mode == 7 && a.completions == 1 && b.completions == 1,
            "shared boundary failed to restore both cameras through original completion");
    fast.beforeTick(true, a, 62, [&] { return a.finish(); });
    require(a.completions == 1, "completion was repeated after the shared boundary");
    fast.load(true, a, 70, 10, [&](bool) { return a.load(); });
    fast.beforeTick(true, a, 80, [&] { return a.finish(); });
    require(a._mode == 18, "next path reused previous deadline");
    fast.beforeTick(true, a, 81, [&] { return a.finish(); });
    require(a._mode == 7 && a.completions == 2, "next path could not complete");
    fast.load(true, a, 90, 10, [&](bool) { return a.load(); });
    a._mode = 4; // An unrelated original camera-mode change interrupts the path.
    fast.beforeTick(true, a, 95, [&] { return a.finish(); });
    require(fast.pending(), "local camera mode shortened the shared path wait");
    fast.beforeTick(true, a, 101, [&] { return a.finish(); });
    require(a._mode == 4 && a.completions == 2, "interrupted path overwrote the new camera mode");
    fast.load(true, a, 110, 10, [&](bool) { return a.load(); });
    fast.reset();
    require(fast.complete(true, a, [&] { return a.finish(); }) == 123, "untracked completion was not forwarded intact");
}

/** Every excluded launch retains the original load and completion behavior. */
void scopeContract() {
    using namespace patch::scripted_camera;
    require(enabledFor(true, 3, false, false, false), "co-op campaign session was excluded");
    const bool excluded[] = {
        enabledFor(false, 3, false, false, false),
        enabledFor(true, 1, false, false, false),
        enabledFor(true, 3, true, false, false),
        enabledFor(true, 3, false, true, false),
        enabledFor(true, 3, false, false, true)};
    for (bool enabled : excluded) {
        require(!enabled, "excluded session received campaign camera synchronization");
        Playback playback;
        EngineCamera camera;
        require(playback.load(enabled, camera, 0, 10, [&](bool) { return camera.load(); }) == 0,
                "excluded session changed original load result");
        require(camera._mode == 18 && !playback.pending(), "excluded session armed shared state");
        playback.beforeTick(enabled, camera, 100, [&] { return camera.finish(); });
        require(camera._mode == 18, "excluded session forced a completion");
        require(playback.complete(enabled, camera, [&] { return camera.finish(); }) == 123 && camera._mode == 7,
                "excluded session suppressed original completion");
    }
}

/** Shared script commands, rather than local renderer deadlines, own type72 waits. */
void motionWaitContract() {
    using patch::scripted_camera::MotionWaits;
    MotionWaits waits;
    int calls = 0;
    auto original = [&] { ++calls; return 17; };
    require(waits.dispatch(false, 4, 1000, 100, 4, false, original) == 17 && calls == 1,
        "disabled command changed original forwarding");
    require(waits.condition(true, 72, 0, 3, 0) == 1, "disabled command armed motion");
    require(waits.dispatch(true, 3, 1000, 100, 4, false, original) == 17 && calls == 2,
        "unrelated action changed original forwarding");
    require(waits.condition(true, 72, 0, 3, 0) == 1, "unrelated action armed motion");
    require(waits.dispatch(true, 4, 0, 100, 4, false, original) == 17 && calls == 3,
        "shared move changed original return or skipped original");
    waits.advance(101, false);
    require(waits.condition(true, 72, 0, 3, 0) == 0 && waits.condition(true, 72, 0, 7, 1) == 0,
        "local camera result or mode shortened the 500ms shared wait");
    waits.advance(102, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1 && waits.condition(true, 72, 0, 3, 1) == 1,
        "500ms move did not finish after two ticks regardless of local DFA");
    require(waits.condition(false, 72, 0, 3, 23) == 23 && waits.condition(true, 71, 0, 3, 24) == 24,
        "excluded predicate changed original result");
    require(waits.condition(true, 72, 10, 3, 1) == 0 && waits.condition(true, 72, 0, 18, 1) == 0,
        "motion correction bypassed endTime or scripted path mode");

    waits.dispatch(true, 4, 1250, 200, 4, false, original);
    waits.advance(202, false);
    waits.dispatch(true, 4, 0, 202, 4, false, original);
    waits.advance(204, false);
    require(waits.condition(true, 72, 0, 3, 1) == 0, "duration zero cleared an authored rotation");
    waits.advance(205, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1, "authored rotation duration was extended");
    waits.dispatch(true, 4, 2000, 210, 4, false, original);
    waits.dispatch(true, 4, 250, 211, 4, false, original);
    waits.advance(212, false);
    require(waits.condition(true, 72, 0, 3, 1) == 0, "new move reused an expired movement deadline");
    waits.advance(213, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1, "new rotation did not replace the previous duration");

    // Path starts at301 and completes at310: [301,310) consumes no motion budget.
    waits.dispatch(true, 4, 1000, 300, 4, false, original);
    waits.advance(301, true);
    waits.advance(310, true);
    waits.advance(310, false);
    waits.advance(312, false);
    require(waits.condition(true, 72, 0, 3, 1) == 0, "path completion consumed an extra motion tick");
    waits.advance(313, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1, "paused rotation failed to resume after shared path finish");
    waits.dispatch(true, 4, 0, 500, 4, true, original);
    waits.advance(510, true);
    waits.advance(510, false);
    waits.advance(511, false);
    require(waits.condition(true, 72, 0, 7, 1) == 0, "move started inside a path was not paused");
    waits.advance(512, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1, "move inside path resumed on the wrong boundary");
    waits.dispatch(true, 4, 1000, 600, 4, false, original);
    waits.advance(0, false);
    require(waits.condition(true, 72, 0, 3, 0) == 1, "tick rewind retained motion from previous simulation");
    waits.dispatch(true, 4, 1000, 1, 4, false, original);
    waits.reset();
    require(waits.condition(true, 72, 0, 3, 0) == 1, "session reset retained a shared motion wait");
    waits.advance(700, true);
    require(waits.condition(true, 72, 0, 7, 1) == 0,
        "local mode escape released a tracked shared path with no pending motion");
    waits.advance(710, false);
    require(waits.condition(true, 72, 0, 7, 0) == 1, "shared path completion retained an empty motion wait");
}

/** Independent possession tweens must not schedule the same campaign message on different ticks. */
void possessionTransitionContract() {
    using patch::scripted_camera::MotionWaits;
    MotionWaits host, client;
    for (uint32_t tick = 338; tick <= 346; ++tick) {
        host.advance(tick, false);
        client.advance(tick, false);
        const int hostEndTime = tick < 346 ? 12345 : 0;
        require(host.condition(true, 72, hostEndTime, 3, !hostEndTime, true) ==
                client.condition(true, 72, 0, 3, 1, true),
            "local possession exit tween scheduled campaign progress on different ticks");
    }
    require(host.condition(true, 72, 12345, 2, 0, true) == 1,
        "local possession entry tween blocked a shared campaign wait");
    require(host.condition(true, 72, 12345, 3, 0, false) == 0 &&
            host.condition(false, 72, 12345, 3, 23, true) == 23 &&
            host.condition(true, 71, 12345, 3, 24, true) == 24,
        "possession correction changed an excluded predicate");
    require(host.condition(true, 72, 0, 18, 1, true) == 0,
        "possession correction released native scripted path mode");
    host.advance(347, true);
    require(host.condition(true, 72, 12345, 3, 0, true) == 0,
        "possession correction released an authored path after a local mode change");
    host.advance(350, false);
    host.dispatch(true, 4, 1000, 350, 4, false, [] { return 1; });
    host.advance(351, false);
    require(host.condition(true, 72, 0, 3, 1, true) == 0,
        "possession correction released authored camera movement early");
    host.advance(352, false);
    require(host.condition(true, 72, 0, 3, 1, true) == 0,
        "possession correction released authored rotation early");
    host.advance(354, false);
    require(host.condition(true, 72, 12345, 3, 0, true) == 1,
        "local tween extended a completed authored camera wait");
}

/** Native mode/tween rejection must not make an authored path exist on only one peer. */
void possessionPathContract() {
    using patch::scripted_camera::Playback;
    struct Camera : EngineCamera {
        int endTime = 0;
        unsigned destinationMode = 3;
        uint16_t fDEC = 42;
        bool fileExists = true;
        char loadBody() {
            if (!fileExists) return 0;
            savedMode = _mode; // Native set-mode18 saves the actual previous mode at +ED2.
            fDEC = 0; // Native 0044AF41 clears the creature target when entering mode18.
            return EngineCamera::load();
        }
        char nativeLoad(bool sharedAcceptance) {
            // DKII 1.70 0044A37B..0044A3AF gates the file load on local presentation.
            if (!sharedAcceptance && (_mode == 18 || _mode == 2 || _mode == 10 || _mode == 11 || endTime))
                return 23;
            if (!sharedAcceptance) return loadBody();
            return patch::scripted_camera::loadShared(*this,
                [&] { _mode = destinationMode; endTime = 0; }, [&] { return loadBody(); });
        }
    };
    for (unsigned mode : {1u, 2u, 3u, 10u, 11u}) {
        for (int tween : {0, 12345}) {
            Playback host, client;
            Camera keeper, possessed;
            possessed._mode = mode;
            possessed.endTime = tween;
            possessed.destinationMode = mode == 1 && tween ? 2 : mode;
            host.load(true, keeper, 100, 10, [&](bool shared = false) { return keeper.nativeLoad(shared); });
            client.load(true, possessed, 100, 10, [&](bool shared = false) { return possessed.nativeLoad(shared); });
            require(host.pending() && client.pending() && keeper._mode == 18 && possessed._mode == 18,
                "local possession mode or tween rejected the shared cutscene");
            require(host.completionTick() == client.completionTick(), "cutscene deadlines diverged across peers");
            require(client.load(true, possessed, 102, 10,
                [&](bool shared = false) { return possessed.nativeLoad(shared); }) == 23,
                "overlapping authored path bypassed native rejection");
            host.beforeTick(true, keeper, 111, [&] { return keeper.finish(); });
            client.beforeTick(true, possessed, 111, [&] { return possessed.finish(); });
            require(keeper._mode == 3 && possessed._mode == possessed.destinationMode && !host.pending() && !client.pending(),
                "cutscene failed to restore each Controller's previous camera mode");
            require(possessed.fDEC == 42, "cutscene lost the possessed creature target");
            require(!possessed.endTime, "cutscene left an unfinished possession tween blocking input");
        }
    }
    Playback excluded, missing;
    Camera camera;
    camera._mode = 2;
    require(excluded.load(false, camera, 1, 10,
        [&](bool shared = false) { return camera.nativeLoad(shared); }) == 23 && camera._mode == 2 && !excluded.pending(),
        "excluded session bypassed native possession rejection");
    camera.fileExists = false;
    missing.load(true, camera, 1, 10, [&](bool shared = false) { return camera.nativeLoad(shared); });
    require(camera._mode == 2 && !missing.pending(), "missing path file changed possession or armed a wait");

    Playback released;
    Camera dead;
    dead._mode = 2;
    released.load(true, dead, 1, 10, [&](bool shared = false) { return dead.nativeLoad(shared); });
    released.beforeTick(true, dead, 12, [&] {
        // Native completion applies any deferred release after restoring its saved mode.
        const int result = dead.finish();
        dead._mode = 3;
        dead.fDEC = 0;
        return result;
    });
    require(dead._mode == 3 && dead.fDEC == 0 && !released.pending(),
        "cutscene resurrected possession after a deferred release or creature death");
}

/** Exercise path and motion clocks together at the same boundaries used by engine hooks. */
void combinedWaitContract() {
    using namespace patch::scripted_camera;
    Playback playback;
    MotionWaits motion;
    EngineCamera camera;
    auto originalMove = [] { return 17; };
    auto beforeTick = [&](uint32_t tick) {
        motion.advance(tick, playback.pending());
        playback.beforeTick(true, camera, tick, [&] { return camera.finish(); });
        motion.advance(tick, playback.pending());
    };
    motion.dispatch(true, 4, 1000, 100, 4, playback.pending(), originalMove);
    beforeTick(101);
    motion.advance(101, playback.pending());
    playback.load(true, camera, 101, 4, [&](bool) { return camera.load(); });
    motion.advance(101, playback.pending());
    beforeTick(105);
    require(camera._mode == 18 && motion.condition(true, 72, 0, camera._mode, 1) == 0,
        "combined wait released before the authored path deadline");
    beforeTick(106);
    require(camera.completions == 1 && camera._mode == 7,
        "combined wait failed to complete original camera presentation");
    beforeTick(108);
    require(motion.condition(true, 72, 0, camera._mode, 1) == 0,
        "path completion consumed part of the remaining three rotation ticks");
    beforeTick(109);
    require(motion.condition(true, 72, 0, camera._mode, 0) == 1,
        "combined wait did not release after the three remaining rotation ticks");
}
}

/** Authored 30 Hz paths complete on shared ticks, independently of rendered frames. */
int main() {
    playbackContract();
    scopeContract();
    motionWaitContract();
    possessionTransitionContract();
    possessionPathContract();
    combinedWaitContract();
    using patch::scripted_camera::Timeline;
    Timeline path;
    require(!path.pending(), "new session has a pending camera path");
    require(!path.started(18, 18, 100, 62, 0, 10), "rejected start armed a deadline");
    require(!path.started(3, 3, 100, 62, 0, 10), "failed file load armed a deadline");
    require(path.started(3, 18, 100, 62, 0, 10), "accepted path did not arm");
    require(!path.advance(120), "path completed before its authored duration");
    require(path.pending(), "render-independent wait vanished before deadline");
    require(path.advance(121), "path did not complete on the first eligible tick");
    require(!path.pending() && !path.advance(122), "completed path completed twice");

    // Reloading the same authored path starts a fresh wait; failed overlapping
    // loads must not move the accepted path's deadline.
    require(path.started(3, 18, 200, 32, 0, 10), "repeated path did not arm");
    require(!path.started(18, 18, 205, 62, 0, 10), "overlapping path was accepted");
    require(!path.advance(210) && path.advance(211), "repeated path used a stale deadline");

    // Paths 50..99 start at authored point seven with the same half-point phase.
    require(path.started(3, 18, 300, 32, 7, 10), "offset path did not arm");
    require(!path.advance(307) && path.advance(308), "offset path duration is wrong");
    require(path.started(3, 18, 400, 3, 0, 20), "exact-boundary path did not arm");
    require(path.advance(401), "exact integer duration was rounded an extra tick");

    require(path.started(3, 18, 500, 62, 0, 10), "reset example did not arm");
    path.reset();
    require(!path.pending() && !path.advance(521), "session reset retained a deadline");
    require(path.started(3, 18, 600, 62, 0, 10), "rewind example did not arm");
    require(!path.advance(0) && !path.pending(), "world reload retained an old deadline");
    require(path.started(3, 18, 0, 62, 0, 10), "new session path did not arm");
    require(path.advance(21), "new session inherited previous-session timing");
    return 0;
}
