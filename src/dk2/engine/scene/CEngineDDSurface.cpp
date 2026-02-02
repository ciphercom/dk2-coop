//
// Created by DiaLight on 1/24/2026.
//
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "dk2/CEngineDDSurface.h"


dk2::CEngineDDSurface *dk2::CEngineDDSurface::constructor(uint32_t width, uint32_t height, MyCEngineSurfDesc *desc, int a5) {
    this->fC_desc = desc;
    *(void **) this = CEngineSurfaceBase::vftable;
    this->width = width;
    this->height = height;
    this->lineWidth = width * desc->bytesize;
    int try_level = 0;
    this->f14 = a5;
    *(void **) this = CEngineDDSurface::vftable;
    DWORD flags = 4103;
    DWORD caps;
    if ((a5 & 1) != 0) {
        caps = 0x405008;
        flags = 0x21007;
    } else {
        caps = 0x1800;
    }

    DDSURFACEDESC2 surfDesc;
    memset(&surfDesc, 0, sizeof(surfDesc));
    surfDesc.dwWidth = width;
    surfDesc.ddsCaps.dwCaps = caps;
    surfDesc.dwSize = 0x7C;
    surfDesc.dwHeight = height;
    surfDesc.dwFlags = flags;
    surfDesc.ddpfPixelFormat = desc->ddPixFmt;
    surfDesc.dwMipMapCount = 1;
    if (MyDirectDraw_instance_devTexture.dd4->CreateSurface(&surfDesc, &this->ddSurf, NULL)) {
        this->ddSurf = NULL;
        this->surfCreated = 1;
    } else {
        if ((a5 & 1) != 0)
            this->ddSurf->QueryInterface(CLSID_IDirect3DTexture2, (LPVOID *) &this->devTex);
        this->surfCreated = 0;
    }
    return this;
}

void dk2::CEngineDDSurface::destructor() {
    *(void **) this = CEngineDDSurface::vftable;
    if (!this->surfCreated) {
        if ((this->f14 & 1) != 0)
            this->devTex->Release();
        this->ddSurf->Release();
    }
    *(void **) this = CEngineSurfaceBase::vftable;
}
