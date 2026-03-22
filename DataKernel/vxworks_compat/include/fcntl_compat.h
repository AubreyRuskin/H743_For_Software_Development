#ifndef FCNTL_COMPAT_H
#define FCNTL_COMPAT_H

/* compat header for <fcntl.h> */

#include <sys/types.h>
// #include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Some embedded toolchains may miss these declarations. */
int open(const char *path, int oflag, ...);
int creat(const char *path, mode_t mode);
int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif




#endif /* FCNTL_COMPAT_H */
