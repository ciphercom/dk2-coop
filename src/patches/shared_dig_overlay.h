#pragma once
#include <dk2/GameActionCtx.h>
#include <cstdlib>

namespace patch::shared_dig_overlay {
/** Replay only this Controller's synchronized dig presentation; never requeue a world action. */
template<class Draw>
void refresh(bool sharedCampaign, uint16_t keeperTag, const dk2::GameActionCtx &actions, Draw draw) {
    if (!sharedCampaign) return;
    if (actions.actionArr_count > 16) std::abort();
    for (unsigned i = 0; i < actions.actionArr_count; ++i) {
        const auto &action = actions.actionArr[i];
        if (action.playerTagId == keeperTag && (action.actionKind == 40 || action.actionKind == 41))
            draw(action);
    }
}
}
