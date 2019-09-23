//
// Created by Gonçalo Palaio on 2019-09-12.
//

#ifndef BLOCKS_GP_PLATFORM_H
#define BLOCKS_GP_PLATFORM_H

// @TODO remove define
#define BUILD_ANDROID

#define PLATFORM_LOGI_FMT(name) void name(const char* fmt, ...)
#define PLATFORM_LOGI_STR(name) void name(const char* str)
#define PLATFORM_READ_ENTIRE_FILE(name) char* name(const char* file_name, char mode)

#ifdef BUILD_ANDROID

PLATFORM_READ_ENTIRE_FILE(android_read_entire_file);

#define read_entire_file android_read_entire_file

PLATFORM_LOGI_STR(android_log_str);
PLATFORM_LOGI_FMT(android_log_fmt);

#define log_str android_log_str
#define log_fmt(...) android_log_fmt(__VA_ARGS__)

#endif

#ifdef BUILD_MACOSX
#define load_entire_file mac_load_entire_file
#endif

#endif //BLOCKS_GP_PLATFORM_H
