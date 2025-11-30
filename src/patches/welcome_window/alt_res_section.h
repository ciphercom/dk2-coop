//
// Created by DiaLight on 11/29/2025.
//

#ifndef FLAME_ALT_RES_SECTION_H
#define FLAME_ALT_RES_SECTION_H


#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "imgui.h"
#include "tools/flame_config.h"

extern flame_config::define_flame_option<std::string> o_altResources;

class OptionsSection;

struct AltResource {
    ImGuiID id;
    std::string dirName;
    bool has_info_file = false;
    bool has_info = false;
    std::string name;
    std::string version;
    std::vector<std::string> authors;
    std::string authorsStr;
    std::string description;
};

struct AltResSection {
    OptionsSection &_opt;
    std::vector<std::unique_ptr<AltResource>> g_available;

    AltResSection(OptionsSection &opt);

    ImVector<ImGuiID>           _Items[2];       // ID is index into ExampleName[]
    ImGuiSelectionBasicStorage  _Selections[2];  // Store ExampleItemId into selection

    void updateAvailable();
    void updateItems();
    void _updateOption();

    void MoveAll(int src, int dst) {
        IM_ASSERT((src == 0 && dst == 1) || (src == 1 && dst == 0));
        for (ImGuiID item_id : _Items[src])
            _Items[dst].push_back(item_id);
        _Items[src].clear();
        if(dst == 0) SortItems(dst);
        _Selections[src].Swap(_Selections[dst]);
        _Selections[src].Clear();
        _updateOption();
    }

    void MoveSelected(int src, int dst) {
        for (int src_n = 0; src_n < _Items[src].Size; src_n++) {
            ImGuiID item_id = _Items[src][src_n];
            if (!_Selections[src].Contains(item_id))
                continue;
            _Items[src].erase(&_Items[src][src_n]);
            _Items[dst].push_back(item_id);
            src_n--;
        }
        if(dst == 0) SortItems(dst);
        _Selections[src].Swap(_Selections[dst]);
        _Selections[src].Clear();
        _updateOption();
    }

    void ApplySelectionRequests(ImGuiMultiSelectIO* ms_io, int side) {
        // In this example we store item id in selection (instead of item index)
        _Selections[side].UserData = _Items[side].Data;
        _Selections[side].AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self, int idx) { ImGuiID* items = (ImGuiID*)self->UserData; return items[idx]; };
        _Selections[side].ApplyRequests(ms_io);
    }

    void SortItems(int n);

    void render();

    bool isChanged();

};


#endif // FLAME_ALT_RES_SECTION_H
