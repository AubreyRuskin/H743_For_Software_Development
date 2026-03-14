#ifndef ARPA_INET_COMPAT_H
#define ARPA_INET_COMPAT_H

/* compat header for <arpa/inet.h> */
/* arm-none-eabi 没有 arpa/inet.h, 使用 lwIP 提供的定义 */
#include "lwip/sockets.h"
#include "lwip/inet.h"

#endif /* ARPA_INET_COMPAT_H */
