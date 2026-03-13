#ifndef __ETH_CALLBACK__
#define __ETH_CALLBACK__
#include <vxWorks.h>  /* 基本的包含头文件 */
#include "assert_compat.h"
#include "stdlib_compat.h"
#include "vxworks_type.h"
#include "iecgoose.h"
#include "GooseInterface.h"


#define ETHERTYPE_VLAN_TYPE_ID 		0x8100

/* Defines and externs for Ethernet Subnetwork (SUBNET_ETHE) only.	*/
#define ETHE_MAC_LEN			6
#define ETHE_LEN_HEAD			(2*ETHE_MAC_LEN + 2)
#define ETHE_LEN_QTAG_PREFIX	4	/* 802.1Q (VLAN) header length	*/

BOOL eth_register_dissector(uint8_t port, uint16_t eth_type, eth_cb_func func);

STATUS FPGAMacAddrSet();

void Poll_Goose();

extern eth_cb_func FPGA_Goose_Recv_Callback_Func[MAX_FPGA_TO_CPU_PORT_NUM];

/* GOOSE状态查询周期 */
extern uint32_t ulGsStsQueryPeriod;	/*daibixiang modify add extern, adc.c中使用 */


STATUS MacAddrGet(uint8_t portNum, uint8_t *addr);

int goose_send_packet(uint8_t portNum, uint8_t *sendBuf, int sendNum);

int gmrp_send_packet(uint8_t portNum, uint8_t *sendBuf, int sendNum);

#endif /* __ETH_CALLBACK__ */
