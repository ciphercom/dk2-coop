//
// Created by DiaLight on 08.07.2024.
//
#include "dk2/resources/MyResources.h"
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "patches/micro_patches.h"


dk2::MyResources *dk2::MyResources::constructor() {
    this->init_resources();
    this->video_settings.constructor();
    this->playerCfg.constructor();
    this->networkCfg.constructor();
    this->soundCfg.constructor();
    this->gameCfg.constructor();
    this->packetRecord.constructor();
    this->configVersion = 0;
    this->f2FB4 = 0;
    this->f2FB8 = 0;
    this->f2FBC = 0;
    this->f2FC0 = 0;
    this->f2FC4 = 0;
    this->useChecksum = 1;
    memset(this->f2F70, 0, sizeof(this->f2F70));
    return this;
}

void dk2::MyResources::readOrCreate() {
    int try_level = 0;
    int v3_version = 0;
    int status;
    {
        RegKey rkey;
        rkey.constructor();
        if (RegKey_create_Dkii_open_Conf(&rkey) && *rkey.read_Bytes(&status, "Version Number", (LPBYTE) & this->configVersion, 4u) >= 0) {
            v3_version = this->configVersion;
        }
        try_level = -1;
        rkey.close();
    }
    if (v3_version == 11) {
        this->init_Conf_Paths();
        this->video_settings.readOrCreate();
        this->playerCfg.readOrCreate();
        this->networkCfg.readOrCreate();
        this->soundCfg.readOrCreate();
        this->gameCfg.readOrCreate();
    } else {
        this->configVersion = 11;
        {
            RegKey rkey;
            rkey.constructor();
            try_level = 1;
            if (RegKey_create_Dkii_open_Conf(&rkey))
                rkey.write_DWORD(&status, "Version Number", 11u);
            try_level = -1;
            rkey.close();
        }
        this->writeDefaultValues();
        this->video_settings.writeDefaultValues();
        this->playerCfg.writeDefaultValues();
        this->networkCfg.writeDefaultValues();
        this->soundCfg.writeDefaultValues();
        this->gameCfg.writeDefaultValues();
    }

    RegKey rkey;
    rkey.constructor();
    try_level = 2;
    if (*rkey.create_BfProdLtd_key(&status, "Dungeon Keeper II", 0) >= 0) {
        uint32_t value;
        if (*rkey.read_DWORD(&status, "Version Number Major", &value) < 0)
            rkey.write_DWORD(&status, "Version Number Major", g_majorVersion);
        if (*rkey.read_DWORD(&status, "Version Number Minor", &value) < 0)
            rkey.write_DWORD(&status, "Version Number Minor", g_minorVersion);
    }
    try_level = -1;
    rkey.close();
}


namespace {
    void getExePath(char *exeDir) {
        dk2::_strcpy(exeDir, "D:\\DEV\\DK2\\");
        char *CommandLineA = GetCommandLineA();
        char *cmdl = CommandLineA;
        char *str_end;
        if (*CommandLineA == '"') {
            cmdl = CommandLineA + 1;
            str_end = strchr(CommandLineA + 1, '"');
            if (!str_end) return;
        }
        str_end = strchr(CommandLineA + 1, ' ');
        if (!str_end) {
            str_end = &cmdl[strlen(cmdl)];
            if (!str_end) return;
        }
        if (str_end > cmdl) {
            do {
                if (*str_end == '\\')
                    break;
                --str_end;
            } while (str_end > cmdl);
            if (str_end > cmdl)
                str_end[1] = '\0';
        }
        dk2::_strcpy(exeDir, cmdl);
    }
}

dk2::MyResources *dk2::MyResources::init_resources() {
    this->meshesFileMan.constructor();
    int v9 = '\b';
    this->devMeshesFileMan.constructor();
    this->engineTexturesFileMan.constructor();
    this->textureFileMan.constructor();
    this->editorFileMan.constructor();
    this->paletteFileMan.constructor();
    this->spriteFileMan.constructor();
    this->textsFileMan.constructor();
    this->pathsFileMan.constructor();
    this->frontEndFileMan.constructor();
    this->f0 = '\0';
    v9 = (uint8_t) 9;
    char exeDir[MAX_PATH];
    getExePath(exeDir);
    if(patch::use_cwd_as_dk2_home_dir::enabled) {
        GetCurrentDirectoryA(MAX_PATH, exeDir);
        strcat(exeDir, "\\");
        // patch::log::dbg("replace exe dir path2: %s -> %s\n", cmdl, exeDir);
    }
    MyGame_log_printf(&MyGame_instance, "HD Path: %s\n", exeDir);
    _strcpy(this->executableDir, exeDir);
    this->resolveMovies();
    sprintf(this->editorDir, "%sdata\\editor\\", this->executableDir);
    sprintf(this->savesDir, "%sdata\\Save\\", this->executableDir);
    sprintf(this->settingsDir, "%sdata\\Settings\\", this->executableDir);
    sprintf(this->globalDir, "GLOBAL\\");
    sprintf(this->textsDir, "%sdata\\Text\\", this->executableDir);
    sprintf(this->textureCacheDir, "%s\\Dk2TextureCache", this->executableDir);
    sprintf(this->soundSfxDir, "%sdata\\sound\\SFX\\", this->executableDir);
    sprintf(this->soundMusicDir, "%sdata\\sound\\Music\\", this->executableDir);
    int status;
    MyDir_CFileManager_addDirWadf(&status, &this->meshesFileMan, "%sdata\\Meshes.Wad", this->executableDir);
    MyDir_CFileManager_addDirWadf(&status, &this->devMeshesFileMan, "K:\\DK2\\Dev\\Data\\Meshes.Wad", this->executableDir);
    MyDir_CFileManager_addDirWadf(&status, &this->engineTexturesFileMan, "%sdata\\EngineTextures.wad", this->executableDir);
    MyDir_CFileManager_addDirWadf(&status, &this->spriteFileMan, "%sdata\\Sprite.Wad", this->executableDir);
    MyDir_CFileManager_addDirWadf(&status, &this->frontEndFileMan, "%sdata\\FrontEnd.wad", this->executableDir);
    MyDir_CFileManager_addDirWadf(&status, &this->pathsFileMan, "%sdata\\Paths.wad", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->editorFileMan, "%sdata\\editor", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->textsFileMan, "%sdata\\text\\", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->textureFileMan, "%sdata\\Texture", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->paletteFileMan, "%sdata\\palette", this->executableDir);
    return this;
}

int dk2::MyResources::setCdPath(const char *path) {
    strcpy(this->diskDrive, path);
    return sprintf(this->dataMoviesDir, "%sdata\\Movies", this->diskDrive);
}

void dk2::MyResources::useUnpackedDirs() {
    int status;
    MyDir_CFileManager_addDirDiscf(&status, &this->meshesFileMan, "%sdata\\Meshes", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->devMeshesFileMan, "K:\\DK2\\Dev\\Data\\Meshes", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->engineTexturesFileMan, "%sdata\\EngineTextures", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->spriteFileMan, "%sdata\\sprite\\", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->frontEndFileMan, "%sdata\\frontend", this->executableDir);
    MyDir_CFileManager_addDirDiscf(&status, &this->pathsFileMan, "%sdata\\paths", this->executableDir);
}

void dk2::MyResources::destructor() {
    int try_level = 9;
    this->f0 = 0;
    RegKey rkey;
    rkey.constructor();
    try_level = 10;
    int status;
    if ( RegKey_create_Dkii_open_Conf_Paths(&rkey) )
        rkey.write_DWORD(&status, "Version Number", 0);
    try_level = 9;
    rkey.close();
    try_level = 8;
    this->frontEndFileMan.destructor();
    try_level = 7;
    this->pathsFileMan.destructor();
    try_level = 6;
    this->textsFileMan.destructor();
    try_level = 5;
    this->spriteFileMan.destructor();
    try_level = 4;
    this->paletteFileMan.destructor();
    try_level = 3;
    this->editorFileMan.destructor();
    try_level = 2;
    this->textureFileMan.destructor();
    try_level = 1;
    this->engineTexturesFileMan.destructor();
    try_level = 0;
    this->devMeshesFileMan.destructor();
    try_level = -1;
    this->meshesFileMan.destructor();
}

void dk2::MyResources::init_Conf_Paths() {
    RegKey rkey;
    rkey.constructor();
    int try_level = 0;
    RegKey_create_Dkii_open_Conf_Paths(&rkey);
    this->writeDefaultValues();
    try_level = -1;
    rkey.close();
}

void dk2::MyResources::writeDefaultValues() {
    RegKey rkey_root;
    rkey_root.constructor();
    int try_level = 0;
    RegKey rkey_conf;
    rkey_conf.constructor();
    try_level = 1;
    RegKey rkey_confPaths2;
    rkey_confPaths2.constructor();
    try_level = 2;
    this->f0 = 0;
    RegKey rkey_confPaths;
    rkey_confPaths.constructor();
    try_level = 3;
    int status;
    if (RegKey_create_Dkii_open_Conf_Paths(&rkey_confPaths))
        rkey_confPaths.write_DWORD(&status, "Version Number", 0);
    try_level = 2;
    rkey_confPaths.close();
    if (*rkey_root.create_BfProdLtd_key(&status, "Dungeon Keeper II", 1) >= 0) {
        if (*rkey_root.open_key(&status, "Configuration", &rkey_conf) < 0)
            rkey_root.create_key(&status, "Configuration", &rkey_conf);
        if (*rkey_conf.open_key(&status, "Paths", &rkey_confPaths2) < 0)
            rkey_conf.create_key(&status, "Paths", &rkey_confPaths2);
    }
    try_level = 1;
    rkey_confPaths2.close();
    try_level = 0;
    rkey_conf.close();
    try_level = -1;
    rkey_root.close();
}
