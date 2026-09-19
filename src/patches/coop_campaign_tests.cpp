#include "coop_campaign.h"
#include "dk2/settings/GameCfg.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <initializer_list>

namespace {
void require(bool valid, const char *why) {
    if (!valid) { std::fprintf(stderr, "%s\n", why); std::exit(EXIT_FAILURE); }
}
dk2::GameCfg carrier() {
    dk2::GameCfg cfg{};
    cfg.useFe_playMode = 3;
    cfg.useFe_unkTy = 7;
    std::wcscpy(cfg.levelName, L"Gonzalez");
    cfg.roomsCfgCount = 2; cfg.spellsCfgCount = 3;
    cfg.creaturesCfgCount = 4; cfg.trapsCfgCount = 5; cfg.doorsCfgcount = 6;
    cfg.maxCreatures = 42;
    return cfg;
}
void unchanged(dk2::GameCfg cfg) {
    const auto before = cfg;
    require(!patch::coop_campaign::apply(cfg), "unselected or unsupported launch was accepted");
    require(std::memcmp(&before, &cfg, sizeof(cfg)) == 0, "excluded launch configuration changed");
}
/** A native-selected asset replaces only the carrier's mission and availability overrides. */
void launches(int nativeId, const wchar_t *assetName) {
    using namespace patch::coop_campaign;
    selectMission(nativeId, assetName, 3);
    require(mission() && mission()->nativeId == nativeId, "native campaign identity was lost");
    require(std::wcscmp(mission()->assetName, assetName) == 0, "selected filename was changed");
    require(missionIdFromSessionTag(sessionTag()) == nativeId, "host mission cannot cross native session metadata");
    auto cfg = carrier();
    auto expected = cfg;
    std::wcscpy(expected.levelName, assetName);
    expected.useFe_unkTy = 3;
    expected.roomsCfgCount = expected.spellsCfgCount = expected.creaturesCfgCount = 0;
    expected.trapsCfgCount = expected.doorsCfgcount = 0;
    require(apply(cfg), "selected campaign did not launch");
    require(std::memcmp(&expected, &cfg, sizeof(cfg)) == 0,
        "co-op must preserve transport settings and exclude only carrier availability");
}
}

/** Both peers launch the host selection without consulting their own progress or retaining stale missions. */
int main() {
    using namespace patch::coop_campaign;
    require(!active() && !mission(), "co-op route must start inactive without a mission");
    unchanged(carrier());
    enter();
    unchanged(carrier());
    require(sessionTag() == 0, "no mission must not advertise a playable session");
    launches(1, L"Level1");
    launches(8, L"Level7");
    launches(24, L"Level20");
    launches(26, L"Secret1");
    // The native selector owns mission availability; this adapter must not add a Level1/2 gate.
    launches(18, L"Level15a");
    for (int mode : {0, 1, 2, 4}) { auto other = carrier(); other.useFe_playMode = mode; unchanged(other); }
    auto save = carrier(); save.hasSaveFile = 1; unchanged(save);
    auto fe = carrier(); fe.useFe3d = 1; unchanged(fe);
    fe = carrier(); fe.useFe2d_unk1 = 1; unchanged(fe);
    auto wrong = carrier(); std::wcscpy(wrong.levelName, L"PillowFight"); unchanged(wrong);
    auto lower = carrier(); std::wcscpy(lower.levelName, L"gonzalez");
    require(apply(lower), "native case-insensitive carrier name was rejected");
    const auto hostTag = sessionTag();
    leave();
    require(!active() && !mission() && sessionTag() == 0, "Back retained co-op mission state");
    unchanged(carrier());
    enter();
    unchanged(carrier());
    // A client resolves the host's identity using native assets, irrespective of local unlocks.
    selectMission(missionIdFromSessionTag(hostTag), L"Level15a", 3);
    auto client = carrier();
    require(apply(client) && std::wcscmp(client.levelName, L"Level15a") == 0,
        "client did not launch the advertised host mission");
    require(missionIdFromSessionTag(0) == 0, "stock session accepted as campaign");
    require(missionIdFromSessionTag(0x4401) == 0, "different protocol marker accepted");
    require(missionIdFromSessionTag(0x4300) == 0, "missing campaign identity accepted");
    returnToFrontend();
    require(active() && !mission() && sessionTag() == 0, "debriefing must retain co-op but clear the completed mission");
    unchanged(carrier());
    returnToFrontend();
    require(active() && !mission(), "frontend reload must preserve the empty co-op route");
    selectMission(16, L"Level16", 3);
    auto next = carrier();
    require(apply(next) && std::wcscmp(next.levelName, L"Level16") == 0,
        "creating the next co-op session must use the newly selected mission");
    leave();
    returnToFrontend();
    require(!active() && !mission(), "ordinary multiplayer and backing out must not acquire the co-op route");
    return 0;
}
