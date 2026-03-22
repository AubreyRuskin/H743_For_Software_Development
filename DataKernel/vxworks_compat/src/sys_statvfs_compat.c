#include "sys_statvfs_compat.h"

#include <errno.h>
#include <string.h>

#include "lfs_port.h"

static int vx_fill_statvfs(struct statvfs *buf)
{
    lfs_t *lfs;
    struct lfs_fsinfo fsinfo;
    lfs_ssize_t used_blocks;
    unsigned long total_blocks;
    unsigned long used_blocks_ul;

    if (buf == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(*buf));

    lfs = lfs_port_fs();
    if (lfs == NULL)
    {
        if (lfs_port_init() < 0)
        {
            errno = EIO;
            return -1;
        }

        lfs = lfs_port_fs();
        if (lfs == NULL)
        {
            errno = EIO;
            return -1;
        }
    }

    if (lfs_fs_stat(lfs, &fsinfo) < 0)
    {
        errno = EIO;
        return -1;
    }

    used_blocks = lfs_fs_size(lfs);
    if (used_blocks < 0)
    {
        errno = EIO;
        return -1;
    }

    total_blocks = (unsigned long)fsinfo.block_count;
    used_blocks_ul = (unsigned long)used_blocks;
    if (used_blocks_ul > total_blocks)
    {
        used_blocks_ul = total_blocks;
    }

    buf->f_bsize = (unsigned long)fsinfo.block_size;
    buf->f_frsize = (unsigned long)fsinfo.block_size;
    buf->f_blocks = total_blocks;
    buf->f_bfree = total_blocks - used_blocks_ul;
    buf->f_bavail = buf->f_bfree;

    /* littlefs has no inode concept, keep inode-related fields as 0. */
    buf->f_files = 0UL;
    buf->f_ffree = 0UL;
    buf->f_favail = 0UL;

    buf->f_fsid = 0UL;
    buf->f_flag = 0UL;
    buf->f_namemax = (unsigned long)fsinfo.name_max;

    return 0;
}

int statvfs(const char *path, struct statvfs *buf)
{
    (void)path;
    return vx_fill_statvfs(buf);
}

int fstatvfs(int fd, struct statvfs *buf)
{
    (void)fd;
    return vx_fill_statvfs(buf);
}
