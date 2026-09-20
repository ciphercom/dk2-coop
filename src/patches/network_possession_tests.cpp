#include "network_possession.h"
#include <cstdio>

namespace {
/** Behavioral failures remain fatal in every build configuration. */
void require(bool valid, const char *message) { if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); } }
}

/** Model the native acceptance boundary: rejected casts never emit a camera-entry command. */
int main() {
    using namespace patch::network_possession;
    require(buttonBlocked(true, 448, 2, 0, 6, 6, 2), "occupied Keeper must disable the possession button");
    require(!buttonBlocked(true, 0, 2, 0, 6, 6, 2), "exit or death must unlock possession");
    require(!buttonBlocked(false, 448, 2, 0, 6, 6, 2), "native sessions must retain their button behavior");
    require(!buttonBlocked(true, 448, 1, 0, 6, 6, 2), "other spells must remain available");
    require(buttonBlocked(true, 448, 1, 1, 6, 6, 7), "possession must be locked on later panel pages too");
    require(!buttonBlocked(true, 448, 1, 0, 6, 6, 7), "matching slot on another page must remain available");
    require(!buttonBlocked(true, 448, 0, 0, 6, 6, 0), "hidden possession must not lock an empty slot");
    require(buttonBlocked(true, 448, 2, 0, 1, 6, 2),
        "possession in the second column must lock even with one icon per column");
    require(buttonBlocked(true, 448, 4, 1, 2, 6, 6),
        "scrolling one column must retain the lock in later visible columns");
    require(!buttonBlocked(true, 448, 7, 0, 2, 6, 7),
        "slots outside the visible panel must not be treated as possession buttons");
    const auto host = originForSlot(0), guest = originForSlot(1);
    Presentation local;
    require(!local.enter(true, guest, host, 3, 3, 448), "remote accepted possession must leave this Controller top-down");
    require(!local.owns(3, 448), "remote acceptance must not grant local input ownership");
    local.allocatedShot(100, host);
    require(local.enter(true, local.consumeShot(100), host, 3, 3, 448) && local.owns(3, 448),
        "only the Controller whose accepted shot caused entry may enter possession");
    require(!local.owns(4, 448) && !local.owns(3, 449), "ownership must match both Keeper and creature");
    require(!local.observe(3, 448), "unchanged authoritative possession must retain the local camera");
    require(local.observe(3, 0) && !local.owns(3, 448), "authoritative exit or death must release the owning local camera");
    require(!local.observe(3, 0), "camera cleanup must happen only once");

    // A failed local request creates no shot; later remote acceptance of that same creature stays remote.
    local.allocatedShot(101, guest);
    require(!local.enter(true, local.consumeShot(101), host, 3, 3, 448),
        "failed local casts must not steal a later remote cast on the same creature");
    local.allocatedShot(102, host);
    local.allocatedShot(103, guest);
    require(!local.enter(true, local.consumeShot(103), host, 3, 3, 448),
        "concurrent same-target casts must follow the accepted shot's origin");
    local.consumeShot(102); // Native busy rejection emits no entry command.
    require(!local.owns(3, 448), "a rejected competing shot must not enter the camera");
    require(local.consumeShot(102) == 0, "consumed or rejected shots must leave no pending origin");
    local.allocatedShot(104, host);
    local.allocatedShot(104, 0);
    require(local.consumeShot(104) == 0, "shot tag reuse must discard stale Controller identity");
    require(!local.enter(true, 0, host, 3, 3, 448), "unattributed entry must not commandeer either local camera");
    require(local.enter(false, guest, host, 3, 3, 448) && !local.owns(3, 448),
        "disabled patch must preserve native entry without recording ownership");
    require(local.enter(true, guest, host, 4, 3, 448) && !local.owns(4, 448),
        "commands for another Keeper must retain the native Keeper filter");
    local.enter(true, host, host, 3, 3, 448);
    require(local.observe(3, 449), "unexpected authoritative replacement must release the old local view");
    local.enter(true, host, host, 3, 3, 448);
    local.observe(3, 0);
    // Native path completion consumes its deferred target before afterWorldTick can intervene.
    unsigned currentMode = 18, deferredMode = 2;
    uint16_t deferredCreature = 448;
    if (local.releaseCamera(currentMode, deferredMode, 3)) {
        // Native updateCameraMode defers this release instead of interrupting the authored path.
        deferredMode = 3; deferredCreature = 0;
    }
    require(currentMode == 18 && deferredMode == 3 && deferredCreature == 0,
        "destroyed creature must be removed from deferred camera entry before native path completion");
    require(!local.releaseCamera(2), "native deferred cleanup must be requested only once");
    local.enter(true, host, host, 3, 3, 448);
    local.observe(3, 0);
    require(local.releaseCamera(18, 0, 2), "saved possession view must be released while the authored path remains active");
    local.enter(true, host, host, 3, 3, 448);
    local.observe(3, 0);
    require(!local.releaseCamera(18, 7, 2), "explicitly queued unrelated view must override the saved possession mode");
    local.enter(true, host, host, 3, 3, 448);
    local.observe(3, 0);
    require(!local.releaseCamera(7) && !local.releaseCamera(2), "unrelated presentation must not be replaced by a stale possession exit");
    local.enter(true, host, host, 3, 3, 448);
    local.observe(3, 0);
    local.enter(true, host, host, 3, 3, 449);
    require(!local.releaseCamera(2) && local.owns(3, 449), "new accepted local possession must supersede deferred old cleanup");
    local.allocatedShot(105, host);
    local.clearPendingShots();
    require(local.owns(3, 449) && local.consumeShot(105) == 0, "reload must clear pending shot identity while retaining accepted local ownership");
    local.allocatedShot(105, host);
    local.enter(true, host, host, 3, 3, 448);
    local.reset();
    require(!local.owns(3, 448) && local.consumeShot(105) == 0, "session reset must clear ownership and pending shots");
    require(!validOrigin(0) && !validOrigin(0x504F5300) && !validOrigin(0x504F5309) && validOrigin(host),
        "only explicit bounded Controller markers may claim a shot");
    return 0;
}
