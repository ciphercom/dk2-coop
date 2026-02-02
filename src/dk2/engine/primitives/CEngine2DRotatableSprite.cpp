//
// Created by DiaLight on 2/2/2026.
//
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "dk2/engine/primitive/2d/CEngine2DRotatableSprite.h"
#include "dk2/SceneObject2E.h"
#include "dk2/MyScaledSurface.h"
#include "dk2/MyCESurfScale.h"
#include <cmath>
#include <algorithm>

dk2::Vec3f operator *(const dk2::Mat3x3f &mat, const dk2::Vec3f &a3) {
    return dk2::Vec3f {
            mat.m[0][0] * a3.x + mat.m[1][0] * a3.y + mat.m[2][0] * a3.z,
            mat.m[0][1] * a3.x + mat.m[1][1] * a3.y + mat.m[2][1] * a3.z,
            mat.m[0][2] * a3.x + mat.m[1][2] * a3.y + mat.m[2][2] * a3.z,
    };
}
dk2::Vec3f operator +(const dk2::Vec3f &v1, const dk2::Vec3f &v2) {
    return dk2::Vec3f {
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z,
    };
}

// rotating icon of newly discovered room
int *dk2::CEngine2DRotatableSprite::_addRenderObj(int a2, SceneObject2E *a3_sceneObj) {
    this->f8_x8 /= 2;
    this->fA_yA /= 2;

    float fx = (float) this->f8_x8;
    float fy = (float) this->fA_yA;

    Mat3x3f zRotMat;
    zRotMat.init_rotationMat(2/*Z_axis*/, this->fC_zRotAngle);
    Vec3f pos = {(float) this->f4_x, (float) this->f6_y, this->f10_z};

    Vec3f vertex4 = zRotMat * Vec3f {-fx, fy, 0.0} + pos;
    Vec3f vertex3 = zRotMat * Vec3f {fx, fy, 0.0} + pos;
    Vec3f vertex2 = zRotMat * Vec3f {fx, -fy, 0.0} + pos;
    Vec3f vertex1 = zRotMat * Vec3f {-fx, -fy, 0.0} + pos;
    return SceneObject2E_add2dSpriteRenderObj(
            a3_sceneObj,
            &vertex1,
            &vertex2,
            &vertex3,
            &vertex4,
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            0.0, 1.0,
            &this->f14_vec14,
            this->f20_x20, this->f24_y24
    );
}

void __cdecl dk2::CEngine2DRotatableSprite_create(
        int16_t a1_posX, int16_t a2_posY,
        int a3_x, int a4_y,
        float a5_zRotAngle, float a6_posZ, int a7_surfIdx,
        Vec3f a8_vec,
        float a11_x, float a12, int a13_y) {
    if (a3_x > 0 && a4_y > 0) {
        CEngine2DRotatableSprite *newObj = (CEngine2DRotatableSprite *) MyHeap_alloc(0x2C);
        CEngine2DRotatableSprite *obj = newObj;
        if (newObj) {
            ((CEngine2DPrimitive *) newObj)->constructor();
            obj->f4_x = a1_posX;
            obj->f6_y = a2_posY;
            obj->f10_z = a6_posZ;
            obj->fC_zRotAngle = a5_zRotAngle;
            *(void **) obj = CEngine2DRotatableSprite::vftable;
            obj->f8_x8 = a3_x;
            obj->fA_yA = a4_y;
            obj->f14_vec14 = a8_vec;
            obj->f14_vec14.x = obj->f14_vec14.x * 255.0;
            obj->f14_vec14.y = obj->f14_vec14.y * 255.0;
            obj->f14_vec14.z = obj->f14_vec14.z * 255.0;
            obj->f24_y24 = a13_y;
            obj->f20_x20 = a11_x;
        } else {
            obj = NULL;
        }
        obj->f0_parent = g_pCEngine2DPrimitive;
        g_pCEngine2DPrimitive = (CEnginePrimitiveBase *) obj;
        MyScaledSurface *scaledSurf = MyEntryBuf_MyScaledSurface_getByIdx(a7_surfIdx);

        int scaleIdx;
        {
//            int f15_prob_height = scaledSurf->prob_height;
//            float a3a = (double) f15_prob_height * a12 - -0.0009765625 - 0.49998999 - -12582912.0;
//            scaleIdx = (((DWORD) a3a) & 0x7FFFFF) - 0x400000;
//            if (scaleIdx < 0) scaleIdx = 0;
//            if (scaleIdx >= f15_prob_height) scaleIdx = f15_prob_height - 1;
            // above is optimized version of
            scaleIdx = std::clamp<int>(
                    std::round((double) scaledSurf->prob_height * a12),
                    0, scaledSurf->prob_height - 1
            );
        }
        MyCESurfHandle *surf = scaledSurf->scaledSurfArr[scaleIdx].surfScaledArr[0];
        int fC_drawFlags = scaledSurf->drawFlags;
        if ((MyDirectDraw_instance.flags & 1) != 0)
            fC_drawFlags = fC_drawFlags & ~0x300 | 0x200;
        MyCESurfHandle_static_addToHashList_flagsOr400(surf, fC_drawFlags);
        int idx = SceneObject2E_count;
        int v21_drawFlags = scaledSurf->drawFlags;
        if (SceneObject2E_count >= SceneObject2EList_instance.maxCount) {
            SceneObject2EList_instance.objects2EToDraw_enlarge(SceneObject2E_count);
            idx = SceneObject2E_count;
        }
        SceneObject2E &sobj = SceneObject2EList_instance.arr[idx];
        sobj.mesh = obj;
        sobj.f2C_ = 0;
        sobj.lod__triangleCount = 2;
        sobj.numVertsEx = 4;
        sobj.drawFlags_x2[0] = v21_drawFlags;
        sobj.renMode = g_renMode_7820A0;
        sobj.surfhCount = 1;
        sobj.propsCount = 1;
        sobj.numTextureSamplers_x2[0] = 1;
        sobj.surfh_x4[0] = surf;
        sobj.zeroOrM1 = 0;
        SceneObject2E_count++;
    }
}


