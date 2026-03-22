#include <dirent_compat.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "lfs_port.h"

typedef struct
{
    lfs_t *lfs;
    lfs_dir_t dir;
    struct dirent entry;
    int is_open;
} vx_lfs_dir_ctx_t;

static void vx_set_errno_from_lfs(int lfs_err)
{
    switch (lfs_err)
    {
    case LFS_ERR_NOENT:
        errno = ENOENT;
        break;
    case LFS_ERR_NOTDIR:
        errno = ENOTDIR;
        break;
    case LFS_ERR_EXIST:
        errno = EEXIST;
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
}

DIR *opendir(const char *name)
{
    DIR *dirp;
    vx_lfs_dir_ctx_t *ctx;
    lfs_t *lfs;
    int rc;

    if (name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    lfs = lfs_port_fs();
    if (lfs == NULL)
    {
        rc = lfs_port_init();
        if (rc < 0)
        {
            vx_set_errno_from_lfs(rc);
            return NULL;
        }
        lfs = lfs_port_fs();
        if (lfs == NULL)
        {
            errno = EIO;
            return NULL;
        }
    }

    dirp = (DIR *)malloc(sizeof(DIR));
    ctx = (vx_lfs_dir_ctx_t *)malloc(sizeof(vx_lfs_dir_ctx_t));
    if ((dirp == NULL) || (ctx == NULL))
    {
        free(ctx);
        free(dirp);
        errno = ENOMEM;
        return NULL;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->lfs = lfs;
    rc = lfs_dir_open(ctx->lfs, &ctx->dir, name);
    if (rc < 0)
    {
        free(ctx);
        free(dirp);
        vx_set_errno_from_lfs(rc);
        return NULL;
    }

    ctx->is_open = 1;
    dirp->fs_handle = ctx;
    return dirp;
}

struct dirent *readdir(DIR *dirp)
{
    vx_lfs_dir_ctx_t *ctx;
    struct lfs_info info;
    int rc;

    if ((dirp == NULL) || (dirp->fs_handle == NULL))
    {
        errno = EBADF;
        return NULL;
    }

    ctx = (vx_lfs_dir_ctx_t *)dirp->fs_handle;
    if (!ctx->is_open)
    {
        errno = EBADF;
        return NULL;
    }

    rc = lfs_dir_read(ctx->lfs, &ctx->dir, &info);
    if (rc < 0)
    {
        vx_set_errno_from_lfs(rc);
        return NULL;
    }

    if (rc == 0)
    {
        return NULL;
    }

    strncpy(ctx->entry.d_name, info.name, DIRENT_NAME_MAX - 1);
    ctx->entry.d_name[DIRENT_NAME_MAX - 1] = '\0';
    return &ctx->entry;
}

int closedir(DIR *dirp)
{
    vx_lfs_dir_ctx_t *ctx;
    int rc;

    if ((dirp == NULL) || (dirp->fs_handle == NULL))
    {
        errno = EBADF;
        return -1;
    }

    ctx = (vx_lfs_dir_ctx_t *)dirp->fs_handle;
    dirp->fs_handle = NULL;

    rc = 0;
    if (ctx->is_open)
    {
        rc = lfs_dir_close(ctx->lfs, &ctx->dir);
        if (rc < 0)
        {
            vx_set_errno_from_lfs(rc);
        }
    }

    free(ctx);
    free(dirp);

    return (rc < 0) ? -1 : 0;
}
