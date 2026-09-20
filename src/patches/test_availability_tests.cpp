#include "test_availability.h"

#include <dk2/CWorld.h>
#include <dk2/entities/CPlayer.h>
#include <dk2/settings/GameCfg.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
/** Keep contract failures active in production-like test builds. */
void require(bool valid, const char *message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}

// The binary owns these objects in production. Tests supply only their ABI storage;
// no game constructors, DLL imports, local Controller, or graphics are involved.
std::array<unsigned char, sizeof(dk2::CWorld)> worldStorage{};
std::array<std::array<unsigned char, sizeof(dk2::CPlayer)>, 4> playerStorage{};
dk2::CWorld &world = *reinterpret_cast<dk2::CWorld *>(worldStorage.data());
std::array<dk2::CPlayer *, 4> players{};
struct Availability { int unavailable = 1; int researchable = 2; int highId = 4; int researchProgress = 71; };
std::array<std::array<Availability, 4>, 4> availability{};

/** Native setter stand-in: unknown IDs are no-ops; only the availability value changes. */
int set(dk2::CPlayer *player, unsigned category, uint8_t id, int state) {
    require(id != 0, "invalid content ID zero was passed to the native setter");
    for (unsigned index = 0; index < players.size(); ++index) {
        if (players[index] != player) continue;
        auto &entry = availability[index][category];
        if (id == 2) entry.unavailable = state;
        if (id == 9) entry.researchable = state;
        if (id == 255) entry.highId = state;
        return 0;
    }
    require(false, "a non-player reached availability mutation");
    return 0;
}

/** Two Keepers with different tags prove that eligibility never depends on the local Controller. */
dk2::GameCfg reset() {
    worldStorage.fill(0);
    for (unsigned index = 0; index < players.size(); ++index) {
        playerStorage[index].fill(0);
        players[index] = reinterpret_cast<dk2::CPlayer *>(playerStorage[index].data());
        players[index]->f0_tagId = static_cast<uint16_t>(11 + index);
        players[index]->playerNumber = static_cast<uint8_t>(index + 1);
        players[index]->nextIdx = index + 1 == players.size() ? 0 : static_cast<uint16_t>(12 + index);
        players[index]->money = 1234;
        players[index]->mana = 567;
        for (auto &entry : availability[index]) entry = Availability{};
    }
    world.playerList.allocatedList = 11;
    world.playerList.numberOfPlayersCreated = static_cast<int>(players.size());
    dk2::GameCfg config{};
    config.useFe_playMode = 3;
    config.creaturesCfgCount = 5;
    config.maxCreatures = 31;
    return config;
}

/** A disabled/ineligible flag must not affect any faction, native storage, or lobby configuration. */
void unchanged(bool enabled, const dk2::GameCfg &config) {
    const auto prior = availability;
    const auto priorWorld = worldStorage;
    const auto priorPlayers = playerStorage;
    require(!patch::test_availability::apply(enabled, config, world), "ineligible launch was overridden");
    require(std::memcmp(prior.data(), availability.data(), sizeof(availability)) == 0,
        "ineligible launch changed content availability");
    require(priorWorld == worldStorage && priorPlayers == playerStorage, "ineligible launch changed world state");
}
}

// These five functions are the external native-game boundary, not mocks of patch logic.
dk2::CTag *dk2::CWorld::getCTag(uint16_t id) {
    for (auto *player : players) if (player->f0_tagId == id) return player;
    return nullptr;
}
int dk2::CPlayer::sub_4BAAD0(uint8_t id, int state) { return set(this, 0, id, state); }
int dk2::CPlayer::sub_4BAC80(uint8_t id, int state) { return set(this, 1, id, state); }
int dk2::CPlayer::sub_4BADE0(uint8_t id, int state) { return set(this, 2, id, state); }
int dk2::CPlayer::sub_4BAF40(uint8_t id, int state) { return set(this, 3, id, state); }

/** Availability expands for every Keeper while research progress, resources, and mission config survive. */
int main() {
    auto config = reset();
    unchanged(false, config);
    for (int mode = 0; mode <= 5; ++mode) {
        if (mode == 3) continue;
        config.useFe_playMode = mode;
        unchanged(true, config);
    }
    config = reset(); config.useFe3d = 1; unchanged(true, config);
    config = reset(); config.useFe2d_unk1 = 1; unchanged(true, config);
    config = reset(); config.hasSaveFile = 1; unchanged(true, config);

    config = reset();
    const auto priorConfig = config;
    const auto priorWorld = worldStorage;
    const auto priorPlayers = playerStorage;
    require(patch::test_availability::apply(true, config, world), "test flag did not unlock a fresh network match");
    for (unsigned index = 0; index < players.size(); ++index) {
        for (const auto &entry : availability[index]) {
            const bool keeper = index >= 2;
            require(entry.unavailable == (keeper ? 3 : 1) && entry.researchable == (keeper ? 3 : 2) &&
                    entry.highId == (keeper ? 3 : 4),
                "every Keeper must receive all loaded spells/buildings; neutral and hero factions must retain authored states");
            require(entry.researchProgress == 71, "unlock changed research progress");
        }
    }
    require(priorWorld == worldStorage && priorPlayers == playerStorage,
        "unlock altered non-availability Keeper/world fields");
    require(std::memcmp(&priorConfig, &config, sizeof(config)) == 0, "unlock altered campaign/lobby or creature-pool configuration");
    const auto once = availability;
    require(patch::test_availability::apply(true, config, world), "repeated eligible application failed");
    require(std::memcmp(once.data(), availability.data(), sizeof(availability)) == 0,
        "repeated application changed already available content");
    return 0;
}
