//
// Created by Gonçalo Palaio on 2019-09-12.
//

#ifndef BLOCKS_GP_PLATFORM_H
#define BLOCKS_GP_PLATFORM_H

// @TODO remove define
#define BUILD_ANDROID

#define PLATFORM_LOGI(name) void name(const char* text)
#define PLATFORM_READ_ENTIRE_FILE(name) char* name(const char* file_name, char mode)

#ifdef BUILD_ANDROID

PLATFORM_READ_ENTIRE_FILE(android_read_entire_file);

#define read_entire_file android_read_entire_file

PLATFORM_LOGI(android_logi);

#define logi android_logi
#endif

#ifdef BUILD_MACOSX
#define load_entire_file mac_load_entire_file
#endif

#endif //BLOCKS_GP_PLATFORM_H
