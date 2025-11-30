//
// Created by DiaLight on 11/28/2025.
//

#include <dk2/resources/dir/MyDir_CFileManager.h>
#include "dk2/dk2_memory.h"
#include "dk2/resources/MyFileContent.h"
#include "dk2/resources/dir/MyDir_Disc.h"
#include "dk2/resources/dir/MyDir_Wad.h"
#include "dk2/resources/dir/MyLList_MyDir.h"
#include "dk2/resources/dir/MyLList_MyDir_entry.h"
#include "dk2/resources/file/MyFile.h"
#include "dk2_functions.h"
#include "dk2_globals.h"


dk2::MyDir_CFileManager * dk2::MyDir_CFileManager::constructor() {
    MyDir::constructor();
    this->fileInfo.pFileName = this->fileInfo.str;
    this->fileInfo.str[0] = '\0';
    this->list = NULL;
    *(void **) this = MyDir_CFileManager::vftable;
    return this;
}

void dk2::MyDir_CFileManager::destructor() {
    *(void **) this = MyDir_CFileManager::vftable;
    MyLList_MyDir *f130_list = this->list;
    int try_level = 1;
    if ( f130_list ) {
        MyLList_MyDir_entry *f4_root = f130_list->root;
        MyLList_MyDir_entry *f0_prev = f4_root->prev;
        while ( f0_prev != f4_root ) {
            MyLList_MyDir_entry *toRemove = f0_prev;
            f0_prev = f0_prev->prev;
            MyLList_MyDir_entry *ignore;
            f130_list->remove(&ignore, toRemove);
        }
        dk2::operator_delete(f130_list->root);
        f130_list->root = NULL;
        f130_list->size = 0;
        dk2::operator_delete(f130_list);
        this->list = NULL;
    }
    try_level = -1;
    MyDir::destructor();
}

void dk2::MyDir_CFileManager::clearDirsList() {
    if (!this->list) return;
    auto * root = this->list->root;
    for (auto * cur = root->prev; cur != root;) {
        auto * toDelete = cur;
        cur = cur->prev;
        {
            toDelete->next->prev = toDelete->prev;
            toDelete->prev->next = toDelete->next;
            toDelete->stor.v_scalar_destructor(0);
            dk2::operator_delete(toDelete);
            --this->list->size;
        }
    }
    dk2::operator_delete(this->list->root);
    this->list->root = NULL;
    this->list->size = 0;
    dk2::operator_delete(this->list);
    this->list = NULL;
}

int *dk2::MyDir_CFileManager::openFile(int *pstatus, MyFile *a3_file, const char *a4_filename) {
    if (!this->list) return *pstatus = -1, pstatus;
    int status;
    for (auto* cur = this->list->root->prev; cur != this->list->root; cur = cur->prev) {
        // default constructor
        MyFileInfo emptyInfo;
        emptyInfo.pFileName = emptyInfo.str;
        emptyInfo.str[0] = '\0';

        // copy assign operator
        MyFileInfo* pInfo = &this->fileInfo;
        *pInfo = emptyInfo;
        pInfo->pFileName = pInfo->str;

        if (*cur->stor.v_getFirstFile(&status, a4_filename, pInfo) < 0) continue;
        cur->stor.v_openInputStream(pstatus, a3_file, a4_filename, 0x80000001, 0);
        return pstatus;
    }
    return *pstatus = -1, pstatus;
}

char *dk2::MyDir_CFileManager::getPath() {
    if ( !this->list )
        return NULL;
    if (!this->list->size)
        return NULL;
    return this->list->root->prev->stor.v_getPath();
}

int *dk2::MyDir_CFileManager::readFile(int *pstatus, MyFileContent *fileData, const char *a4_filename) {
    if (!this->list) return *pstatus = -1, pstatus;

    char file_buf[sizeof(MyFile)];
    MyFile& file = *(MyFile *) file_buf;
    file.constructor_empty();

    int try_level = 0;
    int status;
    if (*this->openFile(&status, &file, a4_filename) < 0) {
        try_level = -1;
        file.destructor();
        return *pstatus = status, pstatus;
    }
    DWORD size = this->fileInfo.fileSizeLow;
    if (size > fileData->capacity) {
        dk2::operator_delete(fileData->buf);
        fileData->buf = NULL;
        fileData->capacity = 0;
        fileData->size = 0;
        fileData->buf = dk2::operator_new(size);
        if (fileData->buf != NULL) {
            fileData->capacity = size;
            fileData->size = size;
        }
    } else {
        fileData->size = size;
    }
    if (file.readBytes(fileData->buf, size) != size) {
        status = -1;
    } else {
        status = 0;
    }
    try_level = -1;
    file.destructor();
    return *pstatus = status, pstatus;
}

int *dk2::MyDir_CFileManager::addDir(int *pstatus, MyDir *dir) {
    if ( !this->list ) {
        auto *newList = (MyLList_MyDir *)dk2::operator_new(sizeof(MyLList_MyDir));
        if (newList) {
            newList->f0 = (char)pstatus;
            auto * root = (MyLList_MyDir_entry *) dk2::operator_new(sizeof(MyLList_MyDir_entry));
            root->prev = root;
            root->next = root;
            newList->root = root;
            newList->size = 0;
        } else {
            newList = NULL;
        }
        this->list = newList;
    }
    if (!this->list) return *pstatus = -1, pstatus;

    auto *root = this->list->root;
    auto *newItem = (MyLList_MyDir_entry *) dk2::operator_new(sizeof(MyLList_MyDir_entry));

    // patched simplified
    {
        auto *last = root->prev;
        newItem->prev = last;
        last->next = newItem;
    }
    {
        newItem->next = root;
        root->prev = newItem;
    }
    // patched end

    if (newItem != (MyLList_MyDir_entry *) -8)  // wtf?
        newItem->stor.copy_constructor(dir);
    ++this->list->size;
    return *pstatus = 0, pstatus;
}

char *dk2::MyDir_CFileManager::findFile(const char *filename) {
    if (!this->list) return NULL;
    for (auto *cur = this->list->root->prev; cur != this->list->root; cur = cur->prev) {
        // default constructor
        MyFileInfo emptyInfo;
        emptyInfo.pFileName = emptyInfo.str;
        emptyInfo.str[0] = '\0';

        // copy assign operator
        MyFileInfo *pInfo = &this->fileInfo;
        *pInfo = emptyInfo;
        pInfo->pFileName = pInfo->str;

        int status;
        if (*cur->stor.v_getFirstFile(&status, filename, pInfo) < 0) continue;
        return this->fileInfo.str;
    }
    return NULL;
}

int * __cdecl dk2::MyDir_CFileManager_addDirDiscf(int *pstatus, MyDir_CFileManager *fileMan, const char *Format, ...) {
    va_list ArgList;
    va_start(ArgList, Format);

    char dir_buf[sizeof(MyDir)];
    MyDir& dir = *(MyDir *) dir_buf;
    dir.constructor();
    *(void **) &dir = MyDir_Disc::vftable;

    int try_level = 0;
    vsprintf(g_CFileManager_formattedString, Format, ArgList);
    int status;
    if ( *((MyDir_Disc *)&dir)->openDirectory(&status, g_CFileManager_formattedString) >= 0 ) {
        fileMan->addDir(&status, &dir);
        *pstatus = 0;
    } else {
        *pstatus = status;
    }
    try_level = -1;
    dir.destructor();
    return pstatus;
}

int *dk2::MyDir_CFileManager_addDirWadf(int *pstatus, MyDir_CFileManager *fileMan, const char *Format, ...) {
    va_list ArgList;
    va_start(ArgList, Format);

    char dir_buf[sizeof(MyDir)];
    MyDir& dir = *(MyDir *) dir_buf;
    dir.constructor();
    *(void **) &dir = MyDir_Wad::vftable;

    int try_level = 0;
    vsprintf(g_CFileManager_formattedString, Format, ArgList);
    OutputDebugStringA(g_CFileManager_formattedString);
    int status;
    if ( *((MyDir_Wad *) &dir)->readAndParse(&status, g_CFileManager_formattedString, 0) >= 0 ) {
        fileMan->addDir(&status, &dir);
        *pstatus = 0;
    } else {
        *pstatus = status;
    }
    try_level = -1;
    dir.destructor();
    return pstatus;
}

