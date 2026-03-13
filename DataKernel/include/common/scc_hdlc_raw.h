/* m8260SccEnd.h - Motorola MPC8260 Serial Communication Controllers (SCC) Ethernet interface header */

/* Copyright 1996-2001 Wind River Systems, Inc. */



#ifndef __INsccHdlcRawh
#define __INsccHdlcRawh

#include "vxworks_type.h"

#ifdef   __cplusplus
extern "C" {
#endif

#define HDLC_FRAME_LEN	512
#define HDLC_HEAD_LEN 3   /*address: 2 bytes; control: 1byte*/

enum CHANNLE_NUM
{
    CHAN_UP =0,
    CHAN_DOWN,
    CHAN_COUNT,
};
enum CHANNLE_CLK
{
    CLK_2MHZ =0,
    CLK_64KHZ
};


enum HDLC_R_STATUS
{
    RECV_TIMEOUT =0,
    R_LEN_ERR =-1,   /*length err*/
    R_FIBERCHN_ERR =-2,  /*fiber channel err,daugter board err*/
    R_CHANNEL_NUMERR =-3,  /*channel number err*/
    R_CRITICAL_ERR =-4
};

enum HDLC_S_STATUS
{
    HDLC_SUCCESS =0,
    S_LEN_ERR,         		/*length err*/
    S_FIBERCHN_ERR,		/*fiber channel err,daugter board err*/
    S_CHANNEL_NUMERR,
    S_CRITICAL_ERR		/*channel number err*/
};

/*hdlc接收通道状态*/
typedef  struct tagHDLC_RECV_STATUS
{
    UINT32  disf_counter;	/* Discarded frame counter, from parameter*/
    UINT32  crc_counter;     /*LG+AB+CR+NO ,from rbd */
    UINT32  dplle_counter; 	/*dpll err, from rbd*/
    UINT32  nmar_counter;  /*Nonmatching address received counter, from parameter*/
    UINT32  ovrun_counter;  /*over run counter, from rbd*/
} HDLC_RECV_STATUS;

/*  Function: enable hdlc send rx error information to application layer
    NOTE:   1. This function must be called before m8260SccHdlcInit (or m8260SccHdlcInit_G)
            2. when receive error packet, the application layer will receive
                a zero lenght packet with NULL buffer pointer.
*/
void    enable_hdlc_rec_rx_err();

/*
hdlc channel init.
Parameters:
	sccNum =3, for CHAN_DOWN;
	sccNum =4, for CHAN_UP.
Return value:0 means OK while -1 means error.
*/
STATUS m8260SccHdlcInit(unsigned char sccNum);

/* hdlc channel init, communication with 603G
Parameters:
	sccNum =3, for CHAN_DOWN;
	sccNum =4, for CHAN_UP.
Return value:0 means OK while -1 means error.
*/
STATUS m8260SccHdlcInit_G(unsigned char sccNum);

/*
stop the scc,then init. the hdlc(call the funtion "STATUS m8260SccHdlcInit(char sccNum)")
Parameters:
	sccNum =3, for CHAN_DOWN;
	sccNum =4, for CHAN_UP.
Return value:0 means OK while -1 means error.

*/
STATUS hdlc_channel_reset(UINT8  sccNum);
/*channel clk master or slave
Parameters:
    channelNum:CHAN_UP or CHAN_DOWN
    clkMstSet: clkMstSet=TRUE, then master

Return value:0 means OK while -1 means error.
*/
STATUS  hdlc_clk_master_set(UINT8 channelNum, BOOL clkMstSet);

/*channel clk select
Parameters:
    channelNum:CHAN_UP or CHAN_DOWN
    clk: CLK_2MHZ or CLK_64KHZ
Return value:0 means OK while -1 means error.
*/
int  hdlc_clk_select(UINT8 channelNum, UINT8 clk);

/* Receive data via hdlc
 * Parameters:
 *      ppucBuf, to save pointer to the received user data.
 *     pTbh,pTbl, recv timebase.
 * Return value:
 *      Bytes of user data received.  Or HDLC_R_STATUS. */
int hdlc_recv_raw(UINT8 channelNum, uint8_t **ppucBuf, UINT32 *pTbh, UINT32 *pTbl,int iTimeout);

/* Send raw data via hdlc.
 * Parameters:
 *      sendData, pointer to user data to be send.This buffer must contain
 *          HDLC_HEAD_LEN bytes space before the user data area.
 *      len, length of user data.  It should<=HDLC_FRAME_LEN and !=0.
 * Return value:
 *      HDLC_S_STATUS */
int hdlc_send_raw(UINT8 channelNum, UINT8 *	, UINT16 len);

/*get the channel recv. status*/
void hdlc_chnstatus_get(UINT8 channelNum, HDLC_RECV_STATUS *channel_status );

/*  Function:   register hdlc receive callback function
    Parameter:
        channelNum：hdlc channel num ，CHAN_UP or CHAN_DOWN
        recvFun,    receive callback function
    return value:   OK, ERROR
    NOTE:   1. parameter of recvFun
            the first is the Hdlc Channel Num, CHAN_UP or CHAN_DOWN
            the second is the pointer to driver receive buffer
            the third is receive datagram length
            Tbh, recv 64 timebase High 4 bytes
            Tbl, recv 64 timebase Low 4 bytes
        2. If the registered function has deal the data buffer,
                it must return OK.
*/
int reg_Hdlc_Recv_Fun(UINT8 channelNum,
                      int (*recvFun)(UINT8, uint8_t *,int, UINT32, UINT32 ));



#ifdef  __cplusplus
}
#endif

#endif  /* __INsccHdlcRawh */


