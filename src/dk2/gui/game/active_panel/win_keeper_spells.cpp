//
// Created by DiaLight on 6/20/2025.
//

#include <dk2_functions.h>
#include <memory>
#include <vector>
#include <dk2/button/button_types.h>
#include <dk2/gui/ButtonCfg.h>
#include <dk2/gui/WindowCfg.h>
#include <dk2/button/CButton.h>
#include <dk2/CDefaultPlayerInterface.h>
#include <dk2/CWorld.h>
#include <dk2/entities/CPlayer.h>
#include <patches/network_possession.h>
#include <cstring>
#include "../game_layout.h"

namespace {
    std::vector<dk2::ButtonCfg> buttons;
    std::unique_ptr<dk2::WindowCfg> window;

    /** Match native panel ordering: unavailable entries and spells 5/14 do not occupy slots. */
    bool possessionBlocked(uint32_t slot, dk2::CDefaultPlayerInterface *controller) {
        if (!patch::network_possession::enabled()) return false;
        if (!controller || !controller->pCWorld) std::abort();
        auto *world = controller->pCWorld;
        auto *keeper = static_cast<dk2::CPlayer *>(world->v_getCTag_508C40(controller->playerTagId));
        if (!keeper) std::abort();
        if (!keeper->creaturePossessed) return false;

        // Native scroll 417730 increments B5 and uses E5 as capacity. Swapping them
        // preserves their product but makes the slot bound zero on the first page.
        uint32_t page, pageSize;
        const auto *bytes = reinterpret_cast<const unsigned char *>(controller);
        std::memcpy(&page, bytes + 0xB5, sizeof(page));
        std::memcpy(&pageSize, bytes + 0xE5, sizeof(pageSize));
        uint32_t index = 0;
        const int count = world->v_getAvailableSpellCount();
        for (int i = 1; i <= count; ++i) {
            const auto spell = static_cast<uint8_t>(world->v_fun_50DE00(i));
            if (keeper->fun_4BAFA0(spell) == 1 || spell == 5 || spell == 14) continue;
            ++index;
            if (spell == 2)
                return patch::network_possession::buttonBlocked(true, keeper->creaturePossessed,
                    slot, page, pageSize, index);
        }
        return false;
    }

    /** Refresh native availability first so exit/death restores normal research and mana restrictions. */
    uint32_t __cdecl updateSpellButton(uint32_t address, dk2::CDefaultPlayerInterface *controller) {
        const auto result = dk2::fun_f34_4113B0(address, controller);
        auto *button = reinterpret_cast<dk2::CButton *>(address);
        if (button->f5D_isVisible && possessionBlocked(button->f63_clickHandler_arg1, controller))
            button->f5D_isVisible = 2; // Native visible-but-disabled spell rendering.
        return result;
    }

    /** Recheck occupancy at click time, including changes since the last button refresh. */
    char __cdecl clickSpellButton(int slot, int arg, dk2::CDefaultPlayerInterface *controller) {
        if (possessionBlocked(slot, controller)) return 0;
        return dk2::CButton_handleLeftClick_411140(slot, arg, controller);
    }
}

dk2::WindowCfg *ActivePanel_KeeperSpells_layout() {
    if (window) return window.get();

    buttons.emplace_back() = {
        BT_CClickButton, 1, 0, dk2::CButton_handleLeftClick_417730, NULL, 0, 0, 0x00000001, 0x00000002, 0,
        0, 0, 64, 440, 24, 0, 6, 440, 0, dk2::fun_f34_410A60, dk2::CButton_render_4129E0, 0x0000000A, 0, 0x00000000, 0x00000000, 14
    };
    buttons.emplace_back() = {
        BT_CClickButton, 1, 0, dk2::CButton_handleLeftClick_417730, NULL, 0, 0, 0xFFFFFFFF, 0x00000002, 0,
        0, 0, 64, 440, 24, 0, 6, 440, 0, dk2::fun_f34_410A60, dk2::CButton_render_4129E0, 0x0000000B, 0, 0x00000000, 0x00000000, 14
    };
    buttons.emplace_back() = {
        BT_CClickButton, 2, 0, clickSpellButton, NULL, 0, 0, 0x00000001, 0x00000000, 0,
        0, 0, 256, 128, 0, 0, 212, 128, 0, updateSpellButton, dk2::CButton_render_412FC0, 0x0000000E, 175, 0x00000000, 0x00000000, 5
    };

    buttons.emplace_back() = EngOfButtonList;

    window = std::make_unique<dk2::WindowCfg>();
    *window = {
        GWID_ActivePanel_KeeperSpells, 0, 1, 1828, 1452, 732, 468, 0, 0, 732, 468, 0, NULL, dk2::CWindow_getPanelItemsCount, 0,
        0, 0, 0, 0, 0, buttons.data(), 0
    };
    return window.get();
}
