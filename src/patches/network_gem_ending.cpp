#include "coop_campaign.h"
#include "coop_campaign_progress.h"
#include "network_gem_ending.h"
#include "diagnostic/game_bridge.h"
#include "scripted_camera_hooks.h"
#include "dk2/CState.h"
#include "dk2/Obj6F2550.h"
#include "dk2/entities/CPlayer.h"
#include "dk2/entities/CCreature.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstdlib>
#include <cstring>

namespace {
patch::network_gem_ending::Ending ending;
unsigned traceLines = 0;
int lastPathWait = -1, lastMoveWait = -1;

/** These fixed continuations are valid only for the exact DKII 1.70 native frames. */
template<size_t N> void expectBytes(uintptr_t address, const unsigned char (&bytes)[N]) {
    if (std::memcmp(reinterpret_cast<void *>(address), bytes, N)) std::abort();
}
void verifyNativeFrames() {
    static const bool verified = [] {
        const unsigned char status[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0xA0,0xC5,0x64,0,0x50};
        const unsigned char start[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0xD0,0xC5,0x64,0,0x50,
            0xA1,0x4B,0xAD,0x75,0,0x64,0x89,0x25,0,0,0,0,0x83,0xEC,0x30,0x83,0xF8,3,
            0x53,0x55,0x56,0x57,0x8B,0xE9,0x0F,0x84,0x51,5,0,0};
        const unsigned char alternative[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0xF0,0xC5,0x64,0,0x50,
            0xA1,0x4B,0xAD,0x75,0,0x64,0x89,0x25,0,0,0,0,0x83,0xEC,0x14,0x83,0xF8,3,
            0x53,0x55,0x56,0x57,0x8B,0xF1,0x0F,0x84,0x60,1,0,0};
        const unsigned char alternativeReady[] = {0x66,0x83,0xBE,0x34,1,0,0,0};
        const unsigned char alternativeReturn[] = {0x8B,0x4C,0x24,0x24,0x5F,0x5E,0x5D,
            0x64,0x89,0x0D,0,0,0,0,0x5B,0x83,0xC4,0x20,0xC3};
        const unsigned char startReady[] = {0x33,0xC0,0x33,0xC9,0x66,0x8B,0x85,0x43,0x0B,0,0};
        const unsigned char startReturn[] = {0x8B,0x4C,0x24,0x40,0x5F,0x5E,0x5D,0x64,0x89,0x0D,0,0,0,0,
            0x5B,0x83,0xC4,0x3C,0xC3};
        const unsigned char path[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0xE8,0xAF,0x64,0,0x50,
            0x64,0x89,0x25,0,0,0,0,0x83,0xEC,0x14,0x56,0x8B,0xF1,0x57};
        const unsigned char pathReady[] = {0x8B,0x46,0x1C,0xB9,4,0,0,0,0x3B,0xC1};
        const unsigned char pathReturn[] = {0x8B,0x4C,0x24,0x1C,0x5F,0xB8,1,0,0,0,0x64,0x89,0x0D,0,0,0,0,
            0x5E,0x83,0xC4,0x20,0xC3};
        const unsigned char move[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0x10,0xB0,0x64,0,0x50,
            0x64,0x89,0x25,0,0,0,0,0x83,0xEC,0x1C,0x53,0x55,0x56,0x8B,0xF1,0x57};
        const unsigned char moveReady[] = {0x8B,0x46,8,0x8B,0x4E,0x1C,0x83,0xF9,1,0x8D,0x78,0x44};
        const unsigned char moveReturn[] = {0x8B,0x4C,0x24,0x2C,0x5F,0x5E,0x5D,0xB8,1,0,0,0,
            0x64,0x89,0x0D,0,0,0,0,0x5B,0x83,0xC4,0x28,0xC3};
        const unsigned char complete[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0x38,0xB0,0x64,0,0x50};
        expectBytes(0x004BE630, status);
        expectBytes(0x004BF9E0, alternative); expectBytes(0x004BFA0C, alternativeReady);
        expectBytes(0x004BFB6C, alternativeReturn);
        expectBytes(0x004BF410, start); expectBytes(0x004BF43C, startReady); expectBytes(0x004BF98D, startReturn);
        expectBytes(0x00474100, path); expectBytes(0x0047413B, pathReady); expectBytes(0x004741B1, pathReturn);
        expectBytes(0x004741D0, move); expectBytes(0x00474211, moveReady); expectBytes(0x0047439D, moveReturn);
        expectBytes(0x004744F0, complete);
        return true;
    }();
    (void) verified;
}

// These handlers belong to the unchanged legacy DKII module, not Flame's SafeSEH table.
// The byte guards above verify their native frames; keep this warning exception local to the thunks.
#pragma warning(push)
#pragma warning(disable: 4733)

/** Reproduce the native SEH chain, local allocation and saved registers before bypassing only mode3. */
__declspec(naked) int __fastcall startNative(dk2::CPlayer *) {
    __asm {
        mov eax, dword ptr fs:[0]
        push -1
        push 0064C5D0h
        push eax
        mov dword ptr fs:[0], esp
        sub esp, 30h
        push ebx
        push ebp
        push esi
        push edi
        mov ebp, ecx
        mov eax, 004BF43Ch
        jmp eax
    }
}

/** Preserve the alternate native ending's frame and shared effects, bypassing only mode3. */
__declspec(naked) void __fastcall startAlternativeNative(dk2::CPlayer *) {
    __asm {
        mov eax, dword ptr fs:[0]
        push -1
        push 0064C5F0h
        push eax
        mov dword ptr fs:[0], esp
        sub esp, 14h
        push ebx
        push ebp
        push esi
        push edi
        mov esi, ecx
        mov eax, 004BFA0Ch
        jmp eax
    }
}

/** E5 keeps its original ready body and epilogue; its local mode predicate is supplied by shared timing. */
__declspec(naked) int __fastcall pathReadyNative(dk2::CState *) {
    __asm {
        mov eax, dword ptr fs:[0]
        push -1
        push 0064AFE8h
        push eax
        mov dword ptr fs:[0], esp
        sub esp, 14h
        push esi
        mov esi, ecx
        push edi
        mov eax, 0047413Bh
        jmp eax
    }
}

/** E6 preserves all native creature/camera effects after the shared movement wait. */
__declspec(naked) int __fastcall moveReadyNative(dk2::CState *) {
    __asm {
        mov eax, dword ptr fs:[0]
        push -1
        push 0064B010h
        push eax
        mov dword ptr fs:[0], esp
        sub esp, 1Ch
        push ebx
        push ebp
        push esi
        mov esi, ecx
        push edi
        mov eax, 00474211h
        jmp eax
    }
}

#pragma warning(pop)

void trace(const char *event, uint16_t keeper = 0, uint16_t creature = 0, int pending = -1) {
    if (!patch::diagnostic::enabled() || traceLines >= 128) return;
    ++traceLines;
    dk2::MyWindow_log_printf(&dk2::MyWindow_instance,
        "[network-gem-ending] event=%s tick=%u keeper=%u creature=%u pending=%d\n", event,
        unsigned(dk2::g_pCWorld->getGameTick()), unsigned(keeper), unsigned(creature), pending);
}

/** The native routine can letterbox even when creature creation/state entry fails. */
uint16_t startedCreature(dk2::CPlayer &keeper) {
    if (!keeper.gemId || !(keeper.playerFlags & 0x40)) return 0;
    uint16_t result = 0;
    for (uint16_t tag = keeper.ownedCreature_first; tag;) {
        auto *creature = static_cast<dk2::CCreature *>(dk2::g_pCWorld->v_getCTag_508C40(tag));
        if (!creature || creature->f26_pPlayer_owner != &keeper) std::abort();
        if (creature->cstate.currentStateId == 0xE4) {
            if (result) std::abort();
            result = tag;
        }
        tag = creature->fC_playerNodeY;
    }
    return result;
}

bool matches(const dk2::CState &state) {
    return state.creature && state.creature->f26_pPlayer_owner &&
        ending.matches(state.creature->f26_pPlayer_owner->f0_tagId, state.creature->f0_tagId);
}
}

bool patch::network_gem_ending::enabled() {
    if (!patch::coop_campaign::active()) return false;
    const auto &config = dk2::MyResources_instance.gameCfg;
    if (config.useFe_playMode != 3 || config.useFe3d || config.useFe2d_unk1 || config.hasSaveFile) return false;
    if (!patch::scripted_camera::enabledForSession()) {
        dk2::MyWindow_log_printf(&dk2::MyWindow_instance,
            "[network-gem-ending] active co-op requires shared scripted-camera timing\n");
        std::abort();
    }
    return true;
}
bool patch::network_gem_ending::activeForKeeper(uint16_t tag) { return enabled() && ending.activeFor(tag); }
bool patch::network_gem_ending::alternativeForKeeper(uint16_t tag) { return enabled() && ending.alternativeFor(tag); }
bool patch::network_gem_ending::physicalCompleteForKeeper(uint16_t tag) { return enabled() && ending.completeFor(tag); }
void patch::network_gem_ending::resetSession() {
    ending.reset(); traceLines = 0; lastPathWait = lastMoveWait = -1;
}
void patch::network_gem_ending::beforeWorldTick(dk2::MyGameSession &session) {
    if (!enabled()) return;
    if (!session.pWorld) std::abort();
    if (ending.observeTick(session.pWorld->getGameTick())) {
        lastPathWait = lastMoveWait = -1;
        trace("rewind-reset");
    }
    // CPlayer::tick clears this shared deadline after emitting the native alternate effects.
    if (const auto tag = ending.alternativeKeeper()) {
        const auto *keeper = static_cast<dk2::CPlayer *>(session.pWorld->v_getCTag_508C40(tag));
        if (!keeper) std::abort();
        if (ending.completeAlternative(tag, keeper->fireworksTime == 0))
            trace("alternative-complete", tag);
    }
}

/** Preserve the authoritative status transition and its ABI/result; the native network branch omits the cinematic. */
int16_t dk2::CPlayer::fun_4BE630(int status) {
    patch::coop_campaign_progress::install();
    const auto original = [&] {
        return reinterpret_cast<int16_t (__thiscall *)(CPlayer *, int)>(0x004BE630)(this, status);
    };
    if (!patch::network_gem_ending::enabled()) return original();
    verifyNativeFrames();
    if (!g_pCWorld) std::abort();
    // The native campaign dispatcher checks the local Keeper before either cinematic.
    // Apply that guard before reserving an ending, so another winner cannot claim it.
    const bool eligible = patch::network_gem_ending::eligibleWinner(f0_tagId, g_pCWorld->v_getMEPlayerTagId());
    return ending.transition(eligible, *this, status, original, [&] {
        if (patch::network_gem_ending::usesAlternativeEnding(dk2::Obj6F2550_instance.f407)) {
            startAlternativeNative(this);
            if (!fireworksTime || !(playerFlags & 0x40)) std::abort();
            trace("alternative-started", f0_tagId);
            return patch::network_gem_ending::Started{0, true};
        }
        startNative(this);
        const auto creature = startedCreature(*this);
        trace(creature ? "started" : "native-fallback", f0_tagId, creature);
        return patch::network_gem_ending::Started{creature};
    });
}

int dk2::CState::sub_474100() {
    const auto original = [&] { return reinterpret_cast<int (__thiscall *)(CState *)>(0x00474100)(this); };
    if (!patch::network_gem_ending::enabled() || !matches(*this)) return original();
    verifyNativeFrames();
    const bool pending = patch::scripted_camera::pathPending();
    if (lastPathWait != int(pending)) {
        lastPathWait = pending;
        trace("path-wait", creature->f26_pPlayer_owner->f0_tagId, creature->f0_tagId, pending);
    }
    return patch::network_gem_ending::wait(true, true, pending, age, original, [&] { return pathReadyNative(this); });
}
int dk2::CState::sub_4741D0() {
    const auto original = [&] { return reinterpret_cast<int (__thiscall *)(CState *)>(0x004741D0)(this); };
    if (!patch::network_gem_ending::enabled() || !matches(*this)) return original();
    verifyNativeFrames();
    const bool pending = patch::scripted_camera::movementPending();
    if (lastMoveWait != int(pending)) {
        lastMoveWait = pending;
        trace("movement-wait", creature->f26_pPlayer_owner->f0_tagId, creature->f0_tagId, pending);
    }
    return patch::network_gem_ending::wait(true, true, pending, age, original, [&] { return moveReadyNative(this); });
}
int dk2::CState::sub_4744F0() {
    const bool scoped = patch::network_gem_ending::enabled();
    if (scoped) verifyNativeFrames();
    const int result = reinterpret_cast<int (__thiscall *)(CState *)>(0x004744F0)(this);
    if (scoped && matches(*this) && ending.complete(creature->f26_pPlayer_owner->f0_tagId,
        creature->f0_tagId, initiatedEndOfLevelGems != 0))
        trace("physical-complete", creature->f26_pPlayer_owner->f0_tagId, creature->f0_tagId);
    return result;
}
