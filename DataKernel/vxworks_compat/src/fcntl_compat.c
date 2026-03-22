#include "fcntl_compat.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

/* Routed to libc/newlib low-level syscall hook in Core/Src/syscalls.c. */
extern int _open(char *path, int flags, ...);
/* lwIP socket fcntl implementation (F_GETFL/F_SETFL/O_NONBLOCK). */
extern int lwip_fcntl(int s, int cmd, int val);

__attribute__((weak)) int open(const char *path, int oflag, ...)
{
    mode_t mode = 0;

    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if ((oflag & O_CREAT) != 0)
    {
        va_list ap;
        va_start(ap, oflag);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }

    return _open((char *)path, oflag, mode);
}

__attribute__((weak)) int creat(const char *path, mode_t mode)
{
    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    return _open((char *)path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}

__attribute__((weak)) int fcntl(int fd, int cmd, ...)
{
    va_list ap;
    int val = 0;

    switch (cmd)
    {
    case F_GETFL:
        return lwip_fcntl(fd, cmd, 0);

    case F_SETFL:
        va_start(ap, cmd);
        val = va_arg(ap, int);
        va_end(ap);
        return lwip_fcntl(fd, cmd, val);

    default:
        errno = ENOSYS;
        return -1;
    }
}
