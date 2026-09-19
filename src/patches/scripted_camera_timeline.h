#pragma once
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace patch::scripted_camera {

/** Shared motion budgets exclude wall-clock rendering and local camera input. */
class MotionWaits {
    uint32_t moveTicks = 0, rotationTicks = 0, lastTick = 0;
    bool hasTick = false, pathPaused = false;

    /** Quantize authored milliseconds to the first eligible simulation tick. */
    static uint32_t durationTicks(uint32_t milliseconds, uint32_t ticksPerSecond) {
        if (!ticksPerSecond) std::abort();
        const uint64_t ticks = (uint64_t(milliseconds) * ticksPerSecond + 999) / 1000;
        if (ticks > std::numeric_limits<uint32_t>::max()) std::abort();
        return static_cast<uint32_t>(ticks);
    }

public:
    bool movementPending() const { return moveTicks != 0; }
    void reset() { moveTicks = rotationTicks = lastTick = 0; hasTick = pathPaused = false; }

    /** The state at the interval's start determines whether that interval consumes time. */
    void advance(uint32_t tick, bool pathPending) {
        if (hasTick && tick < lastTick) reset();
        if (hasTick && !pathPaused) {
            const auto elapsed = tick - lastTick;
            moveTicks = moveTicks > elapsed ? moveTicks - elapsed : 0;
            rotationTicks = rotationTicks > elapsed ? rotationTicks - elapsed : 0;
        }
        lastTick = tick;
        hasTick = true;
        pathPaused = pathPending;
    }

    /** Only the shared script dispatch seam may start waits; presentation still runs unchanged. */
    template<class Original>
    int dispatch(bool enabled, int kind, uint32_t rotationMs, uint32_t tick,
                 uint32_t ticksPerSecond, bool pathPending, Original original) {
        if (enabled && kind == 4) {
            advance(tick, pathPending);
            // The original automatic movement caps its local distance-derived time at 500ms.
            moveTicks = durationTicks(500, ticksPerSecond);
            // Original 0044A550 leaves a pending rotation untouched when duration is zero.
            if (rotationMs) rotationTicks = durationTicks(rotationMs, ticksPerSecond);
        }
        return original();
    }

    /** Replace local motion checks; independent possession also excludes the local mode tween. */
    int condition(bool enabled, int type, int endTime, uint32_t mode, int original,
                  bool controllerLocalPossession = false) const {
        if (!enabled || type != 72) return original;
        // Native possession exit sets endTime from wall time on only its Controller.
        // It still animates locally; shared paths and authored movement/rotation retain their waits.
        return (!endTime || controllerLocalPossession) && mode != 18 &&
            !pathPaused && !moveTicks && !rotationTicks;
    }
};

/** One accepted scripted path, timed by the shared world instead of rendering. */
class Timeline {
    bool active = false;
    uint32_t deadline = 0;
    uint32_t lastTick = 0;

public:
    bool pending() const { return active; }
    uint32_t completionTick() const { return deadline; }
    void reset() { active = false; }

    /** The original return byte is not a success flag; entering mode18 is. */
    bool started(uint32_t beforeMode, uint32_t afterMode, uint32_t tick,
                 int pointCount, int firstPoint, uint32_t ticksPerSecond) {
        if (beforeMode == 18 || afterMode != 18) return false;
        if (active || firstPoint < 0 || pointCount <= firstPoint + 1 || !ticksPerSecond)
            std::abort();
        // Original paths start halfway through their first sample and use 30
        // samples per second. Integer half-samples avoid frame/float rounding.
        const uint64_t halfSamples = 2ull * (pointCount - 1 - firstPoint) - 1;
        const uint64_t ticks = (halfSamples * ticksPerSecond + 59) / 60;
        if (ticks > std::numeric_limits<uint32_t>::max() - tick) std::abort();
        deadline = tick + static_cast<uint32_t>(ticks);
        lastTick = tick;
        active = true;
        return true;
    }

    /** Return true once, before the world evaluates the deadline tick. */
    bool advance(uint32_t tick) {
        if (!active) return false;
        // Loading an earlier world invalidates presentation state from the old
        // session. Explicit session reset also handles reuse at the same tick.
        if (tick < lastTick) {
            reset();
            return false;
        }
        lastTick = tick;
        if (tick < deadline) return false;
        reset();
        return true;
    }
};
}
