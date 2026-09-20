#pragma once
#include <cstdint>

namespace dk2 { struct MyGameSession; struct GameActionCtx; }

/** Hooks receive a session only while its game loop is live; no DK2 pointer crosses threads. */
namespace patch::diagnostic {
/** Reuse the default-off bridge option for bounded native diagnostics. */
bool enabled();
/** Optional game-thread provider keeps co-op ownership outside the reusable bridge. */
using HandOwnerProvider = uint8_t (*)(uint16_t);
void init(HandOwnerProvider handOwner = nullptr);
void cleanup();
void pump(dk2::MyGameSession *session);
void observeLocalQueue(dk2::MyGameSession &session);
bool captureWorldActions(const dk2::GameActionCtx &actions, dk2::GameActionCtx &copy);
void observeWorldReturn(const dk2::GameActionCtx &actions);
}
