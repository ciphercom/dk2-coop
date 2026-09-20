#include "test_availability.h"

#include <dk2/CWorld.h>
#include <dk2/entities/CPlayer.h>
#include <dk2/settings/GameCfg.h>
#include <cstdlib>
#include <limits>

bool patch::test_availability::apply(bool enabled, const dk2::GameCfg &config, dk2::CWorld &world) {
    if (!enabled || config.useFe_playMode != 3 || config.useFe3d ||
        config.useFe2d_unk1 || config.hasSaveFile) return false;

    // Walk the same authoritative faction list on both peers, never the local
    // Controller's Keeper. Native PlayerList allocates at most seven factions.
    uint16_t tag = world.playerList.allocatedList;
    unsigned visited = 0;
    while (tag) {
        if (tag >= 4096 || ++visited > 7) std::abort();
        auto *keeper = static_cast<dk2::CPlayer *>(world.getCTag(tag));
        if (!keeper || keeper->f0_tagId != tag) std::abort();
        // Match CWorld::sub_50C800: neutral/hero player numbers precede Keeper3.
        if (keeper->playerNumber >= 3) {
            // Native setters accept byte IDs and ignore types absent from the
            // loaded level. The spell UI hides state 1 and disables states 2/4;
            // state 3 follows its ordinary usable-content path.
            // Only availability changes: upgrade/research progress, money, mana,
            // creature pools and the authored files retain their native values.
            for (unsigned id = 1; id <= (std::numeric_limits<uint8_t>::max)(); ++id) {
                const auto type = static_cast<uint8_t>(id);
                keeper->sub_4BAAD0(type, 3); // Rooms.
                keeper->sub_4BAC80(type, 3); // Doors.
                keeper->sub_4BADE0(type, 3); // Traps.
                keeper->sub_4BAF40(type, 3); // Keeper spells.
            }
        }
        tag = keeper->nextIdx;
    }
    return true;
}
