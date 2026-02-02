//
// Created by DiaLight on 1/9/2026.
//

#ifndef FLAME_WINEVENTHANDLERS_H
#define FLAME_WINEVENTHANDLERS_H

#include <cstdint>
#include <dk2/MyDdSurfaceEx.h>
#include <dk2/MyDxDevice.h>


namespace dk2 {

    void callWinEvent_ev0_ty4(int isActivated);
    void callWinEvent_ev0_ty5(int f4);
    void callWinEvent_ev0_ty6(int f4);
    void callWinEvent_ev0_ty7_aq0(uint32_t width, uint32_t height, uint32_t displayBitness);
    void callWinEvent_ev0_ty7_aq1(uint32_t width, uint32_t height, uint32_t displayBitness);

    void callWinEvent_ev1_ty0(MyDdSurfaceEx *surf);
    void callWinEvent_ev1_ty1(MyDdSurfaceEx *surf);
    void callWinEvent_ev1_ty2(MyDdSurfaceEx *surf);
    void callWinEvent_ev1_ty3(MyDdSurfaceEx *surf);

    void callWinEvent_ev5_ty11(MyDxDevice *dev);

    void __stdcall static_MyWindow_Event07_cb(int listNum, void *arg, void *obj);

}


#endif // FLAME_WINEVENTHANDLERS_H
