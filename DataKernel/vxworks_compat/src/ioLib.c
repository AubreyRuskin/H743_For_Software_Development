#include "ioLib.h"
#include <errno.h>

#include "lfs_port.h"

int vxworks_mkdir(const char* path){
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

    rc = lfs_mkdir(lfs, path);
    if (rc == 0)
    {
        return 0;
    }

    switch (rc)
    {
    case LFS_ERR_EXIST:
        errno = EEXIST;
        break;
    case LFS_ERR_NOENT:
        errno = ENOENT;
        break;
    case LFS_ERR_NOTDIR:
        errno = ENOTDIR;
        break;
    case LFS_ERR_NOSPC:
        errno = ENOSPC;
        break;
    case LFS_ERR_NOMEM:
        errno = ENOMEM;
        break;
    default:
        errno = EIO;
        break;
    }

    return -1;
}
