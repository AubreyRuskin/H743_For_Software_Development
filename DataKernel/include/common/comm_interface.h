#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

/* typedefs */
#include "vxworks_type.h"

typedef struct
{
    EP_ELEM_IO *pelmSrc;
    void *pvAiHnd;
    uint32_t ulScanTaskNo;
    BOOL bIsRiCplx;
    uint8_t ucArith;
    COMPLEX xVal;
    float fVal;
    float fAngVldMod;                   /* Only used when ucArith==2. */
} VI_MEA_AI_DB;

typedef struct
{
    EP_ELEM_IO *pelmSrc;
    void *pvDiHnd;
    BOOL bVal;
    BOOL bSOE;
    uint16_t usQuality; /* 遥信品质位 */
} VI_MEA_DI_DB;

typedef struct
{
    int iRptFileNum;
    uint16_t uiLastRptFile;
    int iSlowDelRptFileNum;
    uint16_t uiLastSlowDelRptFile;
    int iFastDelRptFileNum;
    uint16_t uiLastFastDelRptFile;
}  RPT_NUM_INFO;

typedef   struct
{
    uint16_t uiCurRptNum;
    BOOL bCurRptIsFastDelFlg;		    /* 当前事件报告是快速删除事件报告标志,报告中只要存在一个慢速事件,则就是慢速事件报告 */
    uint8_t ucFstEvtType;  				/* 第1个事件类型 */
    uint16_t uiFstEvtCode;   		/* 第1个事件的区分码 */
    EP_DATE_TIME  tmRptFaultTime;		/* 报告故障发生的时间 */
}   EVT_RPT_NAME_INFO_TYPE;			/* 事件报告名的相关信息 */

typedef struct
{
    enum
    {
        SHORT_PULSE_ACT,
        LONG_PULSE_ACT,
        MEA_DO_RETURN,
        CUSTOM_PULSE_ACT,
    } type;
    uint32_t iMeaDoPointerNum;   	/* 遥控点号 */
    uint8_t ucCmdType;			/* Type of order. */
    BOOL bVal;   		/* 来遥控信号 */
    BOOL bAppSetFlag;			/* 逻辑图设定标志 */
    uint32_t uiPulseTm;   		/* 当类型为CUSTOM_PULSE_ACT自定义脉宽时的脉宽时间 */
    uint32_t ulTqPara;
} VI_MEA_DO_DB;

typedef struct
{
    BOOL bVal;  		/* 来了电度清零信号 */
    BOOL bAdjustRunFlag;				/* 校准正在进行标志 */
    uint32_t uAdjustStartTickNum;
    uint8_t ucObjNum;  		/* 脉冲电度输出通道号0xff表示所有的通道 */
} VI_POCLEAR_INFO;

typedef struct		/* 铁电相关命令处理 */
{
    BOOL bVal;  		/* 命令是否到来 */
    BOOL bAdjustRunFlag;				/* 校准正在切换标志 */
    uint32_t uAdjustStartTickNum;
    uint8_t ucCmdType;  		/* 命令类型 */
} VI_TDSWITCH_INFO;

typedef struct		/* 外部命令图元 */
{
    BOOL bVal;  		/* 外部命令到来 */
    uint32_t ulOrderType;			/* 命令类型 */
} VI_OUTORDER_INFO;
/*来源于view.c*/





#endif                                  /* COMMINTERFACE_H */