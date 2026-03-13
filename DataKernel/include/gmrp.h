#ifndef	__GMRP__
#define __GMRP__
#include <stdio_compat.h>
// #include "types.h"
#include "vxworks_type.h"


/* 以太网发送函数 */
typedef int (*ETH_TX_FUN)(uint8_t port, uint8_t *buf, int size);

/* 获取Mac地址接口 */
typedef int (*ETH_GET_MAC_FUN)(uint8_t port, uint8_t *addr);

/* GMRP初始化 */
void gmrp_initialize(ETH_TX_FUN eth_tx_fun, ETH_GET_MAC_FUN get_mac_fun);

/* 加入组播地址 */
BOOL gmrp_addmulti(uint8_t port_no, uint8_t *m_addr);
BOOL gmrp_delmulti(uint8_t port_no, uint8_t *m_addr);

/* 组播发送函数，需定时调用 */
void gmrp_join_timer();

/* 显示已加入的组播组信息 */
void gmrp_show();

#endif /* __S_GMRP__ */
