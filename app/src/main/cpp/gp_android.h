//
// Created by Gonçalo Palaio on 2019-09-12.
//

#ifndef BLOCKS_GP_ANDROID_H
#define BLOCKS_GP_ANDROID_H

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <cassert>

#define STB_SPRINTF_IMPLEMENTATION

#include "stb_sprintf.h"

FILE* android_file_open(const char* fname, const char* mode);
#define fopen(name, mode) android_file_open(name, mode)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define  LOG_TAG    "game_blocks"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

void android_log_fmt(const char *fmt, ...);

void android_log_str(const char *str);

void update_asset_manager(AAssetManager *m);

#endif //BLOCKS_GP_ANDROID_H
