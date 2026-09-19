#pragma once
#include "scripted_camera_timeline.h"

namespace patch::scripted_camera {

/** Keep campaign timing out of ordinary multiplayer, local games, frontends and saves. */
inline bool enabledFor(bool campaign, int playMode, bool frontend3d, bool frontend2d, bool save) {
    return campaign && playMode == 3 && !frontend3d && !frontend2d && !save;
}

/** Finish a local tween before suspending its camera, retaining the native return target. */
template<class Camera, class FinishTween, class Original>
char loadShared(Camera &camera, FinishTween finishTween, Original original) {
    // Mode18 does not advance local tweens. Finish through the native routine so
    // the saved return mode is the intended destination, with input unblocked.
    if (camera.endTime) finishTween();
    const auto target = camera.fDEC;
    const char result = original();
    // Native path entry clears DEC, but completion restores only the mode. Keep
    // its creature target now; later death/release and deferred changes still win.
    if (camera._mode == 18) camera.fDEC = target;
    return result;
}

/** Coordinate original engine calls without owning or duplicating camera data. */
class Playback {
    Timeline timeline;
    const void *trackedCamera = nullptr;

public:
    bool pending() const { return timeline.pending(); }
    uint32_t completionTick() const { return timeline.completionTick(); }
    void reset() { timeline.reset(); trackedCamera = nullptr; }

    /** Shared paths bypass local presentation gates; native file loading still decides success. */
    template<class Camera, class Original>
    char load(bool enabled, Camera &camera, uint32_t tick, uint32_t ticksPerSecond, Original original) {
        if (!enabled) return original(false);
        if (trackedCamera != &camera) reset();
        const auto before = camera._mode;
        // Possession and its wall-clock tween are Controller-local. They cannot
        // veto an authored path on one peer, or replace an already running path.
        const char result = original(!timeline.pending() && before != 18);
        // A local mode escape may permit a presentation load, but cannot replace
        // the already accepted shared deadline. Always preserve the engine call.
        if (timeline.pending()) return result;
        if (timeline.started(before, camera._mode, tick, camera.numPoints, camera.fEC6, ticksPerSecond))
            trackedCamera = &camera;
        return result;
    }

    /** Render and local-skip completion requests share this engine boundary. */
    template<class Camera, class Original>
    int complete(bool enabled, Camera &camera, Original original) {
        if (enabled && timeline.pending() && trackedCamera == &camera && camera._mode == 18) {
            // Both original renderers interpolate EC6 and EC6+1. Holding mode18
            // past the path end must leave both indices inside the authored data.
            // A mid-path local skip is ignored without jumping the presentation.
            if (camera.fEC6 >= camera.numPoints - 1) {
                if (camera.numPoints < 2) std::abort();
                camera.fEC6 = camera.numPoints - 2;
                camera.fECA = 0x3f800000; // IEEE-754 1.0, the final sample of this segment.
            }
            return 0; // Both original request callers ignore the result.
        }
        return original();
    }

    /** Restore original camera state before the world reads its completion predicate. */
    template<class Camera, class Original>
    bool beforeTick(bool enabled, Camera &camera, uint32_t tick, Original original) {
        if (!enabled || trackedCamera != &camera) {
            reset();
            return false;
        }
        if (!timeline.advance(tick)) return false;
        // Local mode changes do not release the shared path wait early, and its
        // eventual completion must not overwrite an unrelated presentation mode.
        if (camera._mode != 18) { reset(); return false; }
        original();
        reset();
        return true;
    }
};
}
