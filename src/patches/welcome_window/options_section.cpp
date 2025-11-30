//
// Created by DiaLight on 11/29/2025.
//

#include "options_section.h"
#include "imgui.h"


void Tooltip(const char* desc) {
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    Tooltip(desc);
}

void OptionsSection::load() {
    _roots.clear();
    _options.clear();
    struct root_idx_t {
        int idx;
        std::map<std::string, int> byCategories;
    };
    std::map<std::string, root_idx_t> byRoots;
    flame_config::iterateDefinedOptions([&](flame_config::defined_flame_option& opt) {
        if(opt.group == flame_config::OG_HiddenState) return;
        if(opt.group == flame_config::OG_GameProgress) return;
        std::string path(opt.path);

        root_idx_t *pRoot = nullptr;
        {
            auto pos = path.find(':');
            std::string root;
            if(pos != std::string::npos) {
                root = path.substr(0, pos);
                path = path.substr(pos + 1);
            }
            auto root_it = byRoots.find(root);
            if(root_it == byRoots.end()) {
                auto &r = _roots.emplace_back();
                r.name = root;
                auto it2 = byRoots.insert(std::make_pair(
                    root,
                    root_idx_t {(int) _roots.size() - 1}
                    ));
                root_it = it2.first;
            }
            pRoot = &root_it->second;
        }
        auto& categories = _roots[pRoot->idx].categories;

        int categoryIdx = -1;
        {
            std::string category;
            auto pos = path.rfind(':');
            if(pos != std::string::npos) {
                category = path.substr(0, pos);
                path = path.substr(pos + 1);
            }

            auto it = pRoot->byCategories.find(category);
            if(it == pRoot->byCategories.end()) {
                auto &cat = categories.emplace_back();
                cat.name = category;
                auto it2 = pRoot->byCategories.insert(std::make_pair(category, categories.size() - 1));
                it = it2.first;
            }
            categoryIdx = it->second;
        }
        auto& options = categories[categoryIdx].options;
        options.emplace_back(path, &opt, opt.value);
    });
    for(auto& root : _roots) {
        for(auto& cat : root.categories) {
            for(auto& o : cat.options) {
                _options.insert(std::make_pair(o.opt->path, &o));
            }
        }
    }
}

void OptionsSection::save() {
    for(auto& root : _roots) {
        if(!root.isChanged) continue;
        for(auto& cat : root.categories) {
            for(auto& o : cat.options) {
                if(o.value == o.opt->value) continue;
                flame_config::set_option(o.opt->path, o.value);
            }
        }
    }
    if(flame_config::changed()) flame_config::save();
}

static char *formatStrId(const std::string &name, bool isChanged) {
    static char str_id[256];
    str_id[0] = '\0';
    char *p = str_id;
    p = strcat(p, name.c_str());
    if(isChanged) p = strcat(p, "*");
    strcat(p, "###");
    p = strcat(p, name.c_str());
    return str_id;
}

void OptionsSection::render() {
    if (ImGui::BeginTabBar("AllOptions", ImGuiTabBarFlags_None)) {
        for(auto &root : _roots) {
            if (ImGui::BeginTabItem(formatStrId(root.name, root.isChanged))) {
                root.isChanged = false;
                for(auto &cat : root.categories) {
                    bool open = true;
                    if(!cat.name.empty()) {
                        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                        open = ImGui::TreeNode(cat.name.c_str());
                    }
                    if (open) {
                        int i = 0;
                        for(auto &o : cat.options) {
                            ImGui::PushID(i++);
                            auto& opt = *o.opt;
                            bool isDefault = o.value == opt.defaultValue;
                            if(isDefault) ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(64./255, 64./255, 72./255, 138./255));
                            bool isChanged = o.value == opt.value;
                            if(isChanged) ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(33./255, 79./255, 102./255, 138./255));
                            switch (o.value.ty) {
                            case flame_config::VT_None: break;
                            case flame_config::VT_String: {
                                ImGui::SetNextItemWidth(200);
                                if(ImGui::InputText(o.name.c_str(), o.value.str_value.data(), o.value.str_value.capacity() + 1, ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) -> int {
                                        auto& str = *(std::string *)data->UserData;
                                        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                                            // Resize string callback
                                            // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
                                            IM_ASSERT(data->Buf == str.c_str());
                                            str.resize(data->BufTextLen);
                                            data->Buf = (char*) str.c_str();
                                        }
                                        return 0;
                                    }, &o.value.str_value)) {
                                    //                                        printf("changed str %s: \"%s\" %d/%d\n", opt.path, o.value.str_value.data(), o.value.str_value.size(), o.value.str_value.capacity());
                                    if(o.changed) o.changed();
                                }
                                if(opt.help && *opt.help) {ImGui::SameLine(); HelpMarker(opt.help);}
                                break;
                            }
                            case flame_config::VT_Boolean: {
                                ImGui::SetNextItemWidth(200);
                                if(ImGui::Checkbox(o.name.c_str(), &o.value.bool_value)) {
                                    //                                        printf("changed bool %s\n", opt.path);
                                    if(o.changed) o.changed();
                                }
                                if(opt.help && *opt.help) {ImGui::SameLine(); HelpMarker(opt.help);}
                                break;
                            }
                            case flame_config::VT_Int: {
                                ImGui::SetNextItemWidth(200);
                                if(ImGui::DragInt(o.name.c_str(), &o.value.int_value, 1)) {
                                    //                                        printf("changed int %s: %d\n", opt.path, o.value.int_value);
                                    if(o.changed) o.changed();
                                }
                                if(opt.help && *opt.help) {ImGui::SameLine(); HelpMarker(opt.help);}
                                break;
                            }
                            case flame_config::VT_Float: {
                                ImGui::SetNextItemWidth(200);
                                if(ImGui::DragFloat(o.name.c_str(), &o.value.float_value, 0.1)) {
                                    //                                        printf("changed float %s: %.2f\n", opt.path, o.value.float_value);
                                    if(o.changed) o.changed();
                                }
                                if(opt.help && *opt.help) {ImGui::SameLine(); HelpMarker(opt.help);}
                                break;
                            }
                            }
                            if(isChanged) ImGui::PopStyleColor(1);
                            if(isDefault) ImGui::PopStyleColor(1);

                            if(!isChanged) {
                                root.isChanged = true;
                                ImGui::SameLine(0, 0);
                                ImGui::PushStyleColor(ImGuiCol_Button, 0);
                                if (ImGui::Button("*")) {
                                    o.value = opt.value;
                                }
                                ImGui::PopStyleColor(1);
                                Tooltip("Changed. Click: reset changes");
                            }
                            if(!isDefault) {
                                ImGui::SameLine();
                                if (ImGui::Button("D")) {
                                    o.value = opt.defaultValue;
                                }
                                Tooltip("Click: reset to default");
                            }
                            ImGui::PopID();
                        }
                        if(!cat.name.empty()) ImGui::TreePop();
                    }
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}
void OptionsSection::update_changes() {
    for(auto &root : _roots) {
        root.isChanged = false;
        for(auto &cat : root.categories) {
            for(auto &o : cat.options) {
                auto& opt = *o.opt;
                //                    bool isDefault = o.value == opt.defaultValue;
                bool isChanged = o.value == opt.value;
                if(!isChanged) {
                    root.isChanged = true;
                    break;
                }
            }
            if(root.isChanged) break;
        }
    }
}
bool OptionsSection::isChanged() {
    for(auto &root : _roots) {
        if(!root.isChanged) continue;
        return true;
    }
    return false;
}
void OptionsSection::reset_defaults() {
    for(auto& root : _roots) {
        for(auto& cat : root.categories) {
            for(auto& o : cat.options) {
                if(o.value == o.opt->defaultValue) continue;
                o.value = o.opt->defaultValue;
            }
        }
    }
}
