//
// Created by DiaLight on 11/22/2025.
//

#include "dk2/resources/DirIter.h"
#include "dk2_functions.h"
#include "dk2_globals.h"
#include "patches/alternative_resources.h"
#include "patches/logging.h"
#include "patches/micro_patches.h"

//#define fs_log(fmt, ...) patch::log::dbg(fmt, __VA_ARGS__)
#define fs_log(fmt, ...)

void __cdecl dk2::fs_setExeDirPath(const char * path) {
    size_t len = strlen(path);
    if (len >= MAX_PATH) return;
    char lastChar = path[len - 1];
    if (lastChar == '\\' || lastChar == '/' ) {
        sprintf(fs_dk2HomeDir, "%s", path);
        return;
    }
    if (len + 1 >= MAX_PATH) return;
    sprintf(fs_dk2HomeDir, "%s\\", path);
}

void dk2::fs_resolveDk2HomeDir() {
    if (patch::use_cwd_as_dk2_home_dir::enabled) {
        char tmp[MAX_PATH];
        DWORD len = GetCurrentDirectoryA(MAX_PATH, tmp);
        strcpy(tmp + len, "\\");
        // patch::log::dbg("replace exe dir path1: %s -> %s", dk2::fs_dk2HomeDir, tmp);
        strcpy(dk2::fs_dk2HomeDir, tmp);
        return;
    }
    _strncpy(g_fs_filePath, GetCommandLineA(), MAX_PATH - 1);
    g_fs_filePath[MAX_PATH - 1] = '\0';
    char firstChar = g_fs_filePath[0];

    char sepChar = ' ';
    if (g_fs_filePath[0] == '"') {
        signed int idx = 0;
        sepChar = '"';
        int len = strlen(g_fs_filePath);
        if (len > 0) {
            do {
                g_fs_filePath[idx] = g_fs_filePath[idx + 1];
                ++idx;
            } while (idx < len);
            firstChar = g_fs_filePath[0];
        }
    }

    char* pos = g_fs_filePath;
    if (firstChar) {
        char curChar = firstChar;
        do {
            if (curChar == sepChar)
                break;
            curChar = *++pos;
        } while (curChar);
    }

    *pos = 0;
    char* sep1Pos = strrchr(g_fs_filePath, '/');
    char* sep2Pos = strrchr(g_fs_filePath, '\\');
    char** pSepPos = &sep2Pos;
    if (sep2Pos <= sep1Pos)
        pSepPos = &sep1Pos;
    char* sepPos = *pSepPos;
    if (sepPos) {
        sepPos[1] = 0;
        fs_setExeDirPath(g_fs_filePath);
    }
}

char *dk2::fs_getExeDir() {
    if (!fs_dk2HomeDir[0]) fs_resolveDk2HomeDir();
    return fs_dk2HomeDir;
}

int *__cdecl dk2::fs_buildExeBasedPath(int *pstatus, char *outPath, int outSize, const char *fileName) {
    // null path
    if (!fileName) {
        strcpy(outPath, fs_getExeDir());
        return *pstatus = 0, pstatus;
    }
//    fs_log("fs_buildExeBasedPath: %s", fileName);
    if(patch::alternative_resources::tryUse(outPath, outSize, fileName)) return *pstatus = 0, pstatus;
    // disk path
    if (fileName[1] == ':') {
        if (fileName[2] == '\\' || fileName[2] == '/') {
            strcpy(outPath, fileName);
            return *pstatus = 0, pstatus;
        }
    }
    // device path
    if (fileName[0] == '\\' || fileName[0] == '/') {
        if (fileName[1] == '\\' || fileName[1] == '/') {
            strcpy(outPath, fileName);
            return *pstatus = 0, pstatus;
        }
    }
    // no drive absolute path
    if (fileName[0] == '\\' || fileName[0] == '/') {
        char drive[4];
        _splitpath(fs_getExeDir(), drive, NULL, NULL, NULL);
        strcpy(outPath, drive);
        strcat(outPath, fileName);
        return *pstatus = 0, pstatus;
    }
    // relative path
    strcpy(outPath, fs_getExeDir());
    strcat(outPath, fileName);
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_createDirectory(int *pstatus, const char *dk2Path) {
    int status;
    fs_buildExeBasedPath(&status, g_fs_directoryPath, MAX_PATH, dk2Path);
    fs_log("fs_createDirectory: %s", g_fs_directoryPath);
    if(!CreateDirectoryA(g_fs_directoryPath, NULL)) return *pstatus = -1, pstatus;
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_removeDirectory(int *pstatus, const char *dk2Path) {
    int status;
    fs_buildExeBasedPath(&status, g_fs_directoryPath, MAX_PATH, dk2Path);
    fs_log("fs_removeDirectory: %s", g_fs_directoryPath);
    if (!RemoveDirectoryA(g_fs_directoryPath)) return *pstatus = -1, pstatus;
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_getFileAttributes(int *pstatus, const char *dk2Path, uint32_t *pattributes) {
    int status;
    CHAR FileName[MAX_PATH];
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_getFileAttributes: %s", FileName);
    DWORD attrib = GetFileAttributesA(FileName);
    if (attrib == INVALID_FILE_ATTRIBUTES) return *pstatus = -1, pstatus;
    *pattributes = attrib;
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_setFileAttributes(int *pstatus, const char *dk2Path, uint32_t dwFileAttributes) {
    CHAR FileName[MAX_PATH];
    int status;
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_setFileAttributes: %s", FileName);
    if (!SetFileAttributesA(FileName, dwFileAttributes)) return *pstatus = -1, pstatus;
    return *pstatus = 0, pstatus;
}

int __cdecl dk2::fs_getFileSize(const char *dk2Path) {
    CHAR FileName[MAX_PATH];
    int status;
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_getFileSize: %s", FileName);

    WIN32_FIND_DATAA FindFileData;
    HANDLE hFindFile = FindFirstFileA(FileName, &FindFileData);
    if (hFindFile == INVALID_HANDLE_VALUE) return -1;
    FindClose(hFindFile);
    return FindFileData.nFileSizeLow;
}

BOOL __cdecl dk2::fs_fileExists(const char *dk2Path) {
    CHAR FileName[MAX_PATH];
    int status;
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
//    fs_log("fs_fileExists: %s", FileName);
    return GetFileAttributesA(FileName) != INVALID_FILE_ATTRIBUTES;
}

bool __cdecl dk2::fs_isDirectory(const char *dk2Path) {
    int status;
    CHAR FileName[MAX_PATH];
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_isDirectory: %s", FileName);
    DWORD attrib = GetFileAttributesA(FileName);
    if (attrib == INVALID_FILE_ATTRIBUTES) return false;
    return attrib & FILE_ATTRIBUTE_DIRECTORY;
}

int *__cdecl dk2::fs_deleteFile(int *pstatus, const char *dk2Path) {
    int status;
    CHAR FileName[MAX_PATH];
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_deleteFile: %s", FileName);
    if (!DeleteFileA(FileName)) return *pstatus = -1, pstatus;
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_readFile(int *pstatus, const char *dk2Path, void *dataBuf, int dataSize, int *outSize) {
    int status;
    char FileName[MAX_PATH];
    fs_buildExeBasedPath(&status, FileName, MAX_PATH, dk2Path);
    fs_log("fs_readFile: %s", FileName);
    int FileSize = fs_getFileSize(FileName);
    int size = FileSize;
    if (FileSize == -1) return *pstatus = -1, pstatus;
    if (FileSize > dataSize) return *pstatus = -1, pstatus;
    MyFile_Disc* diskFile;
    if (*MyFile_Disc_create(&status, &diskFile, FileName, 0x80000001) < 0) return *pstatus = -1, pstatus;
    if (*MyFile_Disc_readBytes(&status, diskFile, dataBuf, size, NULL) < 0) return *pstatus = -1, pstatus;
    MyFile_Disc_delete(&status, diskFile);
    if (outSize) *outSize = size;
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_writeFile(int *pstatus, const char *dk2Path, void *dataBuf, int dataSize) {
    int status;
    fs_buildExeBasedPath(&status, g_fs_filePath, MAX_PATH, dk2Path);
    fs_log("fs_writeFile: %s", g_fs_filePath);
    MyFile_Disc* diskFile;
    if (*MyFile_Disc_create(&status, &diskFile, g_fs_filePath, 0xC0000010) < 0) return *pstatus = -1, pstatus;
    if (*MyFile_Disc_writeBytes(&dataSize, diskFile, dataBuf, dataSize, NULL) < 0) return *pstatus = -1, pstatus;
    MyFile_Disc_delete(&dataSize, diskFile);
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_moveFile(int *pstatus, LPCSTR lpExistingFileName, LPCSTR lpNewFileName) {
    fs_log("fs_moveFile: %s to %s", lpExistingFileName, lpNewFileName);
    if (!MoveFileA(lpExistingFileName, lpNewFileName)) {
        *pstatus = -1;
        return pstatus;
    }
    *pstatus = 0;
    return pstatus;
}

int *__cdecl dk2::fs_DirIter_init(int *pstatus, LPCSTR lpFileName, DirIter *dirIter, int ignoreAttributes) {
    fs_log("fs_DirIter_init: %s", lpFileName);
    HANDLE hFind = FindFirstFileA(lpFileName, &dirIter->findData);
    dirIter->hFind = hFind;
    if (hFind == (HANDLE) -1) return *pstatus = -1, pstatus;
    dirIter->ignoreAttributes = ignoreAttributes;
    strcpy(dirIter->path, lpFileName);
    dirIter->lastSlashPos = strchr(dirIter->path, '\0');
    while (dirIter->lastSlashPos > dirIter->path) {
        char* pos = dirIter->lastSlashPos - 1;
        if (*pos == '\\') break;
        if (*pos == '/') break;
        dirIter->lastSlashPos = pos;
    }
    while (
        (dirIter->findData.dwFileAttributes || dirIter->ignoreAttributes != -1)  // has attr or has ignoreAttr
        && (dirIter->findData.dwFileAttributes & dirIter->ignoreAttributes) == 0
    ) {
        if (!FindNextFileA(dirIter->hFind, &dirIter->findData)) return *pstatus = -1, pstatus;
    }
    strcpy(dirIter->lastSlashPos, dirIter->findData.cFileName);
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_DirIter_next(int *pstatus, DirIter *dirIter) {
    if (!FindNextFileA(dirIter->hFind, &dirIter->findData)) return *pstatus = -1, pstatus;
    while (
        (dirIter->findData.dwFileAttributes || dirIter->ignoreAttributes != -1)
        && (dirIter->findData.dwFileAttributes & dirIter->ignoreAttributes) == 0
    ) {
        if (!FindNextFileA(dirIter->hFind, &dirIter->findData)) return *pstatus = -1, pstatus;
    }
    strcpy(dirIter->lastSlashPos, dirIter->findData.cFileName);
    return *pstatus = 0, pstatus;
}

int *__cdecl dk2::fs_DirIter_destroy(int *pstatus, DirIter *dirIter) {
    FindClose(dirIter->hFind);
    return *pstatus = 0, pstatus;
}

FILE *__cdecl dk2::_fopen(const char *FileName, const char *Mode) {
    {  // patch
        char filePath[MAX_PATH];
        if(patch::alternative_resources::tryUse(filePath, MAX_PATH, FileName)) {
            return dk2::__fsopen(filePath, Mode, 64);
        }
    }
    return dk2::__fsopen(FileName, Mode, 64);
}

int *__cdecl dk2::MyLogger_init(int *pstatus, MyLogger *log, const char *dk2Path, int flags) {
    int status;
    if (*fs_buildExeBasedPath(&status, log->filePath, MAX_PATH, dk2Path) < 0) {
        return *pstatus = -1, pstatus;
    }
    char filePath[MAX_PATH];
    strcpy(filePath, log->filePath);

    // truncate file name
    char *fileName;
    if (
        (fileName = strrchr(filePath, '\\')) == NULL
        && (fileName = strrchr(filePath, '/')) == NULL
        && (fileName = strrchr(filePath, ':')) == NULL
    ) return *pstatus = -1, pstatus;
    *fileName = '\0';

    if (!fs_isDirectory(filePath)) return *pstatus = -1, pstatus;
    log->flags = flags;
    log->initialized = 1;
    log->f12C = 0;
    log->f130 = 0;
    log->prefix[0] = 0;
    return *pstatus = 0, pstatus;
}

