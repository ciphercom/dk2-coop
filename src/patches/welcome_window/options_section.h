//
// Created by DiaLight on 11/29/2025.
//

#ifndef FLAME_OPTIONS_SECTION_H
#define FLAME_OPTIONS_SECTION_H

#include <tools/flame_config.h>
#include <map>


extern flame_config::define_flame_option<bool> o_console;
extern flame_config::define_flame_option<bool> o_windowed;
extern flame_config::define_flame_option<bool> o_single_core;
extern flame_config::define_flame_option<std::string> o_menuRes;
extern flame_config::define_flame_option<std::string> o_gameRes;
extern flame_config::define_flame_option<bool> o_gog_enabled;
extern flame_config::define_flame_option<bool> o_gog_Video_HighRes;
extern flame_config::define_flame_option<bool> o_gog_Misc_SingleCore;
extern flame_config::define_flame_option<int> o_autosave;
extern flame_config::define_flame_option<bool> o_external_textures;
constexpr const char *op_Screen_Width = "registry:configuration:video:Screen_Width";
constexpr const char *op_Screen_Height = "registry:configuration:video:Screen_Height";
constexpr const char *op_Res_1024_768_Enabled = "registry:configuration:video:Res_1024_768_Enabled";
constexpr const char *op_Res_1280_1024_Enable = "registry:configuration:video:Res_1280_1024_Enable";
constexpr const char *op_Res_1600_1200_Enable = "registry:configuration:video:Res_1600_1200_Enable";
constexpr const char *op_Screen_Mode_Type = "registry:configuration:video:Screen_Mode_Type";

void Tooltip(const char* desc);
void HelpMarker(const char* desc);

struct OptionsSection {

    struct option_t {
        std::string name;
        flame_config::defined_flame_option *opt;
        flame_config::flame_value value;
        std::function<void()> changed;
    };
    struct category_t {
        std::string name;
        std::vector<option_t> options;
    };
    struct root_t {
        std::string name;
        std::vector<category_t> categories;
        bool isChanged = false;
    };

    std::vector<root_t> _roots;
    std::map<std::string, option_t*> _options;

    void load();
    void save();
    void render();
    void update_changes();
    bool isChanged();
    void reset_defaults();

};


#endif // FLAME_OPTIONS_SECTION_H
