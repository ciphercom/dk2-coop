#pragma once

/** Keep authored campaign rules independent of the multiplayer carrier. */
namespace patch::coop_campaign_init {
/** Supply campaign rules only to fresh selected network campaigns. */
inline int effectiveMode(bool selected, int mode, bool frontend3d, bool frontend2d, bool saved) {
    return selected && mode == 3 && !frontend3d && !frontend2d && !saved ? 1 : mode;
}
/** Skip multiplayer outcome propagation without adding campaign score persistence. */
inline int terminalMode(bool selected, int mode, bool frontend3d, bool frontend2d, bool saved) {
    return mode == 3 && effectiveMode(selected, mode, frontend3d, frontend2d, saved) == 1 ? 0 : mode;
}
/** Install guarded native mode reads; the global mode and network transport stay untouched. */
void install();
}
