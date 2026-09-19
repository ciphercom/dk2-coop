#pragma once
#include <cstdint>

namespace patch::health_flower {
/** Only Keeper-view Controllers override the shared Keeper's possession display gate. */
inline bool possessionDisplay(bool cooperative, bool nativePossessed, uint32_t cameraMode) {
    if (!cooperative) return nativePossessed;
    // Native CCamera::sub_44D870 classifies these modes as Keeper views (group 2).
    switch (cameraMode) {
    case 3: case 4: case 5: case 9: case 13: case 14: case 15: return false;
    default: return nativePossessed;
    }
}
void install();
}
