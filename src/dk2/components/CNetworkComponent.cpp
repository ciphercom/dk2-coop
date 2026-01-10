//
// Created by DiaLight on 1/9/2026.
//

#include "dk2/components/CNetworkComponent.h"
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "patches/logging.h"


dk2::CComponent *dk2::CNetworkComponent::runUntilDestroy() {
    patch::log::dbg("enter CNetworkComponent");
    while (!this->is_component_destroy) {
        MyInputManagerCb_static_processInputs_setStaticListenersAndHandleDxActions(&this->static_listeners, 0, this, 0);
        if (MyGame_instance.selectSurfToRender()) {
            static_MyNBitTexture_f30(0, 0, "Network Component", MyGame_instance.colors.colorWhite);
            static_MyNBitTexture_f30(0, 32, "ESC.Quit", MyGame_instance.colors.colorWhite);
            ProbablyConsole_instance.drawConsole();
            Pos2i* MousePos = MyGame_instance.getMousePos();
            static_MyNBitTexture_f10(MousePos->x, MousePos->y, 8, MyGame_instance.colors.colorRed);
            MyGame_instance.getSurf_unlock();
            MyGame_instance.prepareScreen();
            MyGame_instance.surf_Blt();
        }
        if (isAppExitStatusSet())
            this->gotoComponent(NULL);
    }
    return this->f4_nextComponent;
}
