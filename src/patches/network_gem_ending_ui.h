#pragma once

#include <cstdint>

namespace dk2 { struct CDefaultPlayerInterface; struct RtGuiView; }

namespace patch::network_gem_ending_ui {
/** Wait for shared effects and, for gem endings, the Controller's native animation. */
inline bool pending(bool active, bool physicalComplete, bool uiStarted, bool uiComplete, bool alternative = false) {
    return active && (!physicalComplete || (!alternative && (!uiStarted || !uiComplete)));
}

/** This call site is only the network victory prompt, not cinematic subtitles. */
inline bool hidePrompt(bool waiting, uintptr_t caller, bool subtitleBuffer) {
    return waiting && caller == 0x0040422E && subtitleBuffer;
}

/** Consume skip key presses before any native skip handling; preserve all other calls. */
template<class Original>
int keyboardAction(bool waiting, int key, int pressed, Original original) {
    if (waiting && pressed && (key == 0x39 || key == 0x01)) return 1;
    return original();
}

bool pending(const dk2::CDefaultPlayerInterface *iface);
bool hideVictoryPrompt(const dk2::CDefaultPlayerInterface *iface, const dk2::RtGuiView *view,
    uintptr_t caller);
}
