//
// Created by DiaLight on 10/9/2025.
//

#include "dk2/MyCESurfHandle.h"
#include "dk2/MyStringHashMap_MyCESurfHandle_entry.h"
#include "dk2/MyStringHashMap_entry.h"
#include "dk2/CEngineSurface.h"
#include "dk2/CEngineSurfaceScaler.h"
#include "dk2_functions.h"
#include "dk2_globals.h"


void dk2::MyCESurfHandle::resolveSurface() {
    if (this->cesurf != nullptr) return;
    if ((this->reductionLevel_andFlags & 0x80) != 0) {
        this->create();
        return;
    }
    char *f0_name = MyStringHashMap_MyCESurfHandle_instance.entries.buf[this->mapIdx].name;
    char texName[256];
    sprintf(texName, "%sMM%d", f0_name, this->reductionLevel_andFlags & 7);
    int f24_reductionLevel = MyDirectDraw_instance_devTexture.reductionLevel;
    if (this->surfWidth8) {
        if (MyDirectDraw_instance_devTexture.reductionLevel <= (this->reductionLevel_andFlags & 7) || !this->nextByReduction) {
            this->cesurf = (CEngineSurface*) MyTextures_instance.loadCompressed(texName);
            if (!this->cesurf) {
                int EntryIdx = ((MyStringHashMap *)&MyStringHashMap_MyCESurfHandle_instance)->getEntryIdx(f0_name);
                MyStringHashMap_MyCESurfHandle_instance.entries.buf[EntryIdx].value->loadPrescaled();
            }
        }
        return;
    }
    int v5 = MyTextures_instance.texNameToFileOffsetMap.getEntryIdx(texName);
    if ( v5 < 0 ) {
        this->loadPrescaled();
        return;
    }
    dk2::_fseek(
        MyTextures_instance.fileHandle,
        (int) MyTextures_instance.texNameToFileOffsetMap.entries.buf[v5].value,
        0);
    int width = -1;
    int height = -1;
    readFromFile(&width, 4u, 1u, MyTextures_instance.fileHandle);
    readFromFile(&height, 4u, 1u, MyTextures_instance.fileHandle);

    BOOL needsRescale = width < 16;
    unsigned __int8 height_ = height;
    if ( height < 16 ) needsRescale = 1;
    if ( width > 128 ) needsRescale = 1;
    if ( height > 128 ) needsRescale = 1;
    if ( needsRescale ) {
        this->loadPrescaled();
        return;
    }
    this->surfWidth8 = width;
    this->surfHeight8 = height_;
    this->createReduction();
    if (!f24_reductionLevel || !this->nextByReduction) {
        this->cesurf = (CEngineSurface*) MyTextures_instance.loadCompressed(texName);
    }
}


void dk2::MyCESurfHandle::loadPrescaled() {
    char surfName[2048];
    sprintf(surfName, MyStringHashMap_MyCESurfHandle_instance.entries.buf[this->mapIdx].name);
    char isPrescaled = 0;
    char *prescaledPos = strstr(surfName, "PRESCALED_TO");
    if (prescaledPos) {
        *prescaledPos = 0;
        isPrescaled = 1;
    }
    if (isPrescaled) {
        MyCESurfHandle *f4_value = MyStringHashMap_MyCESurfHandle_instance.entries.buf[
                ((MyStringHashMap *) &MyStringHashMap_MyCESurfHandle_instance)->getEntryIdx(surfName)
        ].value;

        f4_value->reductionLevel_andFlags |= 0x200u;
        CEngineSurface *f0_cesurf = f4_value->cesurf;
        if (f4_value->cesurf == NULL) {
            f4_value->resolveSurface();
            for (f0_cesurf = f4_value->cesurf; !f4_value->cesurf; f0_cesurf = f4_value->cesurf) {
                f4_value->reductionLevel_andFlags &= ~0x200;
                f4_value = f4_value->nextByReduction;
                f0_cesurf = f4_value->cesurf;
                f4_value->reductionLevel_andFlags |= 0x200u;
                if (f4_value->cesurf != NULL) break;
                f4_value->resolveSurface();
            }
        }
        CEngineSurfaceScaler_instance.prescaleWigth = f0_cesurf->width;
        CEngineSurfaceScaler_instance.prescaleHeight = f0_cesurf->height;
        ((CEngineSurfaceBase *) CEngineSurfaceScaler_instance.orig_128x128_8a8r8g8b)->paintSurf(f0_cesurf, 0, 0);
        CEngineSurfaceScaler_instance.isScaled = 0;
        f4_value->reductionLevel_andFlags &= ~0x200u;
    } else {
        MySurface *Png = static_CBridge_loadPng(surfName);
        CEngineSurfaceScaler_instance.convertCopyFrom(Png);
        this->surfWidth8 = Png->size.w;
        this->surfHeight8 = Png->size.h;
        this->createReduction();
    }

    int f24_reductionLevel = MyDirectDraw_instance_devTexture.reductionLevel;
    char surfType;
    if ((this->flags & 8) != 0) {
        if (!g_isSupports_16bit) return;
        surfType = 2;
    } else {
        surfType = (this->reductionLevel_andFlags & 0x80) == 0;
    }

    if (this->surfWidth8 != CEngineSurfaceScaler_instance.prescaleWigth
        || this->surfHeight8 != CEngineSurfaceScaler_instance.prescaleHeight) {
        scaleImg(
                CEngineSurfaceScaler_instance.orig_128x128_8a8r8g8b->v_lockBuf(), CEngineSurfaceScaler_instance.prescaleWigth, CEngineSurfaceScaler_instance.prescaleHeight, 128,
                CEngineSurfaceScaler_instance.scaled_128x128_8a8r8g8b->v_lockBuf(), this->surfWidth8, this->surfHeight8, 128
        );
        CEngineSurfaceScaler_instance.isScaled = 1;
    }
    this->cesurf = (CEngineSurface *) CEngineSurfaceScaler_instance.copyScaledWithType(
            surfType, this->surfWidth8, this->surfHeight8);
    char v32[1024];
    sprintf(
            v32,
            "%sMM%d",
            MyStringHashMap_MyCESurfHandle_instance.entries.buf[this->mapIdx].name,
            this->reductionLevel_andFlags & 7);
    if (f24_reductionLevel > 0) {
        if (this->nextByReduction) {
            if (this->cesurf) this->cesurf->v_scalar_destructor(1u);
            this->cesurf = NULL;
        }
    }

    int dstWidth = this->surfWidth8 >> 1;
    int dstHeight = this->surfHeight8 >> 1;
    int v17_reductionLevel = f24_reductionLevel - 1;
    for (MyCESurfHandle *f18_nextByReduction = this->nextByReduction; f18_nextByReduction; f18_nextByReduction = f18_nextByReduction->nextByReduction) {
        if (dstWidth != CEngineSurfaceScaler_instance.prescaleWigth
            || dstHeight != CEngineSurfaceScaler_instance.prescaleHeight) {
            void *dstBuf = CEngineSurfaceScaler_instance.scaled_128x128_8a8r8g8b->v_lockBuf();
            int srcHeight = CEngineSurfaceScaler_instance.prescaleHeight;
            int srcWidth = CEngineSurfaceScaler_instance.prescaleWigth;
            void *srcBuf = CEngineSurfaceScaler_instance.orig_128x128_8a8r8g8b->v_lockBuf();
            scaleImg(srcBuf, srcWidth, srcHeight, 128, dstBuf, dstWidth, dstHeight, 128);
            CEngineSurfaceScaler_instance.isScaled = 1;
        }
        CEngineSurfaceBase *v21 = CEngineSurfaceScaler_instance.copyScaledWithType(surfType, dstWidth, dstHeight);
        unsigned int v22 = f18_nextByReduction->reductionLevel_andFlags;
        f18_nextByReduction->cesurf = (CEngineSurface *) v21;
        sprintf(
                v32,
                "%sMM%d",
                MyStringHashMap_MyCESurfHandle_instance.entries.buf[f18_nextByReduction->mapIdx].name,
                v22 & 7);
        if (v17_reductionLevel > 0) {
            if (f18_nextByReduction->nextByReduction) {
                if (f18_nextByReduction->cesurf) f18_nextByReduction->cesurf->v_scalar_destructor(1u);
                f18_nextByReduction->cesurf = NULL;
            }
        }
        --v17_reductionLevel;
        dstWidth >>= 1;
        dstHeight >>= 1;
    }
}

