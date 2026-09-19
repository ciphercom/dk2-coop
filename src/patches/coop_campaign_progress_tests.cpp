#include "coop_campaign_progress.h"
#include <cstdio>
#include <cstdlib>

/** Persistence belongs only to the host of a fresh campaign network session. */
int main() {
    for (int campaign = 0; campaign != 2; ++campaign)
    for (int host = 0; host != 2; ++host)
    for (int mode = 0; mode != 5; ++mode)
    for (int fe3d = 0; fe3d != 2; ++fe3d)
    for (int fe2d = 0; fe2d != 2; ++fe2d)
    for (int saved = 0; saved != 2; ++saved) {
        const bool expected = campaign == 1 && host == 1 && mode == 3 && fe3d == 0 && fe2d == 0 && saved == 0;
        if (patch::coop_campaign_progress::eligible(campaign != 0, host != 0, mode, fe3d != 0, fe2d != 0, saved != 0) != expected) {
            std::fprintf(stderr, "Unexpected campaign persistence eligibility\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
