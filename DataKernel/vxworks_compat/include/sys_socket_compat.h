#ifndef SYS_SOCKET_COMPAT_H
#define SYS_SOCKET_COMPAT_H

/* compat header for <sys/socket.h> */
/* arm-none-eabi 没有 BSD socket, 使用 lwIP 的 socket 兼容层 */
#include "lwip/sockets.h"

#endif /* SYS_SOCKET_COMPAT_H */
