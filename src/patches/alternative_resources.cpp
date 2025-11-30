//
// Created by DiaLight on 11/27/2025.
//

#include "alternative_resources.h"
#include <ranges>
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "patches/logging.h"
#include "tools/flame_config.h"

#define fs_log(fmt, ...) patch::log::dbg(fmt, __VA_ARGS__)


flame_config::define_flame_option<std::string> o_altResources(
    "flame:alt-resources", flame_config::OG_Config,
    "Alternative resources list of directory names\n"
    "in flame/resources/ path separated by ';' char\n"
    "example: dir2;dir2;dir3",
    "Quuz_AudioFix;Quuz_LevelFixes;Quuz_UniqueBatTex"
);

namespace {

    std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
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

    bool _patchDk2DataPath(const char* relPath, char *outPath, int outSize, const char *dataPack) {
        CHAR FileName[MAX_PATH];
        strcpy(FileName, dk2::fs_getExeDir());
        strcat(FileName, "flame\\resources\\");
        strcat(FileName, dataPack);
        strcat(FileName, "\\");
        strcat(FileName, relPath);
        DWORD attrib = GetFileAttributesA(FileName);
        if (attrib == INVALID_FILE_ATTRIBUTES) return false;
        if (attrib & FILE_ATTRIBUTE_DIRECTORY) return false;  // don't redirect directories
        strcpy(outPath, FileName);
        return true;
    }

    std::vector<std::string> g_resources;

}

void patch::alternative_resources::init() {
    g_resources = split(*o_altResources, ";");
    auto& myres = dk2::MyResources_instance;
    int status;
    for(auto &res : std::views::reverse(g_resources)) {
        CHAR resDir[MAX_PATH];
        strcpy(resDir, dk2::fs_getExeDir());
        strcat(resDir, "flame\\resources\\");
        strcat(resDir, res.c_str());
        strcat(resDir, "\\");

        MyDir_CFileManager_addDirDiscf(&status, &myres.meshesFileMan, "%sdata\\Meshes", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.engineTexturesFileMan, "%sdata\\EngineTextures", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.spriteFileMan, "%sdata\\sprite\\", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.frontEndFileMan, "%sdata\\frontend", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.pathsFileMan, "%sdata\\paths", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.editorFileMan, "%sdata\\editor", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.textsFileMan, "%sdata\\text\\", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.textureFileMan, "%sdata\\Texture", resDir);
        MyDir_CFileManager_addDirDiscf(&status, &myres.paletteFileMan, "%sdata\\palette", resDir);
    }
}

bool patch::alternative_resources::tryUse(char *outPath, int outSize, const char *fileName) {
    const char* relPath = fileName;
    auto rootLen = strlen(dk2::fs_getExeDir());
    if(strncmp(fileName, dk2::fs_getExeDir(), rootLen) == 0) {
        relPath = fileName + rootLen;
    }
    if (!relPath) return false;
    while (*relPath == '\\' || *relPath == '/') relPath++;
    size_t nameLen;
    if (strnicmp(relPath, "Data", 4) == 0) {
        nameLen = 4;
        size_t relLen = strlen(relPath);
        size_t skip = nameLen + 1;
        // dont use save files as alternative resource
        if (relLen >= skip && strnicmp(relPath + skip, "save", 4) == 0) return false;
        // dont use settings files as alternative resource
        if (relLen >= skip && strnicmp(relPath + skip, "settings", 8) == 0) return false;
    } else if (strnicmp(relPath, "DK2TextureCache", 15) == 0) {
        nameLen = 15;
    } else return false;

    for(auto &res : g_resources) {
        if(_patchDk2DataPath(relPath, outPath, outSize, res.c_str())) {
            fs_log("fs_buildExeBasedPath: %s replaced using %s resource", relPath, res.c_str());
            return true;
        }
    }
    return false;
}

