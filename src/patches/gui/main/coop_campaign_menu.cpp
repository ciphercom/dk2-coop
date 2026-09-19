#include "coop_campaign_menu.h"
#include <dk2/MyMapInfo.h>
#include <dk2/network/MLDPLAY_SESSIONDESC.h>
#include <dk2/button/CButton.h>
#include <dk2/components/CFrontEndComponent.h>
#include <dk2/text/render/MyTextRenderer.h>
#include <dk2_functions.h>
#include <dk2_globals.h>
#include <patches/auto_network.h>
#include <patches/coop_campaign.h>
#include <cstdlib>
#include <cwchar>

namespace {
    // Remember native role-specific visibility so backing out cannot alter ordinary multiplayer.
    constexpr int restrictedButtons[] = {454, 453, 455, 528, 681};
    struct ButtonState {
        uint32_t visible;
        decltype(dk2::CButton::f49_leftClickHandler) left;
        decltype(dk2::CButton::f4D_rightClickHandler) right;
        uint32_t nextWindow;
        uint32_t exitOnClick;
    };
    ButtonState previousState[5]{};
    uint8_t previousMapPlayerCount = 0;
    bool visibilitySaved = false;
    bool choosingMission = false;
}

int __cdecl patch::coop_campaign_menu::enter(uint32_t, int, dk2::CFrontEndComponent *front) {
    patch::coop_campaign::enter();
    if (!patch::auto_network::openTcpIp(front)) patch::coop_campaign::leave();
    return 0;
}

bool patch::coop_campaign_menu::beginHostSelection(dk2::CFrontEndComponent *front) {
    if (!patch::coop_campaign::active() || patch::coop_campaign::mission()) return false;
    choosingMission = true;
    // The native Continue action owns unlocks, map interaction and briefing preparation.
    // Its 255 button sentinel permits programmatic dispatch without a hovered menu button.
    dk2::CButton_handleLeftClick_538000(11, MAKELONG(13, 255), front);
    return true;
}

bool patch::coop_campaign_menu::commitHostSelection(dk2::CFrontEndComponent *front) {
    if (!choosingMission) return false;
    auto &config = dk2::MyResources_instance.gameCfg;
    const int nativeId = dk2::g_petDungeonLevelIdx;
    const auto *name = patch::coop_campaign::nativeMissionName(nativeId);
    if (!patch::coop_campaign::active() || config.useFe_playMode != 1 ||
        !name || _wcsicmp(name, config.levelName) != 0) std::abort();
    patch::coop_campaign::selectMission(nativeId, config.levelName, config.useFe_unkTy);
    choosingMission = false;
    dk2::CSpeechSystem_instance.add_stop_handle0(90);
    // The native provider and its text inputs remain alive during campaign selection.
    // Restore its page and resume the pending Create rather than initialize another provider.
    front->_tableTy = 12;
    front->fun_5321A0(11, 7);
    dk2::CButton_handleLeftClick_changeMenu(32, 83, front);
    return true;
}

void patch::coop_campaign_menu::clearSession(dk2::CFrontEndComponent *front) {
    if (!patch::coop_campaign::active()) return;
    reset(front);
    // A new lobby requires a fresh native host selection and new native readiness.
    patch::coop_campaign::enter();
}

void patch::coop_campaign_menu::cancelHostSelection(dk2::CFrontEndComponent *front) {
    if (choosingMission) reset(front);
}

void patch::coop_campaign_menu::reset(dk2::CFrontEndComponent *front) {
    if (visibilitySaved) {
        front->b4_mapPlayersCount_goldDencity_loseHeartType =
            (front->b4_mapPlayersCount_goldDencity_loseHeartType & 0x0F) | previousMapPlayerCount;
        for (int i = 0; i < 5; ++i) {
            auto *button = front->findBtnBySomeId(restrictedButtons[i], 32);
            if (!button) std::abort();
            const auto &saved = previousState[i];
            button->f5D_isVisible = saved.visible;
            button->f49_leftClickHandler = saved.left;
            button->f4D_rightClickHandler = saved.right;
            button->f55__nextWindowIdOnClick = saved.nextWindow;
            button->f59__isExitOnClick = saved.exitOnClick;
        }
    }
    visibilitySaved = false;
    choosingMission = false;
    patch::coop_campaign::leave();
}

void patch::coop_campaign_menu::onFrontendLoad() {
    // The old window objects have gone; never restore their saved state into a new frontend.
    visibilitySaved = false;
    choosingMission = false;
    // Multiplayer debriefing's action 82 already destroys/reinitializes the native session.
    // Retain only the route so its Create action opens a fresh campaign selection afterward.
    patch::coop_campaign::returnToFrontend();
}

/** The same TCP/IP browser serves both routes; only co-op replaces its native title. */
int *__cdecl patch::coop_campaign_menu::renderTitle(dk2::CButton *button, dk2::CFrontEndComponent *front) {
    if (!patch::coop_campaign::active()) return dk2::CTextBox_renderTitle_536700(button, front);
    dk2::AABB screen, bounds;
    button->getScreenAABB(&screen);
    front->cgui_manager.scaleAabb_2560_1920(&bounds, &screen);
    uint8_t rendererBuffer[sizeof(dk2::MyTextRenderer)];
    auto &renderer = *reinterpret_cast<dk2::MyTextRenderer *>(rendererBuffer);
    renderer.constructor();
    int status;
    renderer.selectMyCR(&status, 0); // Center within the native heading bounds.
    renderer.selectMyTR(&status, 2);
    wchar_t label[] = L"Co-op Campaign";
    uint8_t text[64]{};
    if (!dk2::UniToMb_convert(label, text, sizeof(text))) std::abort();
    // Use the menu font without changing the shared font's color for subsequent controls.
    dk2::FontObj font(dk2::g_FontObj6_instance);
    font.reset_f10(&status);
    font.setColor(&status, &front->color3031E);
    dk2::setDrawSurface(&front->surf65_btnRenderOut);
    renderer.renderText(&status, &bounds, text, &font, nullptr);
    font.destructor();
    renderer.destructor();
    return nullptr;
}

void patch::coop_campaign_menu::selectCarrier(dk2::CFrontEndComponent *front) {
    if (!patch::coop_campaign::active()) return;
    for (int i = 0; i < front->mapsCount; ++i) {
        if (_wcsicmp(front->mapInfoArr[i].name, L"Gonzalez") != 0) continue;
        front->lobbySelectedMapIdx = i;
        front->lobbyMapWasChanged(i);
        // Keep the existing carrier setup intact; the co-op UI suppresses its unrelated thumbnail.
        front->loadMapThumbnail(front->getMapName());
        restrictLobby(front);
        return;
    }
    // Matching original installations must contain the transport carrier used by this fixture.
    std::abort();
}

void patch::coop_campaign_menu::restrictLobby(dk2::CFrontEndComponent *front) {
    if (!patch::coop_campaign::active()) return;
    if (!visibilitySaved)
        previousMapPlayerCount = front->b4_mapPlayersCount_goldDencity_loseHeartType & 0xF0;
    // Native Max Players text and lobby admission checks share this upper nibble.
    front->b4_mapPlayersCount_goldDencity_loseHeartType =
        (front->b4_mapPlayersCount_goldDencity_loseHeartType & 0x0F) | 0x20;
    for (int i = 0; i < 5; ++i) {
        auto *button = front->findBtnBySomeId(restrictedButtons[i], 32);
        if (!button) std::abort();
        if (!visibilitySaved) {
            previousState[i] = {button->f5D_isVisible, button->f49_leftClickHandler,
                button->f4D_rightClickHandler, button->f55__nextWindowIdOnClick, button->f59__isExitOnClick};
        }
        // Also prevent keyboard dispatch through native click handlers and automatic navigation.
        button->f49_leftClickHandler = nullptr;
        button->f4D_rightClickHandler = nullptr;
        button->f55__nextWindowIdOnClick = 0;
        button->f59__isExitOnClick = 0;
        button->f5D_isVisible = 0;
    }
    visibilitySaved = true;
}

void __cdecl patch::coop_campaign_menu::renderMission(dk2::CButton *button, int frontAddress) {
    if (!patch::coop_campaign::active()) {
        dk2::CButton_render_53F8B0(button, frontAddress);
        return;
    }
    auto *front = reinterpret_cast<dk2::CFrontEndComponent *>(frontAddress);
    // Native lobby updates may enable host controls again; keep the session mission immutable.
    restrictLobby(front);
    dk2::AABB bounds;
    button->getScreenAABB(&bounds);
    front->cgui_manager.scaleAabb_2560_1920(&bounds, &bounds);
    uint8_t rendererBuffer[sizeof(dk2::MyTextRenderer)];
    auto &renderer = *reinterpret_cast<dk2::MyTextRenderer *>(rendererBuffer);
    renderer.constructor();
    int status;
    renderer.selectMyCR(&status, 2);
    renderer.selectMyTR(&status, 2);
    // Match the native font encoding used by stock string-table and custom menu text.
    const auto *mission = patch::coop_campaign::mission();
    if (!mission) std::abort();
    uint8_t text[256]{};
    if (!dk2::UniToMb_convert(const_cast<wchar_t *>(mission->assetName), text, sizeof(text))) std::abort();
    dk2::setDrawSurface(&front->surf65_btnRenderOut);
    renderer.renderText(&status, &bounds, text, &dk2::g_FontObj5_instance, nullptr);
    renderer.destructor();
    // The native title renderer also publishes the carrier name used by map-availability packets.
    // Keep that handshake side effect even though this label displays the co-op mission instead.
    wcscpy(dk2::g_selectedMapName_0073FB90, front->getMapName());
}

bool patch::coop_campaign_menu::canJoin(dk2::CFrontEndComponent *front, int index) {
    if (index < 0 || index >= dk2::g_MLDPLAY_SESSIONDESC_arr_count) return false;
    const auto &sessionMap = dk2::g_MLDPLAY_SESSIONDESC_arr[index].mapInfo;
    const int nativeId = patch::coop_campaign::missionIdFromSessionTag(sessionMap.f6);
    if (!patch::coop_campaign::active()) {
        if (!nativeId) return true;
        // Both patched peers must opt into the campaign route or their launch missions differ.
        front->fun_536BA0(0, 0, 2079, 105, 0, 1, 0, 0, 0);
        return false;
    }
    const auto *missionName = patch::coop_campaign::nativeMissionName(nativeId);
    for (int i = 0; missionName && i < front->mapsCount; ++i) {
        const auto &map = front->mapInfoArr[i];
        if (map.nameHash != sessionMap.nameHash || wcslen(map.name) != sessionMap.nameLen) continue;
        if (_wcsicmp(map.name, L"Gonzalez") == 0) {
            // This is the host's immutable session choice, independent of client progress.
            patch::coop_campaign::selectMission(nativeId, missionName, 3);
            return true;
        }
        break;
    }
    // A stock session is a normal user choice; reject it before joining rather than crashing at launch.
    front->fun_536BA0(0, 0, 2079, 105, 0, 1, 0, 0, 0);
    return false;
}

uint8_t __cdecl patch::coop_campaign_menu::renderThumbnail(int button, int front) {
    if (patch::coop_campaign::active()) return 0;
    return dk2::CButton_render_537C30(button, front);
}
