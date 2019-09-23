//
// Created by Gonçalo Palaio on 2019-09-12.
//

#include "gp_android.h"

AAssetManager *asset_manager;

void update_asset_manager(AAssetManager *m) {
    asset_manager = m;
}

void android_log_fmt(const char* fmt, ...) {
    // @TODO remove temporary buffer

    va_list args;
    va_start(args,fmt);
    char buff[500];
    stbsp_vsnprintf(buff, 500, fmt, args);
    va_end(args);
    android_log_str(buff);

}
void android_log_str(const char* str) {
    LOGI("%s", str);
}

char *android_read_entire_file(const char *file_name, char mode) {
    android_log_str("android_read_entire_file");
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
    android_log_str("Listing files");
    while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr) {
        android_log_str(filename);
    }
    AAssetDir_close(assetDir);
}