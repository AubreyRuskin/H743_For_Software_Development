#ifndef FCNTL_COMPAT_H
#define FCNTL_COMPAT_H

/* compat header for <fcntl.h> */

#include <sys/types.h>
#include <fcntl.h>

#ifndef O_RDONLY
#define O_RDONLY 0x0000
#endif

#ifndef O_WRONLY
#define O_WRONLY 0x0001
#endif

#ifndef O_RDWR
#define O_RDWR 0x0002
#endif

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
