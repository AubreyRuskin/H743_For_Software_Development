#include "unistd_compat.h"

#include <errno.h>

#include "lfs_port.h"

int rmdir(const char *path)
{
    lfs_t *lfs;
    int rc;

    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    lfs = lfs_port_fs();
    if (lfs == NULL)
    {
        rc = lfs_port_init();
        if (rc < 0)
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

    rc = lfs_remove(lfs, path);
    if (rc == 0)
    {
        return 0;
    }

    switch (rc)
    {
    case LFS_ERR_NOENT:
        errno = ENOENT;
        break;
    case LFS_ERR_NOTDIR:
        errno = ENOTDIR;
        break;
    case LFS_ERR_NOTEMPTY:
        errno = ENOTEMPTY;
        break;
    case LFS_ERR_ISDIR:
        errno = EISDIR;
        break;
    case LFS_ERR_NOMEM:
        errno = ENOMEM;
        break;
    case LFS_ERR_IO:
        errno = EIO;
        break;
    default:
        errno = EIO;
        break;
    }

    return -1;
}

int fsync(int fd)
{
    (void)fd;

    /* TODO: currently a compatibility no-op.
     * If required later, route fd to a real backend sync implementation.
     */
    return 0;
}
