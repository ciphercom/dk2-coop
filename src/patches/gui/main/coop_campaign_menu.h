#pragma once
#include <cstdint>
namespace dk2 { struct CButton; struct CFrontEndComponent; }

/** Native-menu adapters for the opt-in original Co-op Campaign route. */
namespace patch::coop_campaign_menu {
int __cdecl enter(uint32_t, int, dk2::CFrontEndComponent *front);
/** Suspend Create until the host accepts a mission through the native campaign interface. */
bool beginHostSelection(dk2::CFrontEndComponent *front);
/** Replace the native single-player launch with the pending co-op Create. */
bool commitHostSelection(dk2::CFrontEndComponent *front);
/** Abandon a pending host choice when leaving the native campaign map. */
void cancelHostSelection(dk2::CFrontEndComponent *front);
/** Discard a departed or failed session while staying in the co-op provider route. */
void clearSession(dk2::CFrontEndComponent *front);
/** Restore native lobby controls when abandoning the route. */
void reset(dk2::CFrontEndComponent *front);
/** Forget old window state only when the game has returned to its frontend. */
void onFrontendLoad();
/** Display the route name without changing the ordinary TCP/IP heading. */
int *__cdecl renderTitle(dk2::CButton *button, dk2::CFrontEndComponent *front);
/** Select the proven native transport map after hosting succeeds. */
void selectCarrier(dk2::CFrontEndComponent *front);
/** Keep the selected mission immutable until the lobby is abandoned. */
void restrictLobby(dk2::CFrontEndComponent *front);
/** Refuse incompatible mission choices before the native connection starts. */
bool canJoin(dk2::CFrontEndComponent *front, int index);
uint8_t __cdecl renderThumbnail(int button, int front);
void __cdecl renderMission(dk2::CButton *button, int front);
}
