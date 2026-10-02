#pragma once

namespace patch::possession_navigation {
/** Keep the local capability available throughout prediction, then restore the shared navigation context. */
template<typename Update> auto localMovement(bool isolate, int &capability, Update update) {
    const int sharedCapability = capability;
    const auto result = update();
    if (isolate) capability = sharedCapability;
    return result;
}
}
