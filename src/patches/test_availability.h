#pragma once

namespace dk2 { struct GameCfg; struct CWorld; }

namespace patch::test_availability {
/** Testing only: unlock spell/building availability once after a fresh network world loads. */
bool apply(bool enabled, const dk2::GameCfg &config, dk2::CWorld &world);
}
