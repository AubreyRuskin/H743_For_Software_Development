#ifndef SYS_STATVFS_COMPAT_H
#define SYS_STATVFS_COMPAT_H

/*
 * statvfs compat layer for FreeRTOS (arm-none-eabi)
 * arm-none-eabi 没有 <sys/statvfs.h>
 * 此处提供 struct statvfs 和 fstatvfs 的 stub
 * 实际实现需要对接 LittleFS/FatFS 的磁盘信息接口
 */

#include <stdint.h>

struct statvfs {
    unsigned long f_bsize;    /* 文件系统块大小 */
    unsigned long f_frsize;   /* 基本块大小 */
    unsigned long f_blocks;   /* 总块数 */
    unsigned long f_bfree;    /* 空闲块数 */
    unsigned long f_bavail;   /* 非特权用户可用块数 */
    unsigned long f_files;    /* 总 inode 数 */
    unsigned long f_ffree;    /* 空闲 inode 数 */
    unsigned long f_favail;   /* 非特权用户可用 inode 数 */
    unsigned long f_fsid;     /* 文件系统 ID */
    unsigned long f_flag;     /* 挂载标志 */
    unsigned long f_namemax;  /* 最大文件名长度 */
};

/* stub 声明, 需要在 vxworks_compat/src/ 中实现 */
int fstatvfs(int fd, struct statvfs *buf);
int statvfs(const char *path, struct statvfs *buf);

#endif /* SYS_STATVFS_COMPAT_H */
