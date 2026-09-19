#include "network_gem_ending_ui.h"
#include "network_gem_ending.h"
#include "dk2/CDefaultPlayerInterface.h"
#include "dk2_functions.h"
#include <cstdlib>
#include <cstring>

namespace {
/** Incoming references are replaced; forwarding requires the untouched DKII 1.70 body. */
void verifyOriginalKeyboardEntry() {
    static const bool verified = [] {
        const unsigned char entry[] = {
            0x6a, 0xff, 0x68, 0xf0, 0x8c, 0x64, 0x00, 0x64,
            0xa1, 0x00, 0x00, 0x00, 0x00, 0x50, 0x64, 0x89};
        if (std::memcmp(reinterpret_cast<void *>(0x00405010), entry, sizeof(entry))) std::abort();
        return true;
    }();
    (void) verified;
}
}

bool patch::network_gem_ending_ui::pending(const dk2::CDefaultPlayerInterface *iface) {
    // Frontend/disabled calls cannot assume that the iface or world is initialized.
    if (!network_gem_ending::enabled()) return false;
    if (!iface) std::abort();
    const auto keeper = iface->playerTagId;
    if (!network_gem_ending::activeForKeeper(keeper)) return false;
    return pending(true, network_gem_ending::physicalCompleteForKeeper(keeper),
        iface->f4CE2 != 0, iface->f4CEA != 0, network_gem_ending::alternativeForKeeper(keeper));
}

bool patch::network_gem_ending_ui::hideVictoryPrompt(const dk2::CDefaultPlayerInterface *iface,
    const dk2::RtGuiView *view, uintptr_t caller) {
    // The same text buffer also carries authored captions; only this victory caller is gated.
    if (caller != 0x0040422E || view != &iface->_followPathSubtitleTextBuffer) return false;
    return hidePrompt(pending(iface), caller, true);
}

/** ABI 00405010: prevent local skip checks before entering the original keyboard routine. */
BOOL __cdecl dk2::CDefaultPlayerInterface_onKeyboardAction(int key, int pressed, uint32_t modifiers,
    CDefaultPlayerInterface *iface, int context) {
    return patch::network_gem_ending_ui::keyboardAction(
        patch::network_gem_ending_ui::pending(iface), key, pressed, [&] {
            verifyOriginalKeyboardEntry();
            return reinterpret_cast<BOOL (__cdecl *)(int, int, uint32_t, CDefaultPlayerInterface *, int)>(
                0x00405010)(key, pressed, modifiers, iface, context);
        });
}
