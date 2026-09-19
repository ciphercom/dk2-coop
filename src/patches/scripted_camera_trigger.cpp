#include "scripted_camera_hooks.h"
#include "dk2/entities/CPlayer.h"
#include "dk2/world/MyTriggerWhen.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstdlib>
#include <cstring>

/** Keep authored camera waits on shared ticks; native evaluation still updates trigger state. */
BOOL dk2::CPlayer::fun_4C7B20(uint32_t *rawTrigger) {
    // Incoming references are replaced, while the original DKII 1.70 body remains callable.
    static const bool verified = [] {
        const unsigned char prologue[] = {0x51, 0x8b, 0x44, 0x24, 0x08, 0x53, 0x55, 0x56, 0x8b, 0xf1};
        if (std::memcmp(reinterpret_cast<void *>(0x004C7B20), prologue, sizeof(prologue))) std::abort();
        return true;
    }();
    (void) verified;
    const auto original = reinterpret_cast<BOOL (__thiscall *)(CPlayer *, uint32_t *)>(0x004C7B20);
    const auto *trigger = reinterpret_cast<const MyTriggerWhen *>(rawTrigger);
    if (!patch::scripted_camera::correctsCondition(trigger->type)) return original(this, rawTrigger);
    if (!g_pCWorld) std::abort();
    auto *session = g_pCWorld->pGameSession;
    if (!session || !session->pBridge) std::abort();
    const auto *camera = session->pBridge->v_fD0_getCamera();
    if (!camera) std::abort();
    const BOOL originalResult = original(this, rawTrigger);
    return patch::scripted_camera::conditionResult(trigger->type, originalResult,
        camera->endTime, camera->_mode);
}
