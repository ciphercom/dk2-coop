//
// Created by DiaLight on 1/24/2026.
//
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "dk2/SurfHashList2.h"
#include "dk2/MyCESurfHandle.h"
#include "dk2/SurfaceHolder.h"
#include "dk2/CEngineSurfaceBase.h"


int dk2::SurfHashList2::_probablySort() {
    int handleCount = this->calcHandleCountToFitHolder();
    int handleCount_ = handleCount;
    for (MyCESurfHandle *f4_surfh_first = this->surfh_first; f4_surfh_first; f4_surfh_first = f4_surfh_first->nextByHashList) {
        MyCESurfHandle *curReducted = f4_surfh_first;
        for (int handleLeft = handleCount_;
             (handleLeft > 0 || (curReducted->reductionLevel_andFlags & 7) < MyDirectDraw_instance_devTexture.reductionLevel)
             && curReducted->nextByReduction;
             --handleLeft) {
            curReducted = curReducted->nextByReduction;
        }
        f4_surfh_first->curReduction = curReducted;
        double f28_padNorm_width = curReducted->padNorm_width;
        curReducted->reductionLevel_andFlags |= 8;
        if (f28_padNorm_width == 0.0)
            curReducted->holder_parent = 0;
        if (!curReducted->holder_parent)
            this->deleteHolder(curReducted);
    }
    SurfaceHolder *v9 = NULL;
    SurfaceHolder *v31 = NULL;
    while (this->f8count) {
        int surfCount = 0;
        for (int x = 0; x < 5; ++x) {
            for (int y = 0; y < 5; ++y) {
                for (MyCESurfHandle *i = this->arr5x5_surfh[x][y]; i; i = i->nextByHolder) ++surfCount;
            }
        }
        SurfaceHolder *fD8_holder_first = this->holder_first;
        int v17_minValue = 0x10000;
        SurfaceHolder *v16_minValueItem = this->holder_first;
        if (fD8_holder_first) {
            while (true) {
                int v18_value = fD8_holder_first->calcWeight();
                if (!v18_value) break;
                if (v18_value < v17_minValue) {
                    v17_minValue = v18_value;
                    v16_minValueItem = fD8_holder_first;
                }
                fD8_holder_first = fD8_holder_first->prev_;
                if (!fD8_holder_first) {
                    fD8_holder_first = v16_minValueItem;
                    break;
                }
            }
        } else {
            fD8_holder_first = v16_minValueItem;
        }
        if (!fD8_holder_first) {
            this->sub_593E90(v31);
            break;
        }

        if (SurfaceHolder *f10_prev = fD8_holder_first->next_)
            f10_prev->prev_ = fD8_holder_first->prev_;
        else
            this->holder_first = fD8_holder_first->prev_;

        if (SurfaceHolder *f14_next = fD8_holder_first->prev_)
            f14_next->next_ = fD8_holder_first->next_;
        fD8_holder_first->next_ = NULL;
        fD8_holder_first->prev_ = NULL;
        for (MyCESurfHandle *j = fD8_holder_first->surfh_first; j; j = fD8_holder_first->surfh_first) {
            fD8_holder_first->surfh_first = j->nextByHolder;
            bool v22 = (j->reductionLevel_andFlags & 8) == 0;
            j->nextByHolder = NULL;
            j->holder_parent = NULL;
            if (!v22)
                this->deleteHolder(j);
        }
        this->sub_593880(fD8_holder_first, 0, 0, this->holder_size, this->holder_size, 3);
        if (auto *fD0_ddsurf = this->ddsurf)
            fD8_holder_first->surf->paintSurf((CEngineSurface *) fD0_ddsurf, 0, 0);
        SurfaceHolder *v24 = v31;
        v31 = fD8_holder_first;
        fD8_holder_first->prev_ = v24;
    }
    v9 = v31;
    if (v9) {
        while (true) {
            SurfaceHolder *v25 = this->holder_first;
            SurfaceHolder *v26 = v9->prev_;
            if (v25)
                v25->next_ = v9;
            SurfaceHolder *v27 = this->holder_first;
            v9->next_ = NULL;
            v9->prev_ = v27;
            this->holder_first = v9;
            if (!v26)
                break;
            v9 = v26;
        }
    }
    if (this->surfh_first) {
        MyCESurfHandle *f10_nextByHashList;
        do {
            MyCESurfHandle *v28 = this->surfh_first;
            v28->reductionLevel_andFlags &= ~0x10u;
            v28->curReduction->reductionLevel_andFlags &= ~8u;
            f10_nextByHashList = v28->nextByHashList;
            this->surfh_first = f10_nextByHashList;
        } while (f10_nextByHashList);
    }
    return ++SurfHashList_sortTick;
}
