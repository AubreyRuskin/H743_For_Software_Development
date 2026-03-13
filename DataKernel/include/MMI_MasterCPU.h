/* MMI_MasterCPU.h - This file contains system MMI and Master CPU communicate functions */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02b, 22jun06, hcj add 0x1200.
02a, 30jun06, hcj add modifycase 0x0d40, read files in a directory.
01a, 19dec02, hl first created.
*/

/*
DESCRIPTION
This file contains system MMI and Master CPU communicate functions.
*/

#ifndef MMI_MASTERCPU_H
#define MMI_MASTERCPU_H

#ifdef	__cplusplus
extern "C" {
#endif

/* globals */

extern int EventpipeFD,SOEpipeFD;

/* typedefs */

struct MSG_Event
{
    uint8_t     COT;                    /*事件产生原因      */
    uint8_t     Ftype;                  /*事件类型          */
    uint16_t    Ntag;                   /*报告号            */
    uint8_t     Ftag;                   /*录波号            */
    uint8_t     Hour;                   /*小时              */
    uint8_t     Minute;                 /*分钟              */
    uint8_t     Second;                 /*秒                */
    uint16_t    MSEL;                   /*毫秒              */
    uint16_t    MicroSec;               /*微秒              */
    uint16_t    EventCode;              /*事件区分码        */
    uint8_t     EventArgNum;            /*事件参数个数      */
    uint8_t *   EventArgAddr;           /*事件参数区地址    */
};

struct MSG_SOE
{
    uint8_t     COT;                    /*事件产生原因      */
    uint8_t     Ftype;                  /*事件类型          */
    uint8_t     Hour;                   /*小时              */
    uint8_t     Minute;                 /*分钟              */
    uint8_t     Second;                 /*秒                */
    uint16_t    MSEL;                   /*毫秒              */
    uint16_t    MicroSec;               /*微秒              */
    uint8_t     SN;                     /*遥信量编号        */
    uint8_t     DIQ;                    /*遥信量状态        */
};
typedef  struct
{
    uint8_t aucName[MAX_ID_LEN+1];
    int PreStats;
    int NewStats;
} DI_CH;

/* functions */

/* 该函数提供给用户发送事件消息 */

EP_STATUS Auto_UpLoad_Event(struct MSG_Event *pMSG_Event);
EP_STATUS Auto_UpLoad_SOE(struct MSG_SOE *pMSG_SOE);

#ifdef  __cplusplus
}
#endif

#endif
