#include "coop_campaign_init.h"
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool valid, const char *why) {
    if (!valid) { std::fprintf(stderr, "%s\n", why); std::exit(EXIT_FAILURE); }
}
}

/** Only the selected fresh campaign receives authored rules, regardless of Controller role. */
int main() {
    using patch::coop_campaign_init::effectiveMode;
    using patch::coop_campaign_init::terminalMode;
    const int transportMode = 3;
    require(effectiveMode(true, transportMode, false, false, false) == 1,
        "selected network campaign still uses carrier world-initialization rules");
    // Native timer expiry defeats Keepers only in modes 2 and 3; campaign mode lets scripts decide.
    const auto timerDefeatsKeepers = [](int mode) { return mode == 2 || mode == 3; };
    require(!timerDefeatsKeepers(effectiveMode(true, transportMode, false, false, false)),
        "campaign countdown expiry still automatically defeats the shared Keeper");
    require(timerDefeatsKeepers(effectiveMode(false, transportMode, false, false, false)),
        "ordinary multiplayer timeout no longer ends the match");
    require(terminalMode(true, transportMode, false, false, false) == 0,
        "campaign status change still propagates multiplayer victory or persists a new score");
    for (int selected = 0; selected != 2; ++selected)
        for (int mode = 0; mode != 5; ++mode)
            for (int exclusions = 0; exclusions != 8; ++exclusions) {
                if (selected && mode == 3 && exclusions == 0) continue;
                require(effectiveMode(selected != 0, mode, (exclusions & 1) != 0,
                    (exclusions & 2) != 0, (exclusions & 4) != 0) == mode,
                    "ordinary match, frontend or saved game initialization changed");
                require(terminalMode(selected != 0, mode, (exclusions & 1) != 0,
                    (exclusions & 2) != 0, (exclusions & 4) != 0) == mode,
                    "ordinary match, frontend or saved game outcome handling changed");
            }
    return 0;
}
