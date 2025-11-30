//
// Created by DiaLight on 11/29/2025.
//

#include "alt_res_section.h"
#include <iostream>
#include "dk2/resources/DirIter.h"
#include "dk2_functions.h"
#include "options_section.h"

#define thread_local
#include "toml.hpp"


struct toml_type_config {
    using comment_type  = toml::preserve_comments;

    using boolean_type  = bool;
    using integer_type  = std::int64_t;
    using floating_type = double;
    using string_type   = std::string;

    template<typename T>
    using array_type = std::vector<T>;
    // template<typename K, typename T>
    // using table_type = std::unordered_map<K, T>;
    template<typename K, typename T>
    using table_type = std::map<K, T>;

    static toml::result<integer_type, toml::error_info>
    parse_int(const std::string& str, const toml::source_location &src, const std::uint8_t base) {
        return toml::read_int<integer_type>(str, src, base);
    }
    static toml::result<floating_type, toml::error_info>
    parse_float(const std::string& str, const toml::source_location &src, const bool is_hex) {
        return toml::read_float<floating_type>(str, src, is_hex);
    }
};
using toml_value = toml::basic_value<toml_type_config>;
using toml_table = typename toml_value::table_type;
using toml_array = typename toml_value::array_type;


AltResSection::AltResSection(OptionsSection &opt) : _opt(opt) {}

static int indexOf(const std::vector<std::string>& v, const std::string& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if(it != v.end()) {
        return it - v.begin();
    }
    return -1;
}
static int indexOf(const std::vector<std::unique_ptr<AltResource>>& v, const std::string& x) {
    auto it = std::find_if(v.begin(), v.end(), [&x](const std::unique_ptr<AltResource>& res) {
        return res->dirName == x;
    });
    if(it != v.end()) {
        return it - v.begin();
    }
    return -1;
}
static std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    if(s.empty()) return {};
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

toml_table getTable(toml_value &v, const std::string &name) {
    if(v.is_table() && v.contains(name)) {
        auto e = v.at(name);
        if(e.is_table()) return e.as_table();
    }
    return {};
}
std::string getString(toml_table &v, const std::string &name) {
    if(v.contains(name)) {
        auto e = v.at(name);
        if(e.is_string()) return e.as_string();
    }
    return "";
}
void loadResourceInfo(const std::string &file, AltResource &r) {
    try {
        toml_value toml = toml::parse<toml_type_config>(file + "/resource.toml");
        if(toml.is_table() && toml.contains("resource")) {
            auto e = toml.at("resource");
            if (e.is_table()) {
                auto res = e.as_table();
                r.name = getString(res, "name");
                r.version = getString(res, "version");
                r.authors = split(getString(res, "authors"), ";");
                r.description = getString(res, "description");
                r.has_info = !r.name.empty() || !r.version.empty() || !r.authors.empty() || !r.description.empty();
            }
        }
        r.has_info_file = true;
    } catch (const ::toml::file_io_error &e) {
        std::cout << "failed to load resource info: " << e.what() << std::endl;
    } catch (const ::toml::exception &e) {
        std::cout << "failed to parse resource info: " << e.what() << std::endl;
    }
}

void AltResSection::updateAvailable() {
    g_available.clear();
    CHAR searchPat[MAX_PATH];
    strcpy(searchPat, dk2::fs_getExeDir());
    strcat(searchPat, "flame\\resources\\*");
    dk2::DirIter iter;
    int status;
    if(*dk2::fs_DirIter_init(&status, searchPat, &iter, FILE_ATTRIBUTE_DIRECTORY) >= 0) {
        do {
            if(iter.lastSlashPos && *iter.lastSlashPos) {
                if(strchr(iter.lastSlashPos, '.') == NULL) {  // has no '.' char
                    auto item_id = (ImGuiID) g_available.size();
                    auto& r = *g_available.emplace_back(std::make_unique<AltResource>());
                    r.id = item_id;
                    r.dirName = iter.lastSlashPos;
                    loadResourceInfo(iter.path, r);
                    if(!r.authors.empty()) {
                        for (int i = 0; i < r.authors.size(); ++i) {
                            if(i != 0) r.authorsStr += ", ";
                            r.authorsStr += r.authors[i];
                        }
                    }
                }
            }
        } while(*dk2::fs_DirIter_next(&status, &iter) >= 0);
        dk2::fs_DirIter_destroy(&status, &iter);
    }
}
void AltResSection::updateItems() {
    _Items[0].clear();
    _Items[1].clear();
    _Selections[0].Clear();
    _Selections[1].Clear();
    auto& o = *_opt._options[o_altResources.path];
    auto &altRes = o.value.str_value;
    auto selected = split(altRes, ";");

    for(auto &res : g_available) {
        int idx = indexOf(selected, res->dirName);
        if(idx == -1) {
            _Items[0].push_back(res->id);
        }
    }
    for(auto &sel : selected) {
        int idx = indexOf(g_available, sel);
        if(idx != -1) {
            auto& res = g_available[idx];
            _Items[1].push_back(res->id);
        }
    }
}
void AltResSection::_updateOption() {
    std::string value;
    ImVector<ImGuiID>& items = _Items[1];
    for (int item_n = 0; item_n < items.Size; item_n++) {
        ImGuiID item_id = items[item_n];
        if(item_n != 0) value += ";";
        auto& res = g_available[item_id];
        value += res->dirName;
    }
    _opt._options[o_altResources.path]->value.str_value = value;
    _opt.update_changes();
}

static int __cdecl CompareItemsByValue(const void* lhs, const void* rhs) {
    const int* a = (const int*)lhs;
    const int* b = (const int*)rhs;
    return (*a - *b);
}

void AltResSection::SortItems(int n) {
    qsort(
        _Items[n].Data, (size_t)_Items[n].Size, sizeof(_Items[n][0]),
        CompareItemsByValue
    );
}
void AltResSection::render() {
    if (ImGui::BeginTable("split", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);    // Left side
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);      // Buttons
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);    // Right side
        ImGui::TableNextRow();

        static int last_focused_size = 0;
        AltResource *focused = NULL;
        int request_move_selected = -1;
        int request_move_all = -1;
        float child_height_0 = 0.0f;
        for (int side = 0; side < 2; side++) {
            // FIXME-MULTISELECT: Dual List Box: Add context menus
            // FIXME-NAV: Using ImGuiWindowFlags_NavFlattened exhibit many issues.
            ImVector<ImGuiID>& items = _Items[side];
            ImGuiSelectionBasicStorage& selection = _Selections[side];

            ImGui::TableSetColumnIndex((side == 0) ? 0 : 2);
            ImGui::Text("%s (%d)", (side == 0) ? "Available" : "Enabled", items.Size);
            if(side == 1) {
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::BeginItemTooltip()) {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
                    ImGui::TextUnformatted("The search for a resource file in alternative resources occurs from top to bottom. You can change the search order by dragging and dropping elements");
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }

            // Submit scrolling range to avoid glitches on moving/deletion
            const float items_height = ImGui::GetTextLineHeightWithSpacing();
            ImGui::SetNextWindowContentSize(ImVec2(0.0f, (items.Size + side) * items_height));

            bool child_visible;
            if (side == 0) {
                // Left child is resizable
                ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetFrameHeightWithSpacing() * 4), ImVec2(FLT_MAX, FLT_MAX));
                child_visible = ImGui::BeginChild("0", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 5), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY);
                child_height_0 = ImGui::GetWindowSize().y;
            } else {
                // Right child use same height as left one
                child_visible = ImGui::BeginChild("1", ImVec2(-FLT_MIN, child_height_0), ImGuiChildFlags_FrameStyle);
            }
            if (child_visible) {
                ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_None;
                ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, items.Size);
                ApplySelectionRequests(ms_io, side);

                for (int item_n = 0; item_n < items.Size; item_n++) {
                    ImGuiID item_id = items[item_n];
                    bool item_is_selected = selection.Contains(item_id);
                    ImGui::SetNextItemSelectionUserData(item_n);
                    auto& res = *g_available[item_id];
                    ImGui::Selectable(res.dirName.c_str(), item_is_selected, ImGuiSelectableFlags_AllowDoubleClick);
                    if (!focused && side == last_focused_size && item_is_selected) focused = &res;
                    if (ImGui::IsItemFocused()) {
                        // FIXME-MULTISELECT: Dual List Box: Transfer focus
                        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
                            request_move_selected = side;
                        if (ImGui::IsMouseDoubleClicked(0)) // FIXME-MULTISELECT: Double-click on multi-selection?
                            request_move_selected = side;
                        last_focused_size = side;
                    }
                    if (side == 1) {
                        if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
                            int n_next = item_n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                            if (n_next >= 0 && n_next < items.Size) {
                                items[item_n] = items[n_next];
                                items[n_next] = item_id;
                                ImGui::ResetMouseDragDelta();
                                _updateOption();
                            }
                        }
                    }
                    if (res.has_info && ImGui::BeginItemTooltip()) {
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
                        if(!res.name.empty()) ImGui::Text("Name: %s", res.name.c_str());
                        if(!res.version.empty()) ImGui::Text("Version: %s", res.version.c_str());
                        if(!res.authorsStr.empty()) ImGui::Text("Authors: %s", res.authorsStr.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }

                if(side == 1) {
                    ImGui::Selectable("Original");
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
                        ImGui::TextUnformatted("Original game resources");
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
                ms_io = ImGui::EndMultiSelect();
                ApplySelectionRequests(ms_io, side);
            }
            ImGui::EndChild();
        }

        // Buttons columns
        ImGui::TableSetColumnIndex(1);
        ImGui::NewLine();
        //ImVec2 button_sz = { ImGui::CalcTextSize(">>").x + ImGui::GetStyle().FramePadding.x * 2.0f, ImGui::GetFrameHeight() + padding.y * 2.0f };
        ImVec2 button_sz = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

        // (Using BeginDisabled()/EndDisabled() works but feels distracting given how it is currently visualized)
        if (ImGui::Button(">>", button_sz))
            request_move_all = 0;
        if (ImGui::Button(">", button_sz))
            request_move_selected = 0;
        if (ImGui::Button("<", button_sz))
            request_move_selected = 1;
        if (ImGui::Button("<<", button_sz))
            request_move_all = 1;

        // Process requests
        if (request_move_all != -1)
            MoveAll(request_move_all, request_move_all ^ 1);
        if (request_move_selected != -1)
            MoveSelected(request_move_selected, request_move_selected ^ 1);

        ImGui::EndTable();

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Selected item info", ImGuiTreeNodeFlags_None)) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, (ImU32) ImColor(50, 50, 50, 200));
            ImGui::BeginChild("Info", {0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
            if(focused) {
                auto& res = *focused;
                ImGui::PushTextWrapPos();
                if(res.has_info) {
                    if(!res.name.empty()) ImGui::Text("Name: %s", res.name.c_str());
                    if(!res.version.empty()) ImGui::Text("Version: %s", res.version.c_str());
                    if(!res.authorsStr.empty()) ImGui::Text("Authors: %s", res.authorsStr.c_str());
                    if(!res.description.empty()) {
                        ImGui::Separator();
                        ImGui::TextUnformatted(res.description.c_str());
                    }
                } else if(res.has_info_file) {
                    ImGui::Text("info file %s/resource.toml is invalid", res.dirName.c_str());
                } else {
                    ImGui::Text("info file %s/resource.toml is not found", res.dirName.c_str());
                }
                ImGui::PopTextWrapPos();
            } else {
                ImGui::TextUnformatted("Nothing is selected");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }
}

bool AltResSection::isChanged() {
    auto& o = *_opt._options[o_altResources.path];
    return o.value != o.opt->value;
}
