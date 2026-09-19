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
