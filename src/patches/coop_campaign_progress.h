#pragma once
namespace patch::coop_campaign_progress {
/** Only the host of a fresh Shared Keeper campaign uses native campaign persistence. */
inline bool eligible(bool campaign, bool host, int mode, bool frontend3d, bool frontend2d, bool saved) {
    return campaign && host && mode == 3 && !frontend3d && !frontend2d && !saved;
}
void install();
}
