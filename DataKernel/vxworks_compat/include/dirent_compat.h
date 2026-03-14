#ifndef __DIRENT_COMPAT_H__
#define __DIRENT_COMPAT_H__

/*
 * dirent compat layer for FreeRTOS (arm-none-eabi)
 * arm-none-eabi 的 newlib 不支持 <dirent.h>
 * 此处提供类型和函数声明的 stub, 实际实现需要对接 LittleFS/FatFS
 */

#include <stddef.h>

#define DIRENT_NAME_MAX 256

struct dirent {
    char d_name[DIRENT_NAME_MAX]; /* 文件名 */
};

typedef struct {
    void *fs_handle; /* 底层文件系统句柄, 对接 LittleFS/FatFS 时填充 */
} DIR;

/* POSIX dirent 函数声明 (stub, 需要在 vxworks_compat/src/ 中实现) */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif
