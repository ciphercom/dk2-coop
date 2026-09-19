#include "coop_campaign_progress.h"
#include "coop_campaign.h"
#include "dk2_globals.h"
#include <Windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
/** Supply campaign mode only to persistence branches, never to the network simulation. */
int __cdecl persistenceMode() {
    const auto &cfg = dk2::MyResources_instance.gameCfg;
    if (!patch::coop_campaign_progress::eligible(patch::coop_campaign::active(),
        dk2::CFrontEndComponent_instance.mp_isHost != 0, cfg.useFe_playMode,
        cfg.useFe3d != 0, cfg.useFe2d_unk1 != 0, cfg.hasSaveFile != 0)) return cfg.useFe_playMode;
    // Award the native identity chosen by the host, never stale frontend progress.
    const auto *mission = patch::coop_campaign::mission();
    const auto &player = dk2::MyResources_instance.playerCfg;
    const int nativeId = player.secretLevelNumber ? player.secretLevelNumber + 25 : player.levelNumber;
    if (!mission || mission->nativeId != nativeId) std::abort();
    return 1;
}

/** A native MOV changes only EAX. Preserve flags and all other registers across the C++ predicate. */
__declspec(naked) void modeForPersistence() {
    __asm {
        pushfd
        pushad
        call persistenceMode
        mov dword ptr [esp + 28], eax
        popad
        popfd
        ret
    }
}
}

void patch::coop_campaign_progress::install() {
    if (!patch::coop_campaign::active()) return;
    static const bool installed = [] {
        // CPlayer::fun_4BE630: loss persistence, win/branch persistence, secret effects.
        // Native local-Keeper and terminal-status guards remain intact. Both ending
        // functions separately reject global mode3, retaining the shared cinematic wrapper.
        constexpr uintptr_t sites[] = {0x004BEACB, 0x004BEBD6, 0x004BEDC0};
        constexpr unsigned char expected[] = {0xA1, 0x4B, 0xAD, 0x75, 0x00};
        for (const auto site : sites) {
            if (std::memcmp(reinterpret_cast<void *>(site), expected, sizeof(expected))) std::abort();
        }
        for (const auto site : sites) {
            unsigned char call[5] = {0xE8};
            const auto displacement = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&modeForPersistence) - site - sizeof(call));
            std::memcpy(call + 1, &displacement, sizeof(displacement));
            DWORD oldProtection;
            if (!VirtualProtect(reinterpret_cast<void *>(site), sizeof(call), PAGE_EXECUTE_READWRITE, &oldProtection)) std::abort();
            std::memcpy(reinterpret_cast<void *>(site), call, sizeof(call));
            DWORD ignored;
            if (!VirtualProtect(reinterpret_cast<void *>(site), sizeof(call), oldProtection, &ignored) ||
                !FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(site), sizeof(call))) std::abort();
        }
        return true;
    }();
    (void) installed;
}
