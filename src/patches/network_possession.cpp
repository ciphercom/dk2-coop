#include "coop_campaign.h"
#include "network_possession.h"
#include "network_hands.h"
#include "dk2/MyCreatureCollection.h"
#include "dk2/entities/CPlayer.h"
#include "dk2/entities/CShot.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstring>

namespace {
patch::network_possession::Presentation presentation;
// Native cast allocation and shot processing are synchronous within these separate scopes.
uint32_t castingOrigin = 0, enteringOrigin = 0;

/** The loader redirects references, leaving these original DKII 1.70 bodies callable. */
template<size_t N> void expectBytes(uintptr_t address, const unsigned char (&bytes)[N]) {
    if (std::memcmp(reinterpret_cast<void *>(address), bytes, N)) std::abort();
}
void verifyOriginals() {
    static const bool verified = [] {
        const unsigned char push[] = {0x8B,0x44,0x24,4,0x83,0xC1,0x1C};
        const unsigned char cast[] = {0x8B,0x44,0x24,4,0x8B,8,0x66,0x8B,0x50,4};
        const unsigned char allocate[] = {0x53,0x8B,0x5C,0x24,8,0x56,0x57,0x8B,0xF9};
        const unsigned char shot[] = {0x64,0xA1,0,0,0,0,0x6A,0xFF,0x68,0x60,0xBF,0x64,0,0x50};
        const unsigned char entry[] = {0x8B,0x44,0x24,4,0x53,0x56,0x8B,0xF1,0x66,0x8B,0x58,4};
        const unsigned char exit[] = {0x53,0x8B,0x5C,0x24,8,0x56,0x8B,0xF1,0x57};
        expectBytes(0x00527130, push); expectBytes(0x00513340, cast);
        expectBytes(0x004B7CD0, allocate); expectBytes(0x004AEEA0, shot);
        expectBytes(0x004483D0, entry); expectBytes(0x00448440, entry);
        expectBytes(0x00442820, exit);
        const unsigned char savedMode[] = {0x8B,0x83,0xD2,0x0E,0,0,0x89,0x83,0xA8,0x0D,0,0};
        const unsigned char pathDefers[] = {0x83,0xBE,0xA8,0x0D,0,0,0x12,0x74,0x18};
        const unsigned char deferredMode[] = {0x66,0x8B,0x4C,0x24,0x14,0x89,0xAE,0xB0,0x0D,0,0,
            0x66,0x89,0x8E,0xB4,0x0D,0,0};
        expectBytes(0x0044D90D, savedMode); expectBytes(0x0044A799, pathDefers);
        expectBytes(0x0044A7BA, deferredMode);
        return true;
    }();
    (void) verified;
}
uint32_t localOrigin() { return patch::network_possession::originForSlot(dk2::WeaNetR_instance.playersSlot); }

/** Bridge B/C are emitted only after native possession acceptance, with creature/ Keeper in the first two dwords. */
bool allowEntry(dk2::CBridge &bridge, uint16_t *data) {
    if (!patch::network_possession::enabled()) return true;
    const auto keeper = data[2], creature = data[0];
    const bool allow = presentation.enter(true, enteringOrigin, localOrigin(), keeper,
        bridge.v_fBC_getPlayerId(), creature);
    return allow;
}
}

bool patch::network_possession::enabled() {
    if (!patch::coop_campaign::active()) return false;
    const auto &config = dk2::MyResources_instance.gameCfg;
    return config.useFe_playMode == 3 && !config.useFe3d && !config.useFe2d_unk1 && !config.hasSaveFile;
}
void patch::network_possession::resetSession() {
    presentation.reset(); castingOrigin = enteringOrigin = 0;
}

void patch::network_possession::afterWorldReload() {
    presentation.clearPendingShots();
}

/** Observe shared state after every tick, covering local exit, remote exit, death and destruction. */
void patch::network_possession::afterWorldTick(dk2::MyGameSession &session) {
    if (!enabled()) return;
    if (!session.pPlayer || !session.pWorld || !session.pBridge) std::abort();
    const auto keeperTag = session.pPlayer->playerTagId;
    if (keeperTag >= 4096) std::abort();
    auto *keeper = static_cast<dk2::CPlayer *>(session.pWorld->v_getCTag_508C40(keeperTag));
    const auto creature = keeper ? keeper->creaturePossessed : uint16_t(0);
    presentation.observe(keeperTag, creature);
    auto *camera = session.pBridge->v_fD0_getCamera();
    if (!camera) std::abort();
    uint32_t savedMode = 0;
    if (camera->_mode == 18) {
        verifyOriginals();
        // Native finish reads camera+ED2, represented as the adjacent gap in the generated CBridge layout.
        if (camera != &session.pBridge->camera) std::abort();
        static_assert(sizeof(dk2::CCamera) == 0xED2);
        std::memcpy(&savedMode, session.pBridge->f12b7_gap, sizeof(savedMode));
    }
    if (presentation.releaseCamera(camera->_mode, camera->fDB0, savedMode)) {
        verifyOriginals();
        // Native mode18 queues the release as3/0 while retaining the full authored path.
        // Clear a dead deferred target now: path completion would dereference it before the next world return.
        // This is only the native local camera/audio release, never world action21.
        uint32_t release[] = {0, 0, keeperTag};
        reinterpret_cast<int (__thiscall *)(dk2::CBridge *, uint32_t *)>(0x00442820)(session.pBridge, release);
    }
}

/** The other Controller keeps an absolute dungeon cursor while the shared Keeper possesses a creature. */
bool patch::network_possession::relativeMouse(bool nativePossessed) {
    if (!enabled()) return nativePossessed;
    const auto &controller = dk2::CDefaultPlayerInterface_instance;
    if (!controller.pGameSession || !controller.pGameSession->pBridge || controller.playerTagId >= 4096) return false;
    auto *keeper = static_cast<dk2::CPlayer *>(dk2::sceneObjects[controller.playerTagId]);
    auto *camera = controller.pGameSession->pBridge->v_fD0_getCamera();
    return keeper && camera && (camera->_mode == 1 || camera->_mode == 2) &&
        presentation.owns(controller.playerTagId, keeper->creaturePossessed);
}

/** action55 uses data1 and data2 only; its data3 carries origin through the unchanged 18-byte network action. */
BOOL dk2::MyGameSession::pushAction(GameAction *action) {
    const auto original = [&](GameAction *value) {
        return reinterpret_cast<BOOL (__thiscall *)(MyGameSession *, GameAction *)>(0x00527130)(this, value);
    };
    if (patch::network_hands::enabled() &&
        (patch::network_hands::pickup(action->actionKind) || action->actionKind == 61)) {
        verifyOriginals();
        GameAction tagged = *action;
        patch::network_hands::tagAction(tagged);
        return original(&tagged);
    }
    if (!patch::network_possession::enabled() || action->actionKind != 55 || (action->data1 & 0xFF) != 2)
        return original(action);
    verifyOriginals();
    GameAction tagged = *action;
    tagged.data3 = localOrigin();
    return original(&tagged);
}

/** Cast success is asynchronous: only allocation associates origin, and only native B/C grants presentation. */
int dk2::GameActionHandler_N37(GameAction *action) {
    const auto original = [&](GameAction *value) {
        return reinterpret_cast<int (__stdcall *)(GameAction *)>(0x00513340)(value);
    };
    if (!patch::network_possession::enabled() || (action->data1 & 0xFF) != 2) return original(action);
    verifyOriginals();
    if (castingOrigin) std::abort();
    castingOrigin = patch::network_possession::validOrigin(action->data3) ? action->data3 : 0;
    GameAction native = *action;
    native.data3 = 0;
    const int result = original(&native);
    castingOrigin = 0;
    return result;
}

/** Clear reused shot tags even for unrelated casts; no Controller metadata touches world entities. */
int dk2::MyCreatureCollection::createCShot(int type, int keeper, Vec3i *position,
        uint16_t *tag, CShot **shot, int flags) {
    if (patch::network_possession::enabled()) verifyOriginals();
    const int result = reinterpret_cast<int (__thiscall *)(MyCreatureCollection *, int, int, Vec3i *, uint16_t *, CShot **, int)>
        (0x004B7CD0)(this, type, keeper, position, tag, shot, flags);
    if (result && patch::network_possession::enabled()) {
        if (!shot || !*shot) std::abort();
        presentation.allocatedShot((*shot)->f0_tagId, castingOrigin);
    }
    return result;
}

/** Native occupied-Keeper rejection stays intact; rejected shots consume their origin without camera entry. */
int dk2::CShot::sub_4AEEA0() {
    const auto original = [&] { return reinterpret_cast<int (__thiscall *)(CShot *)>(0x004AEEA0)(this); };
    if (!patch::network_possession::enabled()) return original();
    verifyOriginals();
    if (enteringOrigin) std::abort();
    enteringOrigin = presentation.consumeShot(f0_tagId);
    const int result = original();
    enteringOrigin = 0;
    return result;
}
int dk2::CBridge::idx_handler_B(uint16_t *data) {
    if (!allowEntry(*this, data)) return 1;
    if (patch::network_possession::enabled()) verifyOriginals();
    return reinterpret_cast<int (__thiscall *)(CBridge *, uint16_t *)>(0x004483D0)(this, data);
}
int dk2::CBridge::idx_handler_C(uint16_t *data) {
    if (!allowEntry(*this, data)) return 1;
    if (patch::network_possession::enabled()) verifyOriginals();
    return reinterpret_cast<int (__thiscall *)(CBridge *, uint16_t *)>(0x00448440)(this, data);
}
