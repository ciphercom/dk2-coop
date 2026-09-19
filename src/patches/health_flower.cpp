#include "health_flower.h"
#include "network_possession.h"
#include "coop_campaign.h"
#include "dk2/entities/CPlayer.h"
#include "dk2_globals.h"
#include <Windows.h>
#include <cstdlib>
#include <cstring>

namespace {
/** Rendering reads local camera state without ever spoofing the shared simulation's Keeper. */
bool __cdecl possessionDisplay(const dk2::CPlayer *keeper) {
    const bool native = keeper->creaturePossessed != 0;
    if (!patch::network_possession::enabled()) return native;
    const auto *session = dk2::CDefaultPlayerInterface_instance.pGameSession;
    if (!session || !session->pBridge) std::abort();
    const auto *camera = session->pBridge->v_fD0_getCamera();
    if (!camera) std::abort();
    return patch::health_flower::possessionDisplay(true, native, camera->_mode);
}

/** Preserve EAX's live creature flags and every register; replace only the native CMP result. */
__declspec(naked) void compareCreaturePossession() {
    __asm {
        push eax
        pushfd
        pushad
        push ebp
        call possessionDisplay
        add esp, 4
        mov byte ptr [esp + 28], al
        popad
        popfd
        cmp al, 0
        pop eax
        ret
    }
}

/** The mesh-scale site keeps its Keeper in EAX and a zero in ECX; neither may be clobbered. */
__declspec(naked) void compareMeshPossession() {
    __asm {
        push eax
        pushfd
        pushad
        push eax
        call possessionDisplay
        add esp, 4
        mov byte ptr [esp + 28], al
        popad
        popfd
        cmp al, 0
        pop eax
        ret
    }
}
}

void patch::health_flower::install() {
    if (!patch::coop_campaign::active()) return;
    static const bool installed = [] {
        // Keep all native creature eligibility rules and both original rendering branches.
        constexpr unsigned char creature[] = {0x66,0x83,0xBD,0x34,0x01,0,0,0};
        constexpr unsigned char mesh[] = {0x66,0x39,0x88,0x34,0x01,0,0};
        struct Gate { uintptr_t site; const unsigned char *expected; size_t size; void (*thunk)(); };
        const Gate gates[] = {
            {0x0048E8E1, creature, sizeof(creature), compareCreaturePossession},
            {0x00595416, mesh, sizeof(mesh), compareMeshPossession}
        };
        for (const auto &gate : gates) {
            if (std::memcmp(reinterpret_cast<void *>(gate.site), gate.expected, gate.size)) std::abort();
        }
        for (const auto &gate : gates) {
            unsigned char call[] = {0xE8,0,0,0,0,0x90,0x90,0x90};
            const auto displacement = static_cast<uint32_t>(
                reinterpret_cast<uintptr_t>(gate.thunk) - gate.site - 5);
            std::memcpy(call + 1, &displacement, sizeof(displacement));
            DWORD oldProtection;
            if (!VirtualProtect(reinterpret_cast<void *>(gate.site), gate.size,
                    PAGE_EXECUTE_READWRITE, &oldProtection)) std::abort();
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
