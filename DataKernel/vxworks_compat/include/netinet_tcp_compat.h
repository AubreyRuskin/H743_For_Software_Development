#ifndef NETINET_TCP_COMPAT_H
#define NETINET_TCP_COMPAT_H

/* compat header for <netinet/tcp.h> */
/* arm-none-eabi 没有 netinet/tcp.h, 使用 lwIP 提供的定义 */
#include "lwip/sockets.h"

#endif /* NETINET_TCP_COMPAT_H */
