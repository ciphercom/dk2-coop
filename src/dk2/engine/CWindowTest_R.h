//
// Created by DiaLight on 1/10/2026.
//

#ifndef FLAME_CWINDOWTEST_H
#define FLAME_CWINDOWTEST_H

#include "dk2/engine/CWindowTest.h"

namespace dk2 {

    struct CWindowTest {
        HWND hWnd;
        MyDdSurfaceEx offScreenSurf;
        MyDdSurfaceEx *pCurOffScreenSurf;
        int created;

        /*  0*/ virtual void v_DESTRUCTOR_CWindowTest_void(char) {}

        dk2::CWindowTest *constructor();
        dk2::CWindowTest *scalar_destructor(char);
        int *create(int *, AABB *);
        int *recreateBullfrog(int *);
        AABB *getClientRect(AABB *);
        void reallocBackSurfaceToWindowSize();
        int *fillWithColor(int *, tagRECT *, Bgraf *);
        void copyToWindowSurf();
        int *probably_do_show_window_ev0_7(int *, AABB *);
        MyDdSurfaceEx *getCurOffScreenSurf();
        void recreate();
        int isNeedBlt();

    };
    static_assert(sizeof(CWindowTest) == sizeof(replaced_CWindowTest));

    void CWindowTest_destroy(dk2::CWindowTest *self);

}

#endif // FLAME_CWINDOWTEST_H
