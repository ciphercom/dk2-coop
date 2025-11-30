//
// Created by DiaLight on 10/12/2025.
//

#include <lodepng.h>
#include <map>
#include <thread>
#include <tools/flame_config.h>
#include <xutility>
#include "MyTimer.h"
#include "alt_res_section.h"
#include "options_section.h"
#include "patches/welcome_window/resources/resources.h"
#include "tools/bug_hunter/MyVersionInfo.h"
#include "tools/last_error.h"
#include "welcome_window_imgui.h"

static ImVec2 operator -(const ImVec2& l, const ImVec2& r) { return {l.x - r.x, l.y - r.y}; }
static ImVec2 operator +(const ImVec2& l, const ImVec2& r) { return {l.x + r.x, l.y + r.y}; }
static ImVec2 operator /(const ImVec2& l, float v) { return {l.x / v, l.y / v}; }
static ImVec2 operator *(const ImVec2& l, float v) { return {l.x * v, l.y * v}; }


int countCores(DWORD_PTR value) {
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

DWORD_PTR selectNCores(DWORD_PTR systemMask, int n) {
    DWORD_PTR result = 0;
    for (int i = 0, c = 0; i < (sizeof(result) * 8) && c < n; ++i) {
        if((systemMask & (1 << i)) == 0) continue;
        result |= 1 << i;
        ++c;
    }
    return result;
}

struct WelcomeWindow {
    ImGuiIO& io;
    ImVec4 &clear_color;
    bool &done;
    patch::welcome_window::welcome_data_t& _data;

    bool is_settings = false;

    SIZE _bg_size{};
    ImTextureID _bg = ImTextureID_Invalid;
    OptionsSection _opt;

    std::vector<std::string> _data_modes;
    int _menuRes_current_data_mode = 0;
    int _gameRes_current_data_mode = 0;
    bool DDrawCompat_detected = false;

    AltResSection _altRes;

    WelcomeWindow(ImGuiIO& io, ImVec4 &clear_color, bool &done, patch::welcome_window::welcome_data_t& data) :
        io(io), clear_color(clear_color), done(done), _data(data), _altRes(_opt) {
        // disable imgui creating files
        io.IniFilename = NULL;
        io.LogFilename = NULL;

        HMODULE mod = GetModuleHandle("flame.dll");
        if(mod) {
            HRSRC myResource = ::FindResource(mod, MAKEINTRESOURCE(IDR_WELCOME__MAIN_BACKGROUND), RT_RCDATA);
            if(HGLOBAL myResourceData = ::LoadResource(mod, myResource)) {
                DWORD size = SizeofResource(mod, myResource);
                if(void *data = ::LockResource(myResourceData)) {
                    _bg = patch::welcome_window::LoadTextureFromBuffer(data, size, _bg_size);
                    UnlockResource(data);
                }
                FreeResource(myResourceData);
            }
        }

        if(HMODULE ddraw = GetModuleHandleA("ddraw.dll")) {
            MyVersionInfo ver(ddraw);
            if(ver.open()) {
                auto desc = ver.queryValue("FileDescription");
                DDrawCompat_detected = desc.contains("DDrawCompat");
            }
        }
    }

    void load_options() {
        _opt.load();
        {
            _data_modes.clear();
            _data_modes.emplace_back("unset");
            for(auto& mode : _data.modes) {
                auto& str = _data_modes.emplace_back();
                str.append(std::to_string(mode.width));
                str.append("x");
                str.append(std::to_string(mode.height));
            }

            menuRes_updateDisplayMode();
            _opt._options[o_menuRes.path]->changed = [this] { menuRes_updateDisplayMode(); };

            gameRes_updateDisplayMode();
            _opt._options[o_gameRes.path]->changed = [this] { gameRes_updateDisplayMode(); };
            _opt._options[op_Screen_Width]->changed = [this] { gameRes_updateDisplayMode(); };
            _opt._options[op_Screen_Height]->changed = [this] { gameRes_updateDisplayMode(); };

        }
        {
            _altRes.updateAvailable();
            _altRes.updateItems();
            _opt._options[o_altResources.path]->changed = [this] { _altRes.updateItems(); };
        }
    }
    void menuRes_updateDisplayMode() {
        _menuRes_current_data_mode = 0;
        auto &menuRes = _opt._options[o_menuRes.path]->value.str_value;
        if(!menuRes.empty()) {
            for (int i = 0; i < _data_modes.size(); ++i) {
                if(i == 0) continue;  // skip unset
                if(_data_modes[i] == menuRes) {
                    _menuRes_current_data_mode = i;
                    break;
                }
            }
            return;
        }
    }
    void gameRes_autoConfigureDk2Options(size_t width, size_t height) {
        {  // dk2 enabled options
            if(width >= 1024 && height >= 768) _opt._options[op_Res_1024_768_Enabled]->value.bool_value = true;
            if(width >= 1280 && height >= 1024) _opt._options[op_Res_1280_1024_Enable]->value.bool_value = true;
            if(width >= 1600 && height >= 1200) _opt._options[op_Res_1600_1200_Enable]->value.bool_value = true;
        }
        {  // dk2 screen mode
            // 0: 400x300
            // 1: 512x384
            // 2: 640x480 if VRAM > 2mb
            // 3: 800x600 if VRAM > 3mb
            // 4: 1024x768 if VRAM > 6mb  (most stable)
            // 5: 1280x1024 if VRAM > 10mb  (crashes the game)
            // 6: 1600x1200 if VRAM > 14mb  (fonts not loading)
            int dk2ScreenMode = 0;
            if(width >= 400 && height >= 300) dk2ScreenMode = 0;
            if(width >= 512 && height >= 384) dk2ScreenMode = 1;
            if(width >= 640 && height >= 480) dk2ScreenMode = 2;
            if(width >= 800 && height >= 600) dk2ScreenMode = 3;
            if(width >= 1024 && height >= 768) dk2ScreenMode = 4;
            if(width >= 1280 && height >= 1024) dk2ScreenMode = 5;
            if(width >= 1600 && height >= 1200) dk2ScreenMode = 6;
            _opt._options[op_Screen_Mode_Type]->value.int_value = dk2ScreenMode;
        }
        {  // gog disable if not supported
            _opt._options[o_gog_enabled.path]->value.bool_value = false;
            // only resolutions supported by gog patch:
            if (width == 640 && height == 480
                || width == 800 && height == 600
                || width == 1024 && height == 768
                || width == 1280 && height == 1024
                || width == 1600 && height == 1200
            ) {  // gog high resolution
                _opt._options[o_gog_enabled.path]->value.bool_value = true;
                _opt._options[o_gog_Video_HighRes.path]->value.bool_value = width > 1024 && height > 768;
            }
            if(_opt._options[o_windowed.path]->value.bool_value) _opt._options[o_gog_enabled.path]->value.bool_value = false;
        }
    }
    void gameRes_updateDisplayMode() {
        _gameRes_current_data_mode = 0;
        auto &gameRes = _opt._options[o_gameRes.path]->value.str_value;
        if(!gameRes.empty()) {
            for (int i = 0; i < _data_modes.size(); ++i) {
                if(i == 0) continue;  // skip unset
                if(_data_modes[i] == gameRes) {
                    _gameRes_current_data_mode = i;
                    break;
                }
            }
            return;
        }
        auto width = _opt._options[op_Screen_Width]->value.int_value;
        auto height = _opt._options[op_Screen_Height]->value.int_value;
        for (int i = 0; i < _data_modes.size(); ++i) {
            if(i == 0) continue;  // skip unset
            auto& mode = _data.modes[i - 1];
            if(mode.width == width && mode.height == height) {
                _gameRes_current_data_mode = i;
                break;
            }
        }
    }

    void simple_settings() {
        ImGui::SetNextItemWidth(200);
        if(ImGui::Combo("Menu Display mode", &_menuRes_current_data_mode, [](void *ctx, int idx) -> const char * {
                return ((WelcomeWindow *) ctx)->_data_modes[idx].c_str();
        }, this, _data_modes.size())) {
            auto &menuRes = _opt._options[o_menuRes.path]->value.str_value;
            if(_menuRes_current_data_mode) {
                size_t width = _data.modes[_menuRes_current_data_mode - 1].width;
                size_t height = _data.modes[_menuRes_current_data_mode - 1].height;
//                printf("manu selected mode: %dx%d\n", width, height);
                menuRes = std::to_string(width) + "x" + std::to_string(height);
            } else {
//                printf("manu selected mode: reset\n");
                menuRes.clear();
            }
            _opt.update_changes();
        }

        ImGui::SetNextItemWidth(200);
        if(ImGui::Combo("Game Display mode", &_gameRes_current_data_mode, [](void *ctx, int idx) -> const char * {
                return ((WelcomeWindow *) ctx)->_data_modes[idx].c_str();
        }, this, _data_modes.size())) {
            auto &gameRes = _opt._options[o_gameRes.path]->value.str_value;
            if(_gameRes_current_data_mode) {
                size_t width = _data.modes[_gameRes_current_data_mode - 1].width;
                size_t height = _data.modes[_gameRes_current_data_mode - 1].height;
//                printf("manu selected mode: %dx%d\n", width, height);
                _opt._options[op_Screen_Width]->value.int_value = width;
                _opt._options[op_Screen_Height]->value.int_value = height;
                gameRes_autoConfigureDk2Options(width, height);
            } else {
//                printf("manu selected mode: reset\n");
                gameRes.clear();
            }
            _opt.update_changes();
        }

        ImGui::SetNextItemWidth(200);
        if(ImGui::Checkbox("Windowed mode", &_opt._options[o_windowed.path]->value.bool_value)) {
//            printf("changed bool %s\n", o_windowed.path);
            if(_gameRes_current_data_mode) {
                size_t width = _data.modes[_gameRes_current_data_mode - 1].width;
                size_t height = _data.modes[_gameRes_current_data_mode - 1].height;
                gameRes_autoConfigureDk2Options(width, height);
            }
            _opt.update_changes();
        }
        ImGui::Text("Gog patch enabled: %s", _opt._options[o_gog_enabled.path]->value.bool_value ? "true" : "false");
//        iNeedMultithreading();

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader(_altRes.isChanged() ? "Alternative Resources*###resources" : "Alternative Resources###resources", ImGuiTreeNodeFlags_None)) {
            _altRes.render();
        }
    }

    void iNeedMultithreading() {
        if (ImGui::CollapsingHeader("I need multithreading", ImGuiTreeNodeFlags_None)) {
            bool isSingleCore = false;
            if(_opt._options[o_single_core.path]->value.bool_value) {
                isSingleCore = true;
            } else if(_opt._options[o_gog_enabled.path]->value.bool_value && _opt._options[o_gog_Misc_SingleCore.path]->value.bool_value) {
                isSingleCore = true;
            } else {
                DWORD_PTR affinity = 0;
                DWORD_PTR sysAffinity = 0;
                if(GetProcessAffinityMask(GetCurrentProcess(), &affinity, &sysAffinity)) {
                    int processCores = countCores(affinity);
                    int systemCores = countCores(sysAffinity);
                    ImGui::Text("Affinity process: %d, system: %d", processCores, systemCores);
                    if(processCores <= 1) isSingleCore = true;
                    if(ImGui::SliderInt("affinity", &processCores, 1, systemCores)) {
                        if(processCores != countCores(affinity)) {  // changed
                            DWORD_PTR mask = selectNCores(sysAffinity, processCores);
                            SetProcessAffinityMask(GetCurrentProcess(), mask);
                        }
                    }
                    if(DDrawCompat_detected) {
                        ImGui::TextWrapped("DDrawCompat detected! Use its config to control affinity or remove ddraw.dll from \"Dungeon Keeper 2\" directory");
                    }
                }
            }
            ImGui::Text("Single core: %s", isSingleCore ? "true" : "false");
            {ImGui::SameLine(); HelpMarker("The Flame and Gog patches separately limit the number of cores. To try multithreading, disable single-core in both patches. The pre-configured Affinity mask also affects multithreading");}
        }
    }

    void settings_tick() {
//        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        if (ImGui::BeginTabBar("Settings", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Simple settings")) {
                simple_settings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(_opt.isChanged() ? "All options*###all_changes" : "All options###all_changes")) {
                _opt.render();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void settings_bottom_tick() {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.f, 8.f));
        static float MaxWidth = 0;
        float NewMaxWidth = 10.0f;
        {
            if(ImGui::Button(_opt.isChanged() ? "Ok*###ok" : "Ok###ok", {MaxWidth, 0})) {
                // save changes
                _opt.save();
                is_settings = false;
            }
            NewMaxWidth = std::max(NewMaxWidth, ImGui::GetItemRectSize().x);
        }
        {
            ImGui::SameLine((ImGui::GetWindowWidth() - MaxWidth) / 2);
            if(ImGui::Button("Default", {MaxWidth, 0})) {
                _opt.reset_defaults();
                _opt.update_changes();
                gameRes_updateDisplayMode();
                menuRes_updateDisplayMode();
            }
            NewMaxWidth = std::max(NewMaxWidth, ImGui::GetItemRectSize().x);
        }
        {
            static float LocalButtonWidth = 100.0f;
            const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine(ImGui::GetWindowWidth() - ItemSpacing - LocalButtonWidth);
            if(ImGui::Button("Cancel", {MaxWidth, 0})) {
                is_settings = false;
            }
            LocalButtonWidth = ImGui::GetItemRectSize().x;  // Get the actual width for next frame.
            NewMaxWidth = std::max(NewMaxWidth, ImGui::GetItemRectSize().x);
        }
        MaxWidth = NewMaxWidth;
        ImGui::PopStyleVar();
    }

    void main_tick() {
        float btn_height = 60;
        static ImVec2 LocalButtonSize = {100.0f, 40.0f};
        ImGui::SetCursorPos(ImVec2{
            (ImGui::GetWindowSize().x - LocalButtonSize.x) / 2,
            ImGui::GetWindowSize().y - btn_height - 20
        });
        {
            ImGui::BeginGroup();
            if(ImGui::Button("S", {btn_height, btn_height})) {
                load_options();
                is_settings = true;
            }
            Tooltip("Settings");
            ImGui::SameLine();
            if(ImGui::Button("Play", {300, btn_height})) {
                _data.play = true;
                done = true;
            }
            ImGui::EndGroup();
        }
        LocalButtonSize = ImGui::GetItemRectSize();
    }

    void draw() {
        {
            auto& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1, 0.1, 0.1, 0.5);
        }
        if(is_settings) {
            if(_bg != ImTextureID_Invalid) {
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
                ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                if (ImGui::Begin("bg", NULL, ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                                     ImGuiWindowFlags_NoInputs)) {
                    ImGui::Image(_bg, io.DisplaySize);
                    ImGui::End();
                }
                ImGui::PopStyleVar();
            }

            float bt_h = 40;

            ImVec2 pz(0.0f, 0.0f);
            ImVec2 sz(io.DisplaySize.x, io.DisplaySize.y - bt_h);
            ImGui::SetNextWindowPos(pz);
            ImGui::SetNextWindowSize(sz);
            if(ImGui::Begin("settings_win", NULL, ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_HorizontalScrollbar)) {
                settings_tick();
                ImGui::End();
            }

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImVec2 bt_pz(0.0f, io.DisplaySize.y - bt_h);
            ImVec2 bt_sz(io.DisplaySize.x, bt_h);
            ImGui::SetNextWindowPos(bt_pz);
            ImGui::SetNextWindowSize(bt_sz);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 3.f));
            if(ImGui::Begin("bottom_win", NULL, ImGuiWindowFlags_NoDecoration)) {
                settings_bottom_tick();
                ImGui::End();
            }
            ImGui::PopStyleVar();
        } else {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
            if(ImGui::Begin("main_win", NULL, ImGuiWindowFlags_NoDecoration)) {
                if(_bg != ImTextureID_Invalid) {
                    ImGui::Image(_bg, io.DisplaySize);
                }
                main_tick();
                ImGui::End();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleVar();
        }
    }
    MyTimer timer;
    time_t lastTime = 0;

    void tick() {
        time_t tick_start = timer.now_ms();
        if(lastTime) {
            int fps = 45;
            int mspf = 1000 / fps;
            time_t delta = mspf - (tick_start - lastTime);
//            printf("%d\n", delta);
            timer.sleep(delta);
        }
        lastTime = timer.now_ms();
        draw();
    }

};

void *patch::welcome_window::create(ImGuiIO& io, ImVec4 &clear_color, bool &done, welcome_data_t& data) {
    return new WelcomeWindow(io, clear_color, done, data);
}
void patch::welcome_window::tick(void *ptr) {
    ((WelcomeWindow *) ptr)->tick();
}
void patch::welcome_window::destroy(void *ptr) {
    delete (WelcomeWindow *) ptr;
}

