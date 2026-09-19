#include "coop_campaign_init.h"
#include "coop_campaign.h"
#include "dk2_globals.h"
#include <Windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
/** Only the native world-initialization branches see campaign mode; transport stays networked. */
int __cdecl initializationMode() {
    const auto &cfg = dk2::MyResources_instance.gameCfg;
    return patch::coop_campaign_init::effectiveMode(patch::coop_campaign::mission() != nullptr,
        cfg.useFe_playMode, cfg.useFe3d != 0, cfg.useFe2d_unk1 != 0, cfg.hasSaveFile != 0);
}

/** Zero bypasses both multiplayer automatic outcomes and the separate campaign score writer. */
int __cdecl outcomeMode() {
    const auto &cfg = dk2::MyResources_instance.gameCfg;
    return patch::coop_campaign_init::terminalMode(patch::coop_campaign::mission() != nullptr,
        cfg.useFe_playMode, cfg.useFe3d != 0, cfg.useFe2d_unk1 != 0, cfg.hasSaveFile != 0);
}

/** Replace a MOV into EAX while retaining its original flags and other register values. */
__declspec(naked) void modeForInitialization() {
    __asm {
        pushfd
        pushad
        call initializationMode
        mov dword ptr [esp + 28], eax
        popad
        popfd
        ret
    }
}

/** The status handler loads EDI, not EAX; retain every other register and the native flags. */
__declspec(naked) void modeForOutcome() {
    __asm {
        pushfd
        pushad
        call outcomeMode
        mov dword ptr [esp], eax
        popad
        popfd
        ret
    }
}

/** Restore the native registers and non-arithmetic flags, then return the replacement CMP flags. */
__declspec(naked) void compareCampaignMode() {
    __asm {
        push eax
        pushfd
        pushad
        call initializationMode
        mov dword ptr [esp + 28], eax
        popad
        popfd
        cmp eax, 1
        pop eax
        ret
    }
}
}

void patch::coop_campaign_init::install() {
    if (!patch::coop_campaign::mission()) return;
    static const bool installed = [] {
        // CWorld::sub_50C800 initializes campaign defaults, then skips carrier
        // fog/walls, resource scaling, time limits, availability, AI and alliances.
        // These seven reads are local to that function; no global mode is written.
        constexpr unsigned char eaxRead[] = {0xA1, 0x4B, 0xAD, 0x75, 0x00};
        constexpr unsigned char ediRead[] = {0x8B, 0x3D, 0x4B, 0xAD, 0x75, 0x00};
        constexpr unsigned char campaignCompare[] = {0x83, 0x3D, 0x4B, 0xAD, 0x75, 0x00, 0x01};
        // Instruction widths differ; every overwritten byte is guarded before any write.
        struct Gate { uintptr_t site; const unsigned char *expected; size_t size; void (*thunk)(); };
        const Gate gates[] = {
            {0x0050C803, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050C89D, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050C8FE, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050C982, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050C9C2, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050CB71, eaxRead, sizeof(eaxRead), modeForInitialization},
            {0x0050CBF6, eaxRead, sizeof(eaxRead), modeForInitialization},
            // Native ME-Heart cleanup and its following sole-survivor check use campaign rules.
            {0x004BB368, campaignCompare, sizeof(campaignCompare), compareCampaignMode},
            {0x004BB39A, campaignCompare, sizeof(campaignCompare), compareCampaignMode},
            // Authored status changes must not recursively award victory or defeat other Keepers.
            {0x004BEF14, ediRead, sizeof(ediRead), modeForOutcome}
        };
        for (const auto &gate : gates) {
            if (std::memcmp(reinterpret_cast<void *>(gate.site), gate.expected, gate.size)) std::abort();
        }
        for (const auto &gate : gates) {
            unsigned char call[] = {0xE8, 0, 0, 0, 0, 0x90, 0x90};
            const auto displacement = static_cast<uint32_t>(
                reinterpret_cast<uintptr_t>(gate.thunk) - gate.site - 5);
            std::memcpy(call + 1, &displacement, sizeof(displacement));
            DWORD oldProtection;
            if (!VirtualProtect(reinterpret_cast<void *>(gate.site), gate.size, PAGE_EXECUTE_READWRITE,
                &oldProtection)) std::abort();
            std::memcpy(reinterpret_cast<void *>(gate.site), call, gate.size);
            DWORD ignored;
            if (!VirtualProtect(reinterpret_cast<void *>(gate.site), gate.size, oldProtection, &ignored) ||
                !FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(gate.site), gate.size))
                std::abort();
        }
        return true;
    }();
    (void) installed;
}
