//
// Created by Gonçalo Palaio on 2019-09-12.
//

#include "gp_android.h"

AAssetManager *asset_manager;

void update_asset_manager(AAssetManager *m) {
    asset_manager = m;
}

void android_logi(const char *text) {
    LOGI("%s", text);
}

char *android_read_entire_file(const char *file_name, char mode) {
    android_logi("android_read_entire_file");
    assert(asset_manager);

    AAsset *file = AAssetManager_open(asset_manager, file_name, AASSET_MODE_BUFFER);
    auto fileLength = static_cast<size_t>(AAsset_getLength(file));
    char *fileContent = new char[fileLength + 1];
    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    fileContent[fileLength] = '\0';

    return fileContent;
}

void android_log_files_in_folder(const char *text) {
    AAssetDir *assetDir = AAssetManager_openDir(asset_manager, "");
    const char *filename = (const char *) nullptr;
    android_logi("Listing files");
    while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr) {
        android_logi(filename);
    }
    AAssetDir_close(assetDir);
}