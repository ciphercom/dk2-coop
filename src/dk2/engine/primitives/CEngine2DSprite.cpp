//
// Created by DiaLight on 1/24/2026.
//
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "dk2/engine/primitive/2d/CEngine2DSprite.h"
#include "dk2/SceneObject2E.h"
#include "dk2/MyScaledSurface.h"
#include "dk2/MyCESurfScale.h"
#include <cmath>
#include <algorithm>


void __cdecl dk2::CEngine2DSprite_create(
        int a1_posX,
        int a2_posY,
        int a3_texW,
        int a4_texH,
        float a5_z,
        int idx,
        Vec3f v54_vec,
        float a10_a,
        float a11,
        unsigned int a12) {
    if (a3_texW <= 0) return;
    while (true) {
        CEnginePrimitiveBase *obj = NULL;
        if (a4_texH <= 0) break;
        CEngine2DSprite *newObj = (CEngine2DSprite *) MyHeap_alloc(sizeof(CEngine2DSprite));
        int try_level = 0;
        if (newObj) {
            obj = ((CEngine2DSprite *) newObj)->constructor(
                    a1_posX, a2_posY,
                    a3_texW, a4_texH,
                    a5_z,
                    v54_vec.x, v54_vec.y, v54_vec.z,
                    a10_a,
                    a12,
                    g_viewPos_765218.x, g_viewEnd_765228.x, g_viewPos_765218.y, g_viewEnd_765228.y);
        }
        try_level = -1;
        obj->f0_parent = g_pCEngine2DPrimitive;
        g_pCEngine2DPrimitive = obj;
        BOOL needUpscale = 1;
        if( a1_posX < g_viewPos_765218.x) needUpscale = 0;
        if (a3_texW + a1_posX > g_viewEnd_765228.x) needUpscale = 0;
        if (a2_posY < g_viewPos_765218.y) needUpscale = 0;
        if (a4_texH + a2_posY > g_viewEnd_765228.y) needUpscale = 0;
        MyScaledSurface *Prescaled;
        if (needUpscale)
            Prescaled = MyDblNamedSurface_createPrescaled(idx, a3_texW, a4_texH);
        else
            Prescaled = MyEntryBuf_MyScaledSurface_getByIdx(idx);
        int scaleIdx;
        {
//            int f15_prob_height = Prescaled->prob_height;
//            float a12a = (double) f15_prob_height * a11 - -0.0009765625 - 0.49998999 - -12582912.0;
//            scaleIdx = (((DWORD) a12a) & 0x7FFFFF) - 0x400000;
//            if (scaleIdx < 0) scaleIdx = 0;
//            if (scaleIdx >= f15_prob_height) scaleIdx = f15_prob_height - 1;
            // above is optimized version of
            scaleIdx = std::clamp<int>(
                    std::round((double) Prescaled->prob_height * a11),
                    0, Prescaled->prob_height - 1
            );
        }
        unsigned int a12b = Prescaled->drawFlags;
        MyCESurfHandle *surfh = Prescaled->scaledSurfArr[scaleIdx].surfScaledArr[0];
        int v23;
        int v24;
        transformFlags(a12, (uint32_t *) &v23, (uint32_t *) &v24);
        unsigned int flags = v24 | a12b & v23;
        if ((MyDirectDraw_instance.flags & 1) != 0)
            flags = flags & ~0x300u | 0x200;
        MyCESurfHandle_static_addToHashList_flagsOr400(surfh, flags);
        int objIdx = SceneObject2E_count;
        if (SceneObject2E_count >= SceneObject2EList_instance.maxCount) {
            SceneObject2EList_instance.objects2EToDraw_enlarge(SceneObject2E_count);
            objIdx = SceneObject2E_count;
        }
        SceneObject2E &sobj = SceneObject2EList_instance.arr[objIdx];
        sobj.mesh = obj;
        sobj.f2C_ = 0;
        sobj.lod__triangleCount = 2;
        sobj.numVertsEx = 4;
        sobj.drawFlags_x2[0] = flags;
        sobj.renMode = g_renMode_7820A0;
        sobj.surfhCount = 1;
        sobj.propsCount = 1;
        sobj.numTextureSamplers_x2[0] = 1;
        sobj.surfh_x4[0] = surfh;
        sobj.zeroOrM1 = 0;
        SceneObject2E_count++;
        if ((a12 & 0x40) == 0)
            break;
        a12 = a12 & ~0x42u | 2;
        a5_z = a5_z - static_765214;
    }
}

void dk2::CEngine2DSprite::_addRenderObj(int a2, SceneObject2E *a3_obj) {
    if (f4_posX + this->f8_texW <= this->f24_viewLeft24) return;
    if (f6_posY + this->fA_texH <= this->f28_viewTop28) return;
    if (f4_posX >= this->f26_viewRight26) return;
    if (f6_posY >= this->f2A_viewBottom2A) return;
    if ((MyDirectDraw_instance.flags & 1) != 0) {
        if ((a3_obj->drawFlags_x2[0] & 0x2000) != 0)
            this->createTriangle34(0, a3_obj);
        else
            this->addVert18Triangle(0, a3_obj);
    } else {
        this->addVert1CTriangle(0, a3_obj);
    }
}
