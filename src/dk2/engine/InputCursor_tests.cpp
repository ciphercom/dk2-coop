#include <dk2/InputCursor.h>
#include <dk2/CursorDrawer.h>
#include <dk2/inputs/InputSurf.h>
#include <dk2_functions.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
/** Keep contract failures fatal in every test configuration. */
void require(bool valid, const char *message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); std::exit(EXIT_FAILURE); }
}

std::string calls;
bool locked = false;
char drawEnabled = 0;
char surfaceFlag = 0;
std::array<unsigned char, sizeof(dk2::InputCursor)> cursorStorage{};
std::array<unsigned char, sizeof(dk2::CursorDrawer)> drawerStorage{};
dk2::InputCursor &cursor = *reinterpret_cast<dk2::InputCursor *>(cursorStorage.data());
auto *drawer = reinterpret_cast<dk2::CursorDrawer *>(drawerStorage.data());
dk2::Pos2i mouse{37, 51};
dk2::MyDdSurfaceEx *primary = reinterpret_cast<dk2::MyDdSurfaceEx *>(0x1000);
dk2::MyDdSurfaceEx *offscreen = reinterpret_cast<dk2::MyDdSurfaceEx *>(0x2000);

/** A native surface-provider ABI double: no DirectDraw or game process is required. */
struct Surfaces {
    virtual void *destroy(char) { require(false, "unexpected surface destruction"); return nullptr; }
    virtual dk2::MyDdSurfaceEx *getPrimary() { calls += "primary "; return primary; }
    virtual dk2::MyDdSurfaceEx *getOffscreen() { calls += "offscreen "; return offscreen; }
    virtual dk2::AABB *getBounds(dk2::AABB *) { require(false, "unexpected bounds lookup"); return nullptr; }
    virtual void *getFlag() { calls += "flag "; return reinterpret_cast<void *>(surfaceFlag); }
} surfaces;

/** Model the state published by native surface release/show events (0/5 and 0/7). */
void frame(bool available) {
    cursor.cursorDrawer = available ? drawer : nullptr;
    calls.clear();
    cursor.Event1_2Handler_updateCursor();
    require(locked, "pre-render did not retain the cursor semaphore across the screen blit");
    require(calls == (available ? "lock flag offscreen primary update " : "lock "),
        "pre-render touched unavailable surfaces or changed native call order");
    require(cursor.Event1_3Handler() == 73, "post-render changed the native release result");
    require(!locked, "post-render left the cursor semaphore locked");
    require(cursor.timestamp == 1250, "post-render did not preserve the redraw deadline");
    require(calls == (available ? "lock flag offscreen primary update flag offscreen primary draw time unlock " : "lock time unlock "),
        "post-render touched unavailable surfaces or changed native call order");
}
}

// Native synchronization, clock, and drawing are the boundaries around the real handlers.
char dk2::InputCursor::waitForSema(bool wait) {
    require(wait && !locked, "pre-render must acquire the semaphore exactly once");
    locked = true; calls += "lock "; return 1;
}
int dk2::InputCursor::setAndRelease() {
    require(locked, "post-render released an unowned semaphore");
    locked = false; calls += "unlock "; return 73;
}
uint32_t dk2::getTimeMs() { calls += "time "; return 1200; }
void dk2::CursorDrawer::updateCursorAndForceDraw(Pos2i *pos, MyDdSurface *front,
        MyDdSurfaceEx *back, char enabled, char flag) {
    require(this == drawer, "pre-render dereferenced the released cursor drawer");
    require(locked && pos == &mouse && front == reinterpret_cast<MyDdSurface *>(primary)
        && back == offscreen && enabled == drawEnabled && flag == surfaceFlag,
        "pre-render changed native cursor arguments or lock ownership");
    calls += "update ";
}
int dk2::CursorDrawer::drawCursorTo(MyDdSurfaceEx *front, MyDdSurfaceEx *back, char enabled, char flag) {
    require(this == drawer, "post-render dereferenced the released cursor drawer");
    require(locked && front == primary && back == offscreen && enabled == drawEnabled && flag == surfaceFlag,
        "post-render changed native cursor arguments or lock ownership");
    calls += "draw "; return 0;
}

/** Released frames must stay balanced; normal and hidden cursors resume after surfaces return. */
int main() {
    cursor.pInputSurf = reinterpret_cast<dk2::InputSurf *>(&surfaces);
    cursor.pMousePos = &mouse;
    cursor.timestampDelta = 50;
    for (char enabled : {0, 1}) {
        for (char flag : {0, 1}) {
            drawEnabled = cursor._doDrawCursor = enabled;
            surfaceFlag = flag;
            frame(true);
            frame(false);
            frame(false);
            frame(true);
        }
    }
    return 0;
}
