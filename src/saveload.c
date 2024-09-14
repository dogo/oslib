#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psputility.h>

#include "oslib.h"
#include "saveload.h"

SceUtilitySavedataParam savedata;
PspUtilitySavedataListSaveNewData newData;
char key[] = "QTAK319JQKJ952HA";
int saveLoadType = OSL_DIALOG_NONE;

static void oslInitSavedataBase(SceUtilitySavedataParam *savedata) {
    memset(savedata, 0, sizeof(SceUtilitySavedataParam));
    savedata->base.size = sizeof(SceUtilitySavedataParam);

    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &savedata->base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &savedata->base.buttonSwap);

    savedata->base.graphicsThread = 0x11;
    savedata->base.accessThread = 0x13;
    savedata->base.fontThread = 0x12;
    savedata->base.soundThread = 0x10;
}

static void oslSetSaveDataParameters(SceUtilitySavedataParam *savedata, struct oslSaveLoad *saveData) {
    strcpy(savedata->gameName, saveData->gameID);
    strcpy(savedata->saveName, saveData->saveName);
    strcpy(savedata->fileName, "DATA.BIN");

    savedata->dataBuf = malloc(saveData->dataSize);
    if (savedata->dataBuf) {
        memset(savedata->dataBuf, 0, saveData->dataSize);
        memcpy(savedata->dataBuf, saveData->data, saveData->dataSize);
    }
    savedata->dataBufSize = saveData->dataSize;
    savedata->dataSize = saveData->dataSize;

    strcpy(savedata->sfoParam.title, saveData->gameTitle);
    strcpy(savedata->sfoParam.savedataTitle, saveData->savedataTitle);
    strcpy(savedata->sfoParam.detail, saveData->detail);
    savedata->sfoParam.parentalLevel = 1;

    savedata->pic1FileData.buf = saveData->pic1;
    savedata->pic1FileData.bufSize = saveData->size_pic1;
    savedata->pic1FileData.size = saveData->size_pic1;

    savedata->icon0FileData.buf = saveData->icon0;
    savedata->icon0FileData.bufSize = saveData->size_icon0;
    savedata->icon0FileData.size = saveData->size_icon0;
}

static void oslCleanupSaveDataBuffers(SceUtilitySavedataParam *savedata) {
    if (savedata->dataBuf) {
        free(savedata->dataBuf);
        savedata->dataBuf = NULL;
    }
}

static void oslInitSaveDialogType(struct oslSaveLoad *saveData, int mode) {
    oslInitSavedataBase(&savedata);
    oslSetSaveDataParameters(&savedata, saveData);

    savedata.mode = mode;
    savedata.overwrite = 1;
    savedata.focus = PSP_UTILITY_SAVEDATA_FOCUS_LATEST;

#if _PSP_FW_VERSION >= 200
    strncpy(savedata.key, key, sizeof(savedata.key));
#endif

    newData.title = saveData->gameTitle;
    savedata.newData = &newData;

    sceUtilitySavedataInitStart(&savedata);
    saveLoadType = OSL_DIALOG_SAVE;
}

void oslInitSaveDialog(struct oslSaveLoad *saveData) {
    switch (saveData->dialogType) {
        case 0:
            oslInitMultiSaveDialog(saveData);
            break;
        case 1:
            oslInitSingleSaveDialog(saveData);
            break;
        case 2:
            oslInitAutoSaveDialog(saveData);
            break;
        default:
            oslInitMultiSaveDialog(saveData);
            break;
    }
}

void oslInitMultiSaveDialog(struct oslSaveLoad *saveData) {
    oslInitSaveDialogType(saveData, PSP_UTILITY_SAVEDATA_LISTSAVE);
}

void oslInitSingleSaveDialog(struct oslSaveLoad *saveData) {
    oslInitSaveDialogType(saveData, PSP_UTILITY_SAVEDATA_SAVE);
}

void oslInitAutoSaveDialog(struct oslSaveLoad *saveData) {
    oslInitSaveDialogType(saveData, PSP_UTILITY_SAVEDATA_AUTOSAVE);
}

void oslInitLoadDialog(struct oslSaveLoad *loadData) {
    switch (loadData->dialogType) {
        case 0:
            oslInitMultiLoadDialog(loadData);
            break;
        case 1:
            oslInitSingleLoadDialog(loadData);
            break;
        case 2:
            oslInitAutoLoadDialog(loadData);
            break;
        default:
            oslInitMultiLoadDialog(loadData);
            break;
    }
}

static void oslInitLoadDialogType(struct oslSaveLoad *loadData, int mode) {
    oslInitSavedataBase(&savedata);

    strcpy(savedata.gameName, loadData->gameID);
    strcpy(savedata.saveName, loadData->saveName);
    strcpy(savedata.fileName, "DATA.BIN");

    savedata.dataBuf = loadData->data;
    savedata.dataBufSize = loadData->dataSize;
    savedata.dataSize = loadData->dataSize;

    savedata.mode = mode;
    savedata.focus = PSP_UTILITY_SAVEDATA_FOCUS_LATEST;
    savedata.overwrite = 1;

#if _PSP_FW_VERSION >= 200
    strncpy(savedata.key, key, sizeof(savedata.key));
#endif

    sceUtilitySavedataInitStart(&savedata);
    saveLoadType = OSL_DIALOG_LOAD;
}

void oslInitMultiLoadDialog(struct oslSaveLoad *loadData) {
    oslInitLoadDialogType(loadData, PSP_UTILITY_SAVEDATA_LISTLOAD);
}

void oslInitSingleLoadDialog(struct oslSaveLoad *loadData) {
    oslInitLoadDialogType(loadData, PSP_UTILITY_SAVEDATA_LOAD);
}

void oslInitAutoLoadDialog(struct oslSaveLoad *loadData) {
    oslInitLoadDialogType(loadData, PSP_UTILITY_SAVEDATA_AUTOLOAD);
}

void oslInitDeleteDialog(struct oslSaveLoad *deleteData) {
    oslInitSavedataBase(&savedata);

    strcpy(savedata.gameName, deleteData->gameID);
    strcpy(savedata.saveName, deleteData->saveName);
    strcpy(savedata.fileName, "DATA.BIN");

    savedata.dataBuf = deleteData->data;
    savedata.dataBufSize = deleteData->dataSize;
    savedata.dataSize = deleteData->dataSize;

    savedata.mode = PSP_UTILITY_SAVEDATA_LISTDELETE;
    savedata.focus = PSP_UTILITY_SAVEDATA_FOCUS_LATEST;
    savedata.overwrite = 1;

#if _PSP_FW_VERSION >= 200
    strncpy(savedata.key, key, sizeof(savedata.key));
#endif

    sceUtilitySavedataInitStart(&savedata);
    saveLoadType = OSL_DIALOG_DELETE;
}

void oslDrawSaveLoad() {
    switch (sceUtilitySavedataGetStatus()) {
        case PSP_UTILITY_DIALOG_INIT:
        case PSP_UTILITY_DIALOG_VISIBLE:
            sceGuFinish();
            sceGuSync(0, 0);
            sceUtilitySavedataUpdate(1);
            sceGuStart(GU_DIRECT, osl_list);
            oslSetAlpha(OSL_FX_RGBA, 0xff);
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            sceUtilitySavedataShutdownStart();
            break;
        case PSP_UTILITY_DIALOG_FINISHED:
        case PSP_UTILITY_DIALOG_NONE:
            break;
    }
}

int oslGetLoadSaveStatus() {
    return sceUtilitySavedataGetStatus();
}

int oslGetSaveLoadType() {
    return saveLoadType;
}

int oslSaveLoadGetResult() {
    return savedata.base.result;
}

void oslEndSaveLoadDialog() {
    saveLoadType = OSL_DIALOG_NONE;
    oslCleanupSaveDataBuffers(&savedata);
}
