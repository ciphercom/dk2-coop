#include "coop_campaign.h"

const wchar_t *patch::coop_campaign::nativeMissionName(int nativeId) {
    if (nativeId < 1 || nativeId > 30 || nativeId == 25) return nullptr;
    // DKII 1.70's campaign selector at 53A031 and result flow at 53C3FF
    // index this same native table. Entry 25 is the non-mission "null" slot.
    const auto *names = reinterpret_cast<const wchar_t *const *>(0x006B83EC);
    return names[nativeId];
}