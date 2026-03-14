#ifndef NETINET_IN_COMPAT_H
#define NETINET_IN_COMPAT_H

/* compat header for <netinet/in.h> */
/* arm-none-eabi 没有 netinet/in.h, 使用 lwIP 提供的定义 */
#include "lwip/sockets.h"

#endif /* NETINET_IN_COMPAT_H */
