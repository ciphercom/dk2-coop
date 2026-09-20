#pragma once

namespace patch::diagnostic {

/** Bound lifecycle output while preserving room for the first missing-scratch fault. */
class GraphicsLogBudget {
    unsigned lines_ = 0;
    bool sawMissingScratch_ = false;
public:
    bool take(bool enabled, bool missingScratch) {
        if (!enabled) return false;
        if (missingScratch && !sawMissingScratch_) {
            sawMissingScratch_ = true;
            ++lines_;
            return true;
        }
        if (lines_ >= (sawMissingScratch_ ? 128u : 127u)) return false;
        ++lines_;
        return true;
    }
};

/** Read static graphics state only with capacity; a surface identifies a missing-scratch fault. */
void graphicsLifecycle(const char *event, void *caller, const char *surface = nullptr,
    unsigned width = 0, unsigned height = 0, unsigned reductionFlags = 0);
}
