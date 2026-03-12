#ifndef LFS_PORT_H
#define LFS_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lfs.h"

int lfs_port_init(void);
int lfs_port_deinit(void);
lfs_t *lfs_port_fs(void);

#ifdef __cplusplus
}
#endif

#endif /* LFS_PORT_H */
