#include <dk2/InputCursor.h>
#include <dk2/CursorDrawer.h>
#include <dk2/inputs/InputSurf.h>
#include <dk2_functions.h>

/** Hold the native cursor semaphore until the matching post-blit event. */
void dk2::InputCursor::Event1_2Handler_updateCursor() {
    this->waitForSema(true);
    // Surface-release event 0/5 clears the drawer until window-shown event 0/7.
    // A redraw in that interval caused the focus-return crash at native 0x5DC850.
    // Keep the pre/post semaphore pair even when there is no cursor to draw.
    if (!this->cursorDrawer) return;
    const auto flag = static_cast<char>(reinterpret_cast<uintptr_t>(this->pInputSurf->v_isSurfaceFlag()));
    const auto draw = this->_doDrawCursor;
    auto *back = this->pInputSurf->v_getCurOffScreenSurf();
    auto *front = this->pInputSurf->v_getPrimarySurf();
    this->cursorDrawer->updateCursorAndForceDraw(this->pMousePos,
        reinterpret_cast<MyDdSurface *>(front), back, draw, flag);
}

/** Complete the native cursor draw and release the semaphore held across the blit. */
int dk2::InputCursor::Event1_3Handler() {
    if (this->cursorDrawer) {
        const auto flag = static_cast<char>(reinterpret_cast<uintptr_t>(this->pInputSurf->v_isSurfaceFlag()));
        const auto draw = this->_doDrawCursor;
        auto *back = this->pInputSurf->v_getCurOffScreenSurf();
        auto *front = this->pInputSurf->v_getPrimarySurf();
        this->cursorDrawer->drawCursorTo(front, back, draw, flag);
    }
    // Unavailable cursor frames still finish the native deadline/lock lifecycle.
    this->timestamp = getTimeMs() + this->timestampDelta;
    return this->setAndRelease();
}
