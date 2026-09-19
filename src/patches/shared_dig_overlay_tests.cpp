#include "shared_dig_overlay.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}
uint32_t tile(int x, int y) { return static_cast<uint32_t>(x | (y << 16)); }
}

/** Remote 5x5 marking and cancellation must reach graphics in order without altering the shared batch. */
int main() {
    dk2::GameActionCtx actions{};
    actions.actionArr_count = 4;
    actions.actionArr[0] = {tile(10,20), tile(14,24), 0, 40, 3};
    actions.actionArr[1] = {tile(11,21), tile(13,23), 0, 41, 3};
    actions.actionArr[2] = {tile(10,20), tile(14,24), 0, 40, 4};
    actions.actionArr[3] = {tile(10,20), tile(14,24), 0, 42, 3};
    const auto before = actions;
    std::vector<dk2::GameAction> drawn;
    // The graphics boundary receives the complete rectangle, including currently hidden tiles.
    const auto draw = [&](const dk2::GameAction &action) { drawn.push_back(action); };
    patch::shared_dig_overlay::refresh(true, 3, actions, draw);
    require(drawn.size() == 2, "Shared dig marking/cancellation did not reach the graphics bridge");
    require(std::memcmp(&drawn[0], &actions.actionArr[0], sizeof(dk2::GameAction)) == 0,
        "The full 5x5 designation was changed or truncated");
    require(std::memcmp(&drawn[1], &actions.actionArr[1], sizeof(dk2::GameAction)) == 0,
        "Cancellation did not retain its rectangle and ordering");
    require(std::memcmp(&actions, &before, sizeof(actions)) == 0, "Graphics refresh mutated synchronized actions");
    drawn.clear();
    patch::shared_dig_overlay::refresh(false, 3, actions, draw);
    require(drawn.empty(), "Ordinary multiplayer acquired co-op graphics replay");
    patch::shared_dig_overlay::refresh(true, 5, actions, draw);
    require(drawn.empty(), "Another Keeper's designations were displayed");
    actions.actionArr_count = 0;
    patch::shared_dig_overlay::refresh(true, 3, actions, draw);
    require(drawn.empty(), "Empty action batch produced a graphics update");
    return EXIT_SUCCESS;
}
