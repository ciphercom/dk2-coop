#include "network_hands.h"
#include "network_possession.h"
#include "dk2/entities/CPlayer.h"
#include "dk2_globals.h"
#include "dk2_functions.h"
#include <cstring>
#include <intrin.h>

namespace {
using namespace patch::network_hands;
Ownership ownership;
uint8_t dispatchOrigin = 0;
int dispatchKind = 0;

/** Check original entry points before using the loader's retained native bodies. */
void verifyOriginals() {
    static const bool verified = [] {
        const auto check = [](uintptr_t address, const unsigned char *bytes, size_t size) {
            invariant(std::memcmp(reinterpret_cast<void *>(address), bytes, size) == 0);
        };
        const unsigned char take[] = {0x83,0xEC,0x0C,0x33,0xC0};
        const unsigned char action[] = {0x53,0x8B,0x5C,0x24,0x08};
        const unsigned char save[] = {0x83,0xEC,0x10,0x8D,0x44,0x24,0x00};
        check(0x004BC500, take, sizeof(take));
        check(0x004C09F0, action, sizeof(action));
        check(0x004B8D40, save, sizeof(save));
        check(0x004B9250, save, sizeof(save));
        return true;
    }();
    (void) verified;
}
uint8_t localOrigin() { return originForSlot(dk2::WeaNetR_instance.playersSlot); }
dk2::CPlayer &keeper(dk2::CWorld &world, int rawTag) {
    // Match native getCTag: callers only initialize the low word of this stack argument.
    const auto tag = uint16_t(rawTag);
    invariant(tag > 0 && tag < 4096);
    auto *value = static_cast<dk2::CPlayer *>(world.v_getCTag_508C40(uint16_t(tag)));
    invariant(value && value->getVtbl() == dk2::CPlayer::vftable && value->thingsInHand_count <= 64);
    return *value;
}

/** These native callers are presentation only; world/AI/script queries retain the complete stack. */
bool localQuery(uintptr_t caller, int tag) {
    if (!enabled() || !isLocalKeeper(tag, dk2::CDefaultPlayerInterface_instance.playerTagId)) return false;
    switch (caller) {
    case 0x0040C043: case 0x0040C14A: case 0x0040D04A: case 0x0040D07C:
    case 0x0040D156: case 0x0040D6A8: case 0x0040D6C5: case 0x0040D6D1:
    case 0x0040E03D: case 0x0040E459:
        return true;
    default: return false;
    }
}

}

bool patch::network_hands::enabled() { return patch::network_possession::enabled(); }
void patch::network_hands::resetSession() { ownership.reset(); dispatchOrigin = 0; dispatchKind = 0; }
uint8_t patch::network_hands::owner(uint16_t tag) { return ownership.owner(tag); }
bool patch::network_hands::localOwns(uint16_t tag) { return !enabled() || ownership.visible(tag, localOrigin()); }
uint16_t patch::network_hands::localLatest(dk2::CPlayer &player) {
    const int index = ownership.latest(player.thingsInHand, player.thingsInHand_count, localOrigin());
    return index < 0 ? 0 : player.thingsInHand[index];
}
int patch::network_hands::dropIndex(dk2::CPlayer &player) {
    if (!enabled()) return int(player.thingsInHand_count) - 1;
    const auto origin = dropOrigin(uint32_t(player.inst__playerAction.evData3));
    invariant(origin <= 8);
    // Native/scripted drops still pop the whole Keeper's top entry.
    return origin == 0 ? int(player.thingsInHand_count) - 1 :
        ownership.latest(player.thingsInHand, player.thingsInHand_count, origin);
}

void patch::network_hands::tagAction(dk2::GameAction &action) {
    if (!enabled()) return;
    if (pickup(action.actionKind)) action.data1 = tagPickup(action.data1, localOrigin());
    else if (action.actionKind == 61) action.data3 = tagDrop(action.data3, localOrigin());
}

namespace {
/** Network ticks bypass CWorld::callActionHandler. Decode at each actual handler entry instead. */
int dispatchPickup(dk2::GameAction &action, uintptr_t entry) {
    const auto original = [&](dk2::GameAction *value) {
        return reinterpret_cast<int (__stdcall *)(dk2::GameAction *)>(entry)(value);
    };
    if (!enabled() || !pickup(action.actionKind)) return original(&action);
    verifyOriginals();
    invariant(!dispatchOrigin && !dispatchKind);
    dk2::GameAction native = action;
    dispatchOrigin = pickupOrigin(native.data1);
    invariant(dispatchOrigin <= 8);
    dispatchKind = native.actionKind;
    native.data1 &= ~0xF000u;
    const int result = original(&native);
    dispatchOrigin = 0; dispatchKind = 0;
    return result;
}
}

/** All native callers, including the direct network action table, must strip the marker before tag lookup. */
int dk2::GameActionHandler_N39(GameAction *action) { return dispatchPickup(*action, 0x00513480); }
int dk2::GameActionHandler_N3A(GameAction *action) { return dispatchPickup(*action, 0x005134E0); }
int dk2::GameActionHandler_N3B(GameAction *action) { return dispatchPickup(*action, 0x005135B0); }
int dk2::GameActionHandler_N3C(GameAction *action) { return dispatchPickup(*action, 0x00513600); }

/** Pickup actions 1/2/3 never use evData2; retaining origin there also survives delayed completion. */
int dk2::CPlayer::doPlayerAction_4C09F0(int type, int data1, int data2, int data3) {
    if (patch::network_hands::enabled()) {
        verifyOriginals();
        if (type >= 1 && type <= 3 && dispatchKind == (type == 1 ? 57 : type + 57)) {
            invariant(data2 == 0);
            data2 = dispatchOrigin;
        }
    }
    return reinterpret_cast<int (__thiscall *)(CPlayer *, int, int, int, int)>(0x004C09F0)(this, type, data1, data2, data3);
}

/** All three delayed pickup actions complete through act3; batch action58 inserts directly. */
int dk2::CPlayer::takeThingInHand(uint16_t tag) {
    uint8_t origin = 0;
    if (patch::network_hands::enabled()) {
        verifyOriginals();
        origin = completingOrigin(inst__playerAction.type, uint16_t(inst__playerAction.evData1),
            inst__playerAction.evData2, tag, dispatchKind, dispatchOrigin);
    }
    const int result = reinterpret_cast<int (__thiscall *)(CPlayer *, uint16_t)>(0x004BC500)(this, tag);
    if (result && patch::network_hands::enabled()) ownership.accepted(tag, origin);
    return result;
}

/** Ownership affects future drops, so it must participate in the shared simulation checksum. */
int dk2::CPlayer::calcChecksum() {
    uint32_t result = uint32_t(reinterpret_cast<int (__thiscall *)(CPlayer *)>(0x004BCB60)(this));
    if (patch::network_hands::enabled()) {
        result = ownership.checksum(result, thingsInHand, thingsInHand_count);
    }
    return int(result);
}

/** Extend native Keeper serialization only for the active co-op protocol. */
int dk2::CPlayer::fun_4B8D40(MyFile_Disc **file) {
    if (patch::network_hands::enabled()) verifyOriginals();
    const int result = reinterpret_cast<int (__thiscall *)(CPlayer *, MyFile_Disc **)>(0x004B8D40)(this, file);
    if (!result || !patch::network_hands::enabled()) return result;
    invariant(file && *file && thingsInHand_count <= 64);
    SavedOwners saved = ownership.save(thingsInHand, thingsInHand_count);
    int status = 0, written = 0;
    MyFile_Disc_writeBytes(&status, *file, &saved, sizeof(saved), &written);
    return status >= 0 && written == sizeof(saved);
}

/** Restore owners from the same stream as the native entities, including resync world transfers. */
int dk2::CPlayer::fun_4B9250(MyFile_Disc **file) {
    if (patch::network_hands::enabled()) verifyOriginals();
    const int result = reinterpret_cast<int (__thiscall *)(CPlayer *, MyFile_Disc **)>(0x004B9250)(this, file);
    if (!result || !patch::network_hands::enabled()) return result;
    invariant(file && *file);
    SavedOwners saved;
    int status = 0, read = 0;
    MyFile_Disc_readBytes(&status, *file, &saved, sizeof(saved), &read);
    if (status < 0 || read != sizeof(saved)) return 0;
    return ownership.load(thingsInHand, thingsInHand_count, saved);
}

BOOL dk2::CWorld::hasThingsInHand(int tag) {
    if (localQuery(uintptr_t(_ReturnAddress()), tag)) {
        auto &player = keeper(*this, tag);
        return ownership.count(player.thingsInHand, player.thingsInHand_count, localOrigin()) != 0;
    }
    return reinterpret_cast<BOOL (__thiscall *)(CWorld *, int)>(0x005094B0)(this, tag);
}
int dk2::CWorld::getThingsInHand_count(int tag) {
    if (localQuery(uintptr_t(_ReturnAddress()), tag)) {
        auto &player = keeper(*this, tag);
        return ownership.count(player.thingsInHand, player.thingsInHand_count, localOrigin());
    }
    return reinterpret_cast<int (__thiscall *)(CWorld *, int)>(0x005094D0)(this, tag);
}
int dk2::CWorld::getThingInPlayerHand(int tag, uint32_t index, uint16_t *out) {
    if (localQuery(uintptr_t(_ReturnAddress()), tag)) {
        invariant(out);
        auto &player = keeper(*this, tag);
        const auto held = ownership.at(player.thingsInHand, player.thingsInHand_count, localOrigin(), index);
        if (!held) return 0;
        *out = held;
        return 1;
    }
    return reinterpret_cast<int (__thiscall *)(CWorld *, int, uint32_t, uint16_t *)>(0x005094F0)(this, tag, index, out);
}

/** Two native cursor routines bypass CWorld and request the global last slot directly. */
int dk2::CPlayer::fun_4BCAB0(uint32_t index, uint16_t *out) {
    const auto caller = uintptr_t(_ReturnAddress());
    if (patch::network_hands::enabled() && f0_tagId == CDefaultPlayerInterface_instance.playerTagId &&
        (caller == 0x0040AF59 || caller == 0x0040E4B3)) {
        invariant(out);
        const auto held = localLatest(*this);
        if (!held) return 0;
        *out = held;
        return 1;
    }
    return reinterpret_cast<int (__thiscall *)(CPlayer *, uint32_t, uint16_t *)>(0x004BCAB0)(this, index, out);
}

/** Pending requests are already local; exclude a request whose creature the partner won. */
BOOL dk2::CDefaultPlayerInterface::hasThingsInHand(int tag) {
    if (!patch::network_hands::enabled() || !isLocalKeeper(tag, playerTagId))
        return reinterpret_cast<BOOL (__thiscall *)(CDefaultPlayerInterface *, int)>(0x0040DFF0)(this, tag);
    auto &player = keeper(*pCWorld, tag);
    invariant(thingsInHand_count <= 65);
    for (unsigned i = 0; i < thingsInHand_count; ++i) {
        const auto &entry = thingsInHand[i];
        if (!entry.hasUnderHand && (!player.hasThingInHand(entry.tagId) || localOwns(entry.tagId))) return TRUE;
    }
    return localLatest(player) != 0;
}
uint16_t dk2::CDefaultPlayerInterface::sub_40E050() {
    if (!patch::network_hands::enabled())
        return reinterpret_cast<uint16_t (__thiscall *)(CDefaultPlayerInterface *)>(0x0040E050)(this);
    auto &player = keeper(*pCWorld, playerTagId);
    invariant(thingsInHand_count <= 65);
    for (unsigned i = 0; i < thingsInHand_count; ++i) {
        const auto &entry = thingsInHand[i];
        if (!entry.hasUnderHand && (!player.hasThingInHand(entry.tagId) || localOwns(entry.tagId))) return entry.tagId;
    }
    return localLatest(player);
}
