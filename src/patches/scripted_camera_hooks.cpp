#include "coop_campaign.h"
#include "scripted_camera_hooks.h"
#include "scripted_camera_playback.h"
#include "network_possession.h"
#include "dk2/CCamera.h"
#include "dk2/CWorld.h"
#include "dk2/GameAction.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstring>

namespace {
patch::scripted_camera::Playback playback;
patch::scripted_camera::MotionWaits motion;

/** Loader rewrites incoming references, preserving the DKII 1.70 original bodies. */
void verifyOriginalEntries() {
    static const bool verified = [] {
        const unsigned char load[] = {0x81, 0xec, 0x0c, 0x01, 0x00, 0x00, 0x53, 0x56, 0x8b, 0xf1, 0x57};
        const unsigned char loadReady[] = {0x8b,0xbc,0x24,0x28,0x01,0,0,0x8b,0x8e,0x3c,0x0e,0,0};
        const unsigned char tween[] = {0x56,0x57,0x8b,0x7c,0x24,0x0c,0x8b,0xf1,0x85,0xff};
        const unsigned char finish[] = {0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x6a, 0xff};
        const unsigned char dispatch[] = {0x53, 0x56, 0x57, 0x8b, 0xf9, 0x8b, 0x5c, 0x24, 0x10, 0x8b, 0x47, 0x04};
        if (std::memcmp(reinterpret_cast<void *>(0x0044A370), load, sizeof(load)) ||
            std::memcmp(reinterpret_cast<void *>(0x0044A3B5), loadReady, sizeof(loadReady)) ||
            std::memcmp(reinterpret_cast<void *>(0x0044BD70), tween, sizeof(tween)) ||
            std::memcmp(reinterpret_cast<void *>(0x0044D8F0), finish, sizeof(finish)) ||
            std::memcmp(reinterpret_cast<void *>(0x0050E5C0), dispatch, sizeof(dispatch))) std::abort();
        return true;
    }();
    (void) verified;
}

/** Keep the original ret16 frame and mode intact, bypassing only local mode/tween rejection. */
__declspec(naked) char __fastcall loadSharedPath(dk2::CCamera *, void *, uint32_t, uint32_t, uint32_t, int) {
    __asm {
        sub esp, 10Ch
        push ebx
        push esi
        mov esi, ecx
        push edi
        xor ebx, ebx
        mov eax, 0044A3B5h
        jmp eax
    }
}

/** Disabled calls must not depend on initialized world/session objects. */
bool enabled() {
    if (!patch::coop_campaign::active()) return false;
    const auto &config = dk2::MyResources_instance.gameCfg;
    return patch::scripted_camera::enabledFor(true, config.useFe_playMode,
        config.useFe3d != 0, config.useFe2d_unk1 != 0, config.hasSaveFile != 0);
}

/** Use the original routine's saved mode, final transform, and camera cleanup. */
int finishOriginal(dk2::CCamera &camera) {
    verifyOriginalEntries();
    return reinterpret_cast<int (__thiscall *)(dk2::CCamera *)>(0x0044D8F0)(&camera);
}
}

bool patch::scripted_camera::enabledForSession() { return enabled(); }
bool patch::scripted_camera::pathPending() { return playback.pending(); }
bool patch::scripted_camera::movementPending() { return motion.movementPending(); }

void patch::scripted_camera::resetSession() {
    playback.reset();
    motion.reset();
}

void patch::scripted_camera::beforeWorldTick(dk2::MyGameSession &session) {
    if (!enabled()) {
        playback.reset();
        motion.reset();
        return;
    }
    if (!session.pWorld || !session.pBridge) std::abort();
    auto *camera = session.pBridge->v_fD0_getCamera();
    if (!camera) std::abort();
    const auto tick = session.pWorld->getGameTick();
    // [path start, path finish) remains paused; finishing now does not consume an extra tick.
    motion.advance(tick, playback.pending());
    playback.beforeTick(true, *camera, tick, [&] { return finishOriginal(*camera); });
    motion.advance(tick, playback.pending());
}

bool patch::scripted_camera::correctsCondition(int type) { return type == 72 && enabled(); }

int patch::scripted_camera::conditionResult(int type, int original, int endTime, unsigned mode) {
    return motion.condition(enabled(), type, endTime, mode, original, patch::network_possession::enabled());
}

/** ABI 0044A370, ret16; the original return byte does not indicate acceptance. */
char dk2::CCamera::loadEnginePath(uint32_t path, uint32_t x, uint32_t y, int flags) {
    verifyOriginalEntries();
    const auto original = [&] {
        return reinterpret_cast<char (__thiscall *)(CCamera *, uint32_t, uint32_t, uint32_t, int)>(0x0044A370)
            (this, path, x, y, flags);
    };
    if (!enabled()) return original();
    if (!g_pCWorld || !g_pCWorld->pGameSession) std::abort();
    const auto tick = g_pCWorld->getGameTick();
    const auto ticksPerSecond = g_pCWorld->pGameSession->gameTicksPerSecond;
    motion.advance(tick, playback.pending());
    const char result = playback.load(true, *this, tick, ticksPerSecond, [&](bool sharedAcceptance) {
        // Native set-mode18 saves the real previous mode, including possession,
        // for native completion; spoofing Keeper mode here would lose that return.
        if (!sharedAcceptance) return original();
        return patch::scripted_camera::loadShared(*this,
            [&] { reinterpret_cast<void (__thiscall *)(CCamera *, int)>(0x0044BD70)(this, 0); },
            [&] { return loadSharedPath(this, nullptr, path, x, y, flags); });
    });
    motion.advance(tick, playback.pending());
    return result;
}

/** Natural rendering and local skip requests cannot release a shared wait early. */
int dk2::CCamera::sub_44D8F0() {
    return playback.complete(enabled(), *this, [&] { return finishOriginal(*this); });
}

/** ABI 0050E5C0, ret4. Local interface camera input bypasses this shared action seam. */
int dk2::CWorld::callActionHandler(GameAction *action) {
    verifyOriginalEntries();
    const auto original = [&] {
        return reinterpret_cast<int (__thiscall *)(CWorld *, GameAction *)>(0x0050E5C0)(this, action);
    };
    if (!enabled() || action->actionKind != 4) return original();
    if (!pGameSession) std::abort();
    const auto tick = getGameTick();
    const auto rotationMs = action->data3;
    const auto ticksPerSecond = pGameSession->gameTicksPerSecond;
    // Acceptance and timing come from the shared command, never the local camera's mode/distance.
    const int result = motion.dispatch(true, action->actionKind, rotationMs, tick,
        ticksPerSecond, playback.pending(), original);
    return result;
}
