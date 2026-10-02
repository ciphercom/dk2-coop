#include "possession_navigation_context.h"
#include "network_possession.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
/** The native local update takes its Controller in ECX, despite the generated cdecl declaration. */
void * __fastcall localMovement(void *controller, void *) {
    const auto original = [controller] {
        return reinterpret_cast<void * (__thiscall *)(void *)>(0x0040E910)(controller);
    };
    if (!patch::network_possession::enabled()) return original();
    static const bool verified = [] {
        constexpr unsigned char entry[] = {0x55,0x8B,0xEC,0x6A,0xFF,0x68,0xD8,0x92,0x64,0};
        if (std::memcmp(reinterpret_cast<void *>(0x0040E910), entry, sizeof(entry))) std::abort();
        // The loader must redirect the sole incoming CALL to this ABI-preserving thunk.
        const auto *call = reinterpret_cast<const unsigned char *>(0x00406779);
        uint32_t displacement;
        std::memcpy(&displacement, call + 1, sizeof(displacement));
        if (*call != 0xE8 || uintptr_t(0x0040677Eu + displacement) !=
                reinterpret_cast<uintptr_t>(&dk2::CDefaultPlayerInterface_sub_40E910)) std::abort();
        return true;
    }();
    (void) verified;
    // 40F6B0 selects this capability for local terrain probes. AI 4D5A40 refreshes
    // the other five context globals but inherits this one. Restore only after
    // the entire local update, retaining its native downstream collision queries.
    return patch::possession_navigation::localMovement(true,
        dk2::g_MyPilotNavigation__6EC9E4, original);
}
}

/** Preserve ECX and the original no-argument stack frame while exporting the generated symbol. */
__declspec(naked) void * __cdecl dk2::CDefaultPlayerInterface_sub_40E910() {
    __asm { jmp localMovement }
}
