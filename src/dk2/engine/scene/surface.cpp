//
// Created by DiaLight on 1/22/2026.
//

#include <cmath>
#include <cfenv>
#include "fpu_control.h"
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "dk2/SurfaceHolder.h"
#include "dk2/CEngineDDSurface.h"
#include "dk2/CEngineSurface.h"
#include "dk2/MyCESurfHandle.h"
#include "dk2/SurfHashListItem.h"
#include "dk2/ToDraw.h"


void __cdecl dk2::textureCache_init(float a1_cullTo, float a2_cullFrom, const char *textureCacheDir, int a4_heapSize) {
    uint16_t _cw;
    if ( g_7651F8_initialized ) {
        g_7651F8_initialized = 0;
        if ( g_unused7651F0 ) {
            void *v4 = g_unused7651F0;
//            releaseList_574550(g_unused7651F0);
            MyHeap_free(v4);
        }
        g_unused7651F0 = 0;
        ScreenObjectArr_instance.itemsCount = 0;
        NewObj571B3B_clear();
        MyDirectDraw_destroy();
        SceneObject2EList_ToDrawList_static_destroy();
//        ret_void_0args_0(v5);
//        ret_void_0args_0(v6);
        surfaces_cleanup();
        MyEntryBuf_MyScaledSurface_reset();
        MyStringHashMap_MyMeshResourceHolder_cleanup();
        MyHeap_static_destroy();
        _cw = g_FpuControlWord;
        _FPU_SETCW(_cw);
    }

    MyHeap_static_init(a4_heapSize);
    g_heapSize = a4_heapSize;
    g_cullFrom = a2_cullFrom;
    g_cullTo = a1_cullTo;

    if ( g_7651F8_initialized ) {
        g_7651F8_initialized = 0;
        if ( g_unused7651F0 ) {
            void *v7 = g_unused7651F0;
//            releaseList_574550(g_unused7651F0);
            MyHeap_free(v7);
        }
        g_unused7651F0 = NULL;
        ScreenObjectArr_instance.itemsCount = 0;
        NewObj571B3B_clear();
        MyDirectDraw_destroy();
        SceneObject2EList_ToDrawList_static_destroy();
//        ret_void_0args_0(v8);
//        ret_void_0args_0(v9);
        surfaces_cleanup();
        MyEntryBuf_MyScaledSurface_reset();
        MyStringHashMap_MyMeshResourceHolder_cleanup();
        MyHeap_static_destroy();
        _cw = g_FpuControlWord;
        _FPU_SETCW(_cw);
    }
    _FPU_GETCW(_cw);
    *(uint16_t *) &g_FpuControlWord = _cw;

    g_pNewObj571B3B_root = NULL;

    fpu_control_t fw;
    _FPU_GETCW(fw);
    fw &= ~(_FPU_EXTENDED | _FPU_RC_DOWN | _FPU_RC_UP);
    static_assert((_FPU_EXTENDED | _FPU_RC_DOWN | _FPU_RC_UP) == 0xF00);
    _FPU_SETCW(fw);

    g_vec_760A98.x = 65025.0;
    g_vec_760A98.y = 65025.0;
    g_vec_760A98.z = 65025.0;
    DDGAMMARAMP gammaRamp;
    for (int i = 0; i < 256; ++i) {
//        __int64 v12 = (__int64)(dk2::_pow((double)i * 0.00390625, 1.0) * 65536.0);
        __int64 v12 = (__int64)(dk2::_pow((double)i / 256, 1.0) * (256 * 256));
        gammaRamp.red[i] = v12;
        gammaRamp.green[i] = v12;
        gammaRamp.blue[i] = v12;
    }
    g_gammaRamp = gammaRamp;
    if ( g_sc_is3dInitialized && (MyDirectDraw_instance.flags & 2) != 0 )
        g_sc_dd_gamma_control->SetGammaRamp(0, &g_gammaRamp);
    g_doAdd_0x741_objToScene = 1;
    idxMap780E78_init();
    MyTextures_resetCacheDir(textureCacheDir);
    MyEntryBuf_MyScaledSurface_static_alloc();
    meshHolderList_init();
    sceneObj2E_f21_to_triangleIndices_init();
    RenderData_instance_arr_init();
    zArrs_init(1, 0.0, 1.0, 0.0, 0.1, 0);
    SceneObject2EList_ToDrawList_static_init();
    g_ddSceneSessionId = 1;
    g_7651F8_initialized = 1;
    g_pNewObj571B3B = NULL;
    g_pNewObj571B3B_end = NULL;
    g_unused760AA4 = 0;
}


