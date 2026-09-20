#include "graphics_lifecycle.h"
#include "game_bridge.h"
#include "dk2_globals.h"
#include "dk2_functions.h"

namespace patch::diagnostic {

void graphicsLifecycle(const char *event, void *caller, const char *surface,
    unsigned width, unsigned height, unsigned reductionFlags) {
    static GraphicsLogBudget budget;
    if (!budget.take(enabled(), surface != nullptr)) return;
    // Initialization and cleanup also run outside gameplay. Never follow a world/session pointer here.
    const auto &scaler = dk2::CEngineSurfaceScaler_instance;
    dk2::MyWindow_log_printf(&dk2::MyWindow_instance,
        "[graphics-lifecycle] event=%s caller=%p wall_ms=%lu surface=%.160s width=%u height=%u reduction=0x%X "
        "is3d=%d mgsr=%d lost=%d hash_initialized=%d dd_flags=0x%X dev_flags=0x%X "
        "orig=%p scaled=%p hash_sw=%p hash_hw=%p hash_hw2=%p\n",
        event, caller, GetTickCount(), surface ? surface : "-", width, height, reductionFlags,
        static_cast<int>(dk2::g_sc_is3dInitialized), static_cast<int>(dk2::g_sc_mgsr_initialized),
        static_cast<int>(dk2::g_sc_isCurDdSurfLost), static_cast<int>(dk2::SurfHashList2_initialized),
        static_cast<unsigned>(dk2::MyDirectDraw_instance.flags),
        static_cast<unsigned>(dk2::MyDirectDraw_instance_devTexture.flags),
        static_cast<void *>(scaler.orig_128x128_8a8r8g8b), static_cast<void *>(scaler.scaled_128x128_8a8r8g8b),
        static_cast<void *>(dk2::g_pSurfHashList), static_cast<void *>(dk2::g_pSurfHashList2),
        static_cast<void *>(dk2::pSurfHashList2_2));
}
}
