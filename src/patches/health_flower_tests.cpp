#include "health_flower.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

/** Exercise both Controllers, native sessions, and transitions without changing shared possession. */
int main() {
    using patch::health_flower::possessionDisplay;
    for (unsigned mode = 0; mode <= 18; ++mode) {
        const bool keeperView = mode == 3 || mode == 4 || mode == 5 || mode == 9 ||
            mode == 13 || mode == 14 || mode == 15;
        for (const bool occupied : {false, true}) {
            if (possessionDisplay(false, occupied, mode) != occupied ||
                possessionDisplay(true, occupied, mode) != (occupied && !keeperView)) {
                std::fprintf(stderr, "flower display regression: camera=%u possession=%d\n", mode, occupied);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
