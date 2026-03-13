#include "scc_hdlc_raw.h"

int hdlc_recv_raw(UINT8 channelNum, uint8_t **ppucBuf, UINT32 *pTbh, UINT32 *pTbl,int iTimeout){
    return 0;
}


int hdlc_send_raw(UINT8 channelNum, UINT8 *, UINT16 len){
    return 0;
}

STATUS  hdlc_clk_master_set(UINT8 channelNum, BOOL clkMstSet){
    return OK;
}

void hdlc_chnstatus_get(UINT8 channelNum, HDLC_RECV_STATUS *channel_status ){
    return ;
}

int  hdlc_clk_select(UINT8 channelNum, UINT8 clk){
    return 0;
}

int reg_Hdlc_Recv_Fun(UINT8 channelNum,
                      int (*recvFun)(UINT8, uint8_t *,int, UINT32, UINT32 )){
                        return 0;
                      }

STATUS hdlc_channel_reset(UINT8  sccNum){
    return OK;
}