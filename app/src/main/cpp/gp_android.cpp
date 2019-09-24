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

static int android_file_read(void* cookie, char* buf, int size) {
    android_log_fmt("game_blocks: Calling android_file_read");
    return AAsset_read((AAsset*)cookie, buf, size);
}

static int android_file_write(void* cookie, const char* buf, int size) {
    return 0;
}

static fpos_t android_file_seek(void* cookie, fpos_t offset, int whence) {
    return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_file_close(void* cookie) {
    AAsset_close((AAsset*)cookie);
    return 0;
}

FILE* android_file_open(const char* fname, const char* mode)
{
    android_log_fmt("game_blocks: Calling android_file_open %s", fname);

    if(mode[0] == 'w'){
#undef  fopen
        return fopen(fname,mode);
    }
#define fopen(name, mode) android_fopen(name, mode)
    AAsset* asset = AAssetManager_open(asset_manager, fname, 0);
    if(asset) return funopen(asset, android_file_read, android_file_write, android_file_seek, android_file_close);
#undef  fopen
    return fopen(fname,mode);
}
