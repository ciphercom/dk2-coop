#include "game_bridge.h"
#include "pipe.h"

#include "dk2_globals.h"
#include "dk2/entities/CPlayer.h"
#include "tools/flame_config.h"
#include <cstdlib>
#include <memory>

namespace patch::diagnostic {
namespace {
flame_config::define_flame_option<bool> option(
    "flame:diagnostic-bridge", flame_config::OG_Config,
    "Enable the local PID-scoped Diagnostic bridge (traces start off)", false);
std::unique_ptr<Pipe> pipe;
Core core;
HandOwnerProvider handOwner = nullptr;

/** Assertions remain fatal even if this file is built with NDEBUG. */
void invariant(bool valid) { if (!valid) std::abort(); }

/** Snapshot only initialized game-loop state, and distinguish interface identity from world identity. */
std::string snapshot(dk2::MyGameSession *session, Snapshot name) {
    const std::string identity = "\"pid\":" + std::to_string(GetCurrentProcessId());
    if (!session || !session->pWorld || dk2::MyResources_instance.gameCfg.useFe3d ||
        dk2::MyResources_instance.gameCfg.useFe2d_unk1)
        return "{" + identity + ",\"available\":false,\"world_active\":false}";
    auto *world = session->pWorld;
    auto *controller = session->pPlayer;
    const auto keeperTag = world->v_getMEPlayerTagId();
    const bool network = dk2::MyResources_instance.gameCfg.useFe_playMode == 3;
    if (name == Snapshot::session) {
        return "{" + identity + ",\"available\":true,\"world_active\":true,\"play_mode\":" +
            std::to_string(dk2::MyResources_instance.gameCfg.useFe_playMode) +
            ",\"network\":" + (network ? "true" : "false") + ",\"session_tick\":" + std::to_string(session->gameTick) +
            ",\"world_tick\":" + std::to_string(world->getGameTick()) + ",\"world_keeper_tag\":" + std::to_string(keeperTag) +
            ",\"controller_keeper_tag\":" + (controller ? std::to_string(controller->playerTagId) : "null") +
            ",\"network_slot\":" + (network ? std::to_string(dk2::WeaNetR_instance.playersSlot) : "null") +
            ",\"in_menu\":" + (session->inMenu ? "true" : "false") +
            ",\"out_of_sync\":" + (session->isOutOfSync ? "true" : "false") +
            ",\"checksum_enabled\":" + (dk2::MyResources_instance.useChecksum ? "true" : "false") + "}";
    }
    if (!controller) return "{" + identity + ",\"available\":false}";
    // The original tag getter indexes a 4096-entry table without checking its argument.
    invariant(controller->playerTagId < 4096);
    auto *keeper = static_cast<dk2::CPlayer *>(world->v_getCTag_508C40(controller->playerTagId));
    if (!keeper) return "{" + identity + ",\"available\":false}";
    invariant(keeper->getVtbl() == dk2::CPlayer::vftable);
    invariant(keeper->f0_tagId == controller->playerTagId);
    const std::string prefix = "{" + identity + ",\"available\":true,\"keeper_tag\":" + std::to_string(keeper->f0_tagId);
    if (name == Snapshot::player)
        return prefix + ",\"money\":" + std::to_string(keeper->money) + ",\"mana\":" + std::to_string(keeper->mana) +
            ",\"player_flags\":" + std::to_string(keeper->playerFlags) + "}";
    if (name == Snapshot::possession) {
        const auto *camera = session->pBridge ? session->pBridge->v_fD0_getCamera() : nullptr;
        return prefix + ",\"creature_possessed\":" + std::to_string(keeper->creaturePossessed) +
            ",\"creature_held_in_possession\":" + std::to_string(keeper->creatureHeldInPossession) +
            ",\"camera_mode\":" + (camera ? std::to_string(camera->_mode) : "null") + "}";
    }
    invariant(name == Snapshot::hand);
    invariant(keeper->thingsInHand_count <= 64 && controller->thingsInHand_count <= 65);
    std::string result = prefix + ",\"keeper_things\":[";
    for (unsigned i = 0; i < keeper->thingsInHand_count; ++i) {
        if (i) result += ',';
        result += std::to_string(keeper->thingsInHand[i]);
    }
    result += "],\"keeper_hand_owners\":";
    if (handOwner) {
        result += '[';
        for (unsigned i = 0; i < keeper->thingsInHand_count; ++i) {
            if (i) result += ',';
            result += std::to_string(handOwner(keeper->thingsInHand[i]));
        }
        result += ']';
    } else {
        // Absence of a co-op provider is unknown ownership, not native origin zero.
        result += "null";
    }
    result += ",\"controller_things\":[";
    for (unsigned i = 0; i < controller->thingsInHand_count; ++i) {
        if (i) result += ',';
        const auto &entry = controller->thingsInHand[i];
        std::optional<HandDropTarget> target;
        if (entry.hasUnderHand)
            target = HandDropTarget{entry.underHand.x_if12, entry.underHand.y_if12, entry.underHand.type, entry.underHand.tagId};
        result += handEntryJson(entry.tagId, entry.dropped, target);
    }
    return result + "]}";
}

void record(Boundary boundary, int tick, const dk2::GameAction &action) {
    core.record({boundary, tick, action.actionKind, action.playerTagId, action.data1, action.data2, action.data3});
}
}

bool enabled() { return option.get(); }

void init(HandOwnerProvider provider) {
    handOwner = provider;
    if (option.get()) pipe = std::make_unique<Pipe>();
}

void cleanup() { pipe.reset(); handOwner = nullptr; }

void pump(dk2::MyGameSession *session) {
    if (!pipe) return;
    pipe->pump(core, [session](Snapshot name) { return snapshot(session, name); }, [](bool enabled) {
        // This override is deliberately temporary: diagnostic sessions must not rewrite saved logging preferences.
        flame_config::set_tmp_option("flame:logging:debug", flame_config::flame_value(enabled));
    });
}

/** Observe the queue before the original handler; repeats are possible if dispatch does not drain it. */
void observeLocalQueue(dk2::MyGameSession &session) {
    if (!pipe || !core.tracing() || dk2::MyResources_instance.gameCfg.useFe3d) return;
    const auto &queue = session.clickList;
    invariant(queue.loopArr32_count <= 32 && queue.loopArr32_idx >= 0 && queue.loopArr32_idx < 32);
    // DKII 1.70 GameActionArray::pop at 0x525F80 reads index +0x244 then wraps at 32.
    for (unsigned i = 0; i < queue.loopArr32_count; ++i)
        record(Boundary::local_queue_before_handle, session.gameTick, queue.loopArr32[(queue.loopArr32_idx + i) % 32]);
}

/** Preserve input arguments because original action handlers receive mutable pointers into this batch. */
bool captureWorldActions(const dk2::GameActionCtx &actions, dk2::GameActionCtx &copy) {
    if (!pipe || !core.tracing() || dk2::MyResources_instance.gameCfg.useFe3d) return false;
    invariant(actions.actionArr_count <= 16);
    copy = actions;
    return true;
}

/** A returned world tick confirms the batch crossed dispatch, not that each action succeeded. */
void observeWorldReturn(const dk2::GameActionCtx &actions) {
    if (!pipe || !core.tracing() || dk2::MyResources_instance.gameCfg.useFe3d) return;
    invariant(actions.actionArr_count <= 16);
    for (unsigned i = 0; i < actions.actionArr_count; ++i)
        record(Boundary::world_tick_return, actions.gameTick, actions.actionArr[i]);
}
}
