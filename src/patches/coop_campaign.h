#pragma once
#include <cstdint>
namespace dk2 { struct GameCfg; }

/** A menu-selected campaign route; the host's native selection lasts only for this session. */
namespace patch::coop_campaign {
/** Native campaign identity is distinct from the filename for branches and secret missions. */
struct Mission {
    int nativeId;
    int keeperId;
    wchar_t assetName[64];
};
bool active();
void enter();
void leave();
/** Clear the completed mission while retaining the co-op route for the next session. */
void returnToFrontend();
/** Retain a native selection (or the same selection resolved from the host's session). */
void selectMission(int nativeId, const wchar_t *assetName, int keeperId);
const Mission *mission();
/** Resolve the original executable's campaign filename table without consulting local progress. */
const wchar_t *nativeMissionName(int nativeId);
/** Advertise the selected identity in the native session's unused map metadata word. */
int16_t sessionTag();
/** Return zero for stock sessions or an unsupported co-op marker. */
int missionIdFromSessionTag(int16_t tag);
/** Apply only the selected fresh network carrier; false leaves the complete configuration unchanged. */
bool apply(dk2::GameCfg &config);
}
