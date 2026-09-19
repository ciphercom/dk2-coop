#pragma once
namespace dk2 { struct MyGameSession; }

namespace patch::scripted_camera {
/** Read-only shared wait queries for native cinematic states outside the type72 trigger. */
bool enabledForSession();
bool pathPending();
bool movementPending();
/** Clear presentation deadlines before a session can initialize its first path. */
void resetSession();
/** Complete due paths immediately before the original shared world tick. */
void beforeWorldTick(dk2::MyGameSession &session);
/** Select only the original camera-complete condition in co-op campaign sessions. */
bool correctsCondition(int type);
/** Keep original endTime/mode checks while replacing local movement/rotation deadlines. */
int conditionResult(int type, int original, int endTime, unsigned mode);
}
