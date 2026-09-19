#include "network_gem_ending_ui.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {
/** Assertions remain fatal even when the build disables the standard assert macro. */
void require(bool valid, const char *message) {
    if (!valid) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}
}

/** Exit requires the shared physical ending and this Controller's native gem animation. */
int main() {
    using namespace patch::network_gem_ending_ui;
    const bool completionCases[][4] = {
        // Shared physical completion, UI started, UI complete, expected pending.
        {false, false, false, true},
        {false, false, true, true},
        {false, true, false, true},
        {false, true, true, true},
        {true, false, false, true},
        {true, false, true, true},
        {true, true, false, true},
        {true, true, true, false},
    };
    for (const auto &state : completionCases) {
        require(!pending(false, state[0], state[1], state[2]),
            "inactive Keeper acquired an ending gate");
        require(pending(true, state[0], state[1], state[2]) == state[3],
            "exit did not require shared completion and both native UI latches");
    }

    require(pending(true, false, false, false, true) &&
        !pending(true, true, false, false, true),
        "alternative ending must wait for shared completion without requiring gem UI latches");

    require(hidePrompt(true, 0x0040422E, true), "pending victory prompt remained visible");
    require(!hidePrompt(false, 0x0040422E, true), "completed or inactive victory prompt was hidden");
    require(!hidePrompt(true, 0x0040422E, false), "unrelated text buffer was hidden");
    require(!hidePrompt(true, 0x00404178, true), "other native overlay caller was hidden");
    require(!hidePrompt(true, 0, true), "non-victory caption caller was hidden");

    for (bool waiting : {false, true}) {
        for (int key : {0x01, 0x39, 0x1C, 0x20}) {
            for (int pressed : {0, 1}) {
                for (int nativeResult : {0, 17}) {
                    int calls = 0;
                    const auto result = keyboardAction(waiting, key, pressed, [&] {
                        ++calls;
                        return nativeResult;
                    });
                    const bool blocked = waiting && pressed && (key == 0x01 || key == 0x39);
                    require(calls == (blocked ? 0 : 1),
                        "keyboard event was forwarded incorrectly or more than once");
                    require(result == (blocked ? 1 : nativeResult),
                        "keyboard event changed the native return result or was not consumed");
                }
            }
        }
    }
    return 0;
}
