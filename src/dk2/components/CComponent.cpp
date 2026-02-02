//
// Created by DiaLight on 1/22/2026.
//
#include "dk2/components/CComponent.h"
#include "dk2_functions.h"
#include "dk2_globals.h"


int __cdecl dk2::CComponent_onKeyboardActionWithCtrl(int keyCode, int isPressed, int controlKeyFlags, CComponent *comp) {
    if (!isPressed) return 1;
    if (keyCode == 1) {
        comp->release();
        return 1;
    }
    if (keyCode == 28 && controlKeyFlags == 2) {
        MyWindow_instance.prepareScreen2();
        return 1;
    }
    ProbablyConsole_instance.kbAction(keyCode, comp);
    return 1;
}
