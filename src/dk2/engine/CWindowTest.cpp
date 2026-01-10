//
// Created by DiaLight on 9/19/2025.
//
#include "dk2/engine/CWindowTest.h"
#include "WinEventHandlers.h"
#include "dk2/MyDisplayProperties.h"
#include "dk2/MyStr.h"
#include "dk2/dk2_memory.h"
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "game_engine.h"
#include "gog_patch.h"
#include "patches/big_resolution_fix/big_resolution_fix.h"
#include "patches/logging.h"
#include "patches/micro_patches.h"
#include "patches/remember_window_location_and_size.h"
#include "patches/replace_mouse_dinput_to_user32.h"
#include "patches/show_wireframe.h"
#include "patches/use_wheel_to_zoom.h"
#if __has_include(<dk2_research.h>)
#include <dk2_research.h>
#endif


namespace dk2 {

    CWindowTest *dk2::CWindowTest::constructor() {
        this->offScreenSurf.dd_surf.fld10_00 = 0;
        this->offScreenSurf.dd_surf.fld11_ff = 0;
        this->offScreenSurf.dd_surf.fld12_00 = 0;
        this->offScreenSurf.dd_surf.fld13_ff = 0;
        this->offScreenSurf.dd_surf.dwColorSpaceValue_00 = 0;
        this->offScreenSurf.dd_surf.f20 = 0;
        this->offScreenSurf.dd_surf.dd_surface = NULL;
        this->offScreenSurf.surf.constructor_empty();
        this->pCurOffScreenSurf = NULL;
        this->created = 0;
        return this;
    }

    void CWindowTest_destroy(CWindowTest *self) {
        if (!self->created) return;
        int status;
        if (self->pCurOffScreenSurf) {
            MyDdSurface_release(&status, &self->pCurOffScreenSurf->dd_surf);
            self->pCurOffScreenSurf = NULL;
        }
        self->created = 0;
        BullfrogWindow_destroy();
        DestroyWindow(self->hWnd);
        BullfrogWindow_create(&status, MyGame_instance.getSelectedGuid(), 1, NULL, NULL);
    }

    dk2::CWindowTest *dk2::CWindowTest::scalar_destructor(char a2) {
        CWindowTest_destroy(this);
        if ( (a2 & 1) != 0 )
            dk2::operator_delete(this);
        return this;
    }

    void getDisplayProperties(MyDisplayProperties *a1) {
        HDC DCA = CreateDCA("DISPLAY", NULL, NULL, NULL);
        if (!DCA) return;
        a1->bitsPerPixel = GetDeviceCaps(DCA, BITSPIXEL);
        a1->horPixels = GetDeviceCaps(DCA, HORZRES);
        a1->verPixels = GetDeviceCaps(DCA, VERTRES);
        DeleteDC(DCA);
    }

    LRESULT CALLBACK CWindowTest_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {  // windowed proc
#if __has_include(<dk2_research.h>)
        research::windowProc(hWnd, Msg, wParam, lParam);
#endif

        // patch::BEFORE_WINDOW_PROC
        patch::remember_window_location_and_size::window_proc(hWnd, Msg, wParam, lParam);
        patch::replace_mouse_dinput_to_user32::emulate_dinput_from_user32(hWnd, Msg, wParam, lParam);
        patch::use_wheel_to_zoom::window_proc(hWnd, Msg, wParam, lParam);
        patch::fix_keyboard_state_on_alt_tab::window_proc(hWnd, Msg, wParam, lParam);
        patch::show_wireframe::window_proc(hWnd, Msg, wParam, lParam);
        patch::bring_to_foreground::window_proc(hWnd, Msg, wParam, lParam);
        if (!patch::fix_close_window::window_proc(hWnd, Msg, wParam, lParam)) return 0;
        switch(Msg) {
        case WM_ACTIVATE:
            g_isNeedBlt_fullscr = wParam != 0;
            break;
        case WM_SYSCOMMAND: {
            switch ( wParam ) {
            case 0xF090u:
            case 0xF093u:
            case 0xF100u:
            case 0xF160u:
            case 0xF163u:
                return 0;
            default:
                break;
            }
            break;
        }
        case WM_MOUSEMOVE: {
            if(!patch::replace_mouse_dinput_to_user32::enabled) {
                Pos2i pos;
                pos.x = LOWORD(lParam);
                pos.y = HIWORD(lParam);
                MyInputManagerCb_static_setMousePos(&pos);
            }
            break;
        }
        }
        if(patch::hide_mouse_cursor_in_window::window_proc(hWnd, Msg, wParam, lParam)) return TRUE;

        if (auto CustomDefWindowProcA = getCustomDefWindowProcA())
            return CustomDefWindowProcA(hWnd, Msg, wParam, lParam);
        return DefWindowProcA(hWnd, Msg, wParam, lParam);
    }

}

int *dk2::CWindowTest::create(int *pstatus, AABB *rect) {
    WNDCLASSEXA wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.cbSize = sizeof(wndClass);
    if (!GetClassInfoExA(getHInstance(), "LibWindow", &wndClass)) {
        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.cbSize = sizeof(wndClass);

        wndClass.style = 3;
        wndClass.lpfnWndProc = CWindowTest_proc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = getHInstance();
        wndClass.hIcon = LoadIconA(getHInstance(), getLibIconName());
        wndClass.hCursor = LoadCursorA(NULL, (LPCSTR) 0x7F00);
        wndClass.hbrBackground = NULL;
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = "LibWindow";
        wndClass.hIconSm = LoadIconA(getHInstance(), getLibIconName());
        RegisterClassExA(&wndClass);
    }
    int minY = rect->minY;
    int minX = rect->minX;
    int maxX = rect->maxX;
    int maxY = rect->maxY;
    LONG width = maxX - minX;
    LONG height = maxY - minY;

    RECT windowRect {0, 0, width, height};
    AdjustWindowRect(&windowRect, 0xCF0000u, 0);

    MyDisplayProperties props;
    getDisplayProperties(&props);

    char buf[256];
    sprintf(buf, "(%dx%dx%d)", width, height, props.bitsPerPixel);

    char windowName_buf[sizeof(MyStr)];
    MyStr &windowName = *(MyStr *) windowName_buf;
    windowName.constructor(getWindowName());

    int try_level = 0;
    windowName.v_append(" - LibraryWindow ");
    windowName.v_append(buf);
    HWND hWnd = CreateWindowExA(
        0, "LibWindow", windowName.buf,
        0xCF0000u, minX, minY,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, getHInstance(), NULL
    );
    if (!hWnd) {
        try_level = -1;
        windowName.destructor();
        return *pstatus = -1, pstatus;
    }
    HMENU SystemMenu = GetSystemMenu(hWnd, 0);
    EnableMenuItem(SystemMenu, 3u, 0x402u);
    EnableMenuItem(SystemMenu, 5u, 0x402u);
    ShowWindow(hWnd, 5);
    UpdateWindow(hWnd);
    SetFocus(hWnd);
    this->hWnd = hWnd;
    this->created = 1;

    try_level = -1;
    windowName.destructor();
    return *pstatus = 0, pstatus;
}


int *dk2::CWindowTest::recreateBullfrog(int *pstatus) {
    int status;
    BullfrogWindow_destroy();
    GUID *guid = MyGame_instance.getSelectedGuid();
    if (*BullfrogWindow_create(&status, guid, 0, this->hWnd, NULL) < 0) {
        patch::log::err("recreate dd window failed");
        return *pstatus = -1, pstatus;
    }
    HWND prevHWnd = getHWindow();
    setHWindow(this->hWnd);
    if (*ge_dk2dd_init(&status, 640u, 480u, MyResources_instance.video_settings.display_bitnes, 0x58, NULL) < 0) {
        patch::log::err("dd init failed");
        return *pstatus = -1, pstatus;
    }
    setHWindow(prevHWnd);

    return *pstatus = 0, pstatus;
}


dk2::AABB *dk2::CWindowTest::getClientRect(AABB *a2) {
    RECT Rect;
    GetClientRect(this->hWnd, &Rect);
    a2->minX = Rect.left;
    a2->minY = Rect.top;
    a2->maxX = Rect.right;
    a2->maxY = Rect.bottom;
    return a2;
}


void dk2::CWindowTest::reallocBackSurfaceToWindowSize() {
    if ( (client_rect_initialized & 1) == 0 ) {
        client_rect.left = 0;
        client_rect_initialized |= 1u;
        client_rect.top = 0;
        client_rect.right = 0;
        client_rect.bottom = 0;
//        atexit(nullsub_9);
    }
    RECT Rect;
    GetClientRect(this->hWnd, &Rect);
    if(patch::big_resolution_fix::enabled) {  // patch from Ember project
//        int width = Rect.right - Rect.left;
//        int height = Rect.bottom - Rect.top;
//        if (width != api::g_width || height != api::g_height) {
//            printf("FIX: GetClientRect: l=%d, t=%d, r=%d, b=%d => create surf %dx%d but game expect to work with buffers %dx%d\n",
//                   Rect.left, Rect.top, Rect.right, Rect.bottom,
//                   width, height,
//                   api::g_width, api::g_height
//            );
//            Rect.right = Rect.left + api::g_width;
//            Rect.bottom = Rect.top + api::g_height;
//        }
    }
    if (client_rect.left != Rect.left
        || client_rect.right != Rect.right
        || client_rect.top != Rect.top
        || client_rect.bottom != Rect.bottom) {
        GetClientRect(this->hWnd, &Rect);
        client_rect = Rect;
        if (this->pCurOffScreenSurf) {
            int status;
            MyDdSurface_release(&status, &this->pCurOffScreenSurf->dd_surf);
            this->pCurOffScreenSurf = NULL;
        }
    }
    if (!this->pCurOffScreenSurf) {
        GetClientRect(this->hWnd, &Rect);
        int status;
        static_assert((DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE) == 0x6000u);
        if (*MyDdSurface_createOffScreenSurface(
                &status,
                Rect.right - Rect.left,
                Rect.bottom - Rect.top,
                DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE,
                &this->offScreenSurf.dd_surf) < 0)
            return;
        this->pCurOffScreenSurf = &this->offScreenSurf;
        setCurOffScreen(&this->offScreenSurf);
    }
}


int dk2::CWindowTest::isNeedBlt() {
    return g_isNeedBlt_fullscr;
}

void dk2::CWindowTest::copyToWindowSurf() {
    if ( g_isWinSurfCopying )
        __debugbreak();
    g_isWinSurfCopying = 1;
    callWinEvent_ev1_ty0(this->pCurOffScreenSurf);
    POINT pt{0, 0};
    ClientToScreen(this->hWnd, &pt);
    RECT Rect;
    GetClientRect(this->hWnd, &Rect);
    RECT v6_rect {
        pt.x + Rect.left,
        pt.y + Rect.top,
        pt.x + Rect.right,
        pt.y + Rect.bottom
    };
    int status;
    static_MyDdSurfaceEx_Blt(
        &status, &g_primarySurf, &v6_rect,
        this->pCurOffScreenSurf, NULL, 0
    );
    callWinEvent_ev1_ty1(this->pCurOffScreenSurf);
    g_isWinSurfCopying = 0;
}

int *dk2::CWindowTest::fillWithColor(int *pstatus, RECT *a3_rect, Bgraf *a4_bgrau) {
    MyDdSurfaceEx* f58_pCurOffScreenSurf = this->pCurOffScreenSurf;
    if (!f58_pCurOffScreenSurf) return *pstatus = 0, pstatus;
    MySurface* surf = f58_pCurOffScreenSurf->updateDesc();
    AABB v13 {0, 0, (int) surf->dwWidth, (int) surf->dwHeight};
    RECT dstRect;
    if (a3_rect) {
        AABB aabb {a3_rect->left, a3_rect->top, a3_rect->right, a3_rect->bottom};
        AABB v14;
        dstRect = *(RECT*) aabb.intersection(&v14, &v13);
    } else {
        dstRect = {0, 0, (LONG) surf->dwWidth, (LONG) surf->dwHeight};
    }
    MyDdSurfaceEx_fillWithColor(pstatus, this->pCurOffScreenSurf, &dstRect, *a4_bgrau, 0);
    return pstatus;
}


int *dk2::CWindowTest::probably_do_show_window_ev0_7(int *pstatus, AABB *rect) {
    int status;

    if (this->created) {
        if (this->pCurOffScreenSurf) {
            MyDdSurface_release(&status, &this->pCurOffScreenSurf->dd_surf);
            this->pCurOffScreenSurf = NULL;
        }
        this->created = 0;
        BullfrogWindow_destroy();
        DestroyWindow(this->hWnd);

        GUID* guid = MyGame_instance.getSelectedGuid();
        BullfrogWindow_create(&status, guid, 1, NULL, NULL);
    }

    AABB* v5_rect = rect;
    if (*this->create(&status, rect) < 0) {
        patch::log::err("create window failed");
        return *pstatus = -1, pstatus;
    }

    if (*this->recreateBullfrog(&status) < 0) {
        return *pstatus = -1, pstatus;
    }

    POINT screenPoint = {0, 0};
    ClientToScreen(this->hWnd, &screenPoint);

    AABB clientRect;
    clientRect.constructor();
    this->getClientRect(&clientRect);

    AABB res;
    clientRect.appendPoint(&res, &screenPoint);

    Bgraf color;
    MyDdSurfaceEx_fillWithColor(
        &status, &g_primarySurf, (RECT*) &clientRect,
        *color.constructor(200, 200, 200, 0), 0);
    if (status < 0) {
        patch::log::err("MyDdSurfaceEx_fillWithColor failed");
        return *pstatus = -1, pstatus;
    }

    this->reallocBackSurfaceToWindowSize();
    if (this->pCurOffScreenSurf) {
        auto* desc = this->pCurOffScreenSurf->updateDesc();
        screenPoint.x = 0;
        screenPoint.y = 0;

        clientRect.constructor();
        clientRect.minY = screenPoint.y;
        clientRect.maxX = desc->dwWidth;
        clientRect.minX = screenPoint.x;
        clientRect.maxY = desc->dwHeight;
        MyDdSurfaceEx_fillWithColor(
            &status, this->pCurOffScreenSurf, (RECT*) &clientRect,
            Bgraf{200, 200, 200, 0xFF, 0}, 0);
    }
    callWinEvent_ev0_ty7_aq1(
        v5_rect->maxX - v5_rect->minX,
        v5_rect->maxY - v5_rect->minY,
        MyResources_instance.video_settings.display_bitnes
    );

    return *pstatus = 0, pstatus;
}

dk2::MyDdSurfaceEx *dk2::CWindowTest::getCurOffScreenSurf() {
    return this->pCurOffScreenSurf;
}

void dk2::CWindowTest::recreate() {
    if (!this->created) return;
    int status;
    if (this->pCurOffScreenSurf) {
        MyDdSurface_release(&status, &this->pCurOffScreenSurf->dd_surf);
        this->pCurOffScreenSurf = 0;
    }
    this->created = 0;
    BullfrogWindow_destroy();
    DestroyWindow(this->hWnd);
    GUID* SelectedGuid = MyGame_instance.getSelectedGuid();
    BullfrogWindow_create(&status, SelectedGuid, 1, NULL, NULL);
}

