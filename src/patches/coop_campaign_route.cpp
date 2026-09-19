#include "coop_campaign.h"
#include "dk2/settings/GameCfg.h"
#include <cstdlib>
#include <cwchar>

namespace {
bool selected = false;
patch::coop_campaign::Mission selectedMission{};
// The unused session metadata word identifies this campaign route without changing native map handshakes.
constexpr uint16_t campaignTag = 0x4300;
}
bool patch::coop_campaign::active() { return selected; }
void patch::coop_campaign::enter() { selectedMission = {}; selected = true; }
void patch::coop_campaign::leave() { selected = false; selectedMission = {}; }
void patch::coop_campaign::returnToFrontend() { selectedMission = {}; }
const patch::coop_campaign::Mission *patch::coop_campaign::mission() {
    return selected && selectedMission.nativeId ? &selectedMission : nullptr;
}
void patch::coop_campaign::selectMission(int nativeId, const wchar_t *assetName, int keeperId) {
    // Callers resolve only native campaign selections; malformed packets are rejected before this boundary.
    if (!selected || nativeId < 1 || nativeId > 30 || nativeId == 25 || !assetName ||
        !assetName[0] || std::wcslen(assetName) >= 64 || keeperId < 1 || keeperId > 7) std::abort();
    selectedMission = {};
    selectedMission.nativeId = nativeId;
    selectedMission.keeperId = keeperId;
    std::wcscpy(selectedMission.assetName, assetName);
}
int16_t patch::coop_campaign::sessionTag() {
    return mission() ? static_cast<int16_t>(campaignTag | selectedMission.nativeId) : 0;
}
int patch::coop_campaign::missionIdFromSessionTag(int16_t tag) {
    const auto bits = static_cast<uint16_t>(tag);
    const int nativeId = bits & 0xFF;
    return (bits & 0xFF00) == campaignTag && nativeId >= 1 && nativeId <= 30 && nativeId != 25
        ? nativeId : 0;
}
bool patch::coop_campaign::apply(dk2::GameCfg &config) {
    // The native lobby still transports its proven two-peer carrier. Never silently
    // convert a different session reached through an unintended menu path.
    if (!mission() || config.useFe_playMode != 3 || config.useFe3d ||
        config.useFe2d_unk1 || config.hasSaveFile || _wcsnicmp(config.levelName, L"Gonzalez", 64) != 0)
        return false;
    std::wcscpy(config.levelName, selectedMission.assetName);
    // Native campaign launch selects this Keeper; both network peers must retain that identity.
    config.useFe_unkTy = selectedMission.keeperId;
    // CWorld applies these carrier records after authored availability. Empty counts
    // preserve mission unlock triggers while keeping buffers owned by the native lobby.
    config.creaturesCfgCount = config.roomsCfgCount = config.spellsCfgCount = 0;
    config.trapsCfgCount = config.doorsCfgcount = 0;
    return true;
}
