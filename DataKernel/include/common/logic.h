/* logic.h - This file contains declarations to interface with EDP system */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 10nov02, dxh Created first version 0.1.
02a, 29Jan08, zy Merge modify version 1.0.
*/

/*
DESCRIPTION
This file contains declarations to interface with EDP system.
*/

#ifndef LOGIC_H
#define LOGIC_H

/* includes */

#include "edpbase.h"
#include "datetime.h"
#include "realdata.h"
#include "logmsg.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_OUTPUT_NUM  32              /* 最多允许的输出个数 ，用64个输出会导致内存不足DY 4/21/2007 */

#define MAX_REC_RESULT_NUM   4          /* 图元允许保存的最多的结果值数目(包括当前值now)
                                            注意：防止recbuf数组写入溢出*/
#define MAX_REC_TEMP_BUFSIZE   1024          /* 图元不录波的临时共用缓冲大小sdm 11/03/2011*/

/* typedefs */

typedef  union
{
    COMPLEX xVal;
    float fVal;
    uint32_t ulVal;
    int32_t lVal;
    BOOL bVal;
}   VALUE_TYPE;

typedef struct   /* 输入、输出、中间结果的数据结构定义 */
{
    VALUE_TYPE  now;                              	/* 当前值 */
    /* 类型，0=AI通道，1=DI通道，2=定值，3=DI模件,4=测量通道,5=po通道, 0xFF=中间结果 */
    uint8_t ucType;
    uint8_t ucAttrib;                   /* 属性，按照下面表1的定义,DI模件对应的类型为0x6B,32位方式字 */
    void *pvCh;                        /* 通道句柄，对各种通道和字符串时有意义。对字符串时，表示字符串的基址，2008-7-18日张云 */
    VALUE_TYPE *recbuf;	/*20110413 为节省内存，改为动态申请，不需要录波的则都指向同一块数组*/
    /*VALUE_TYPE recbuf[MAX_REC_RESULT_NUM-1];*/   	/* 当逻辑图多点扫描时，保存用于录波的当前采样点以前的结果值buf(为了节省资源，只用保存非BOOL值)，
                                         													 数组成员依次是前1采样点数据，前2采样点数据(当前采样点数据已在now中)等等，
                                         													 注意数组不能越界，若需保存的值比MAX_SAVE_NUM还多，则不用保存 */
} EP_ELEM_IO;

/* ucAttrib        含义                数值类型
 * ----------+-------------------+----------------
 * 0x00         控制字                  uint32_t
 * 0x04         逻辑开关量              BOOL
 * 0x08         实数式电流(A)           float
 * 0x09         复数式电流(A)           COMPLEX
 * 0x0A         辐角式电流(A)           COMPLEX
 * 0x0B         实数式电流(KA)          float
 * 0x0C         复数式电流(KA)          COMPLEX
 * 0x0D         辐角式电流(KA)          COMPLEX
 * 0x10         实数式电流(mA)          float
 * 0x11         复数式电流(mA)          COMPLEX
 * 0x12         辐角式电流(mA)          COMPLEX
 * 0x14         实数式电压(V)           float
 * 0x15         复数式电压(V)           COMPLEX
 * 0x16         辐角式电压(V)           COMPLEX
 * 0x17         实数式电压(KV)          float
 * 0x18         复数式电压(KV)          COMPLEX
 * 0x19         辐角式电压(KV)          COMPLEX
 * 0x1A         实数式阻抗(欧)          float
 * 0x1B         复数式阻抗(欧)          COMPLEX
 * 0x1C         辐角式阻抗(欧)          COMPLEX
 * 0x1D         实数式阻抗(千欧)        float
 * 0x1E         复数式阻抗(千欧)        COMPLEX
 * 0x1F         辐角式阻抗(千欧)        COMPLEX
 * 0x20         时间(秒)                float
 * 0x21         时间(毫秒)              float
 * 0x22         时间(微秒)              float
 * 0x28         频率(HZ)                float
 * 0x2B         滑差(HZ/秒)             float
 * 0x30         电压变化率(V/秒)        float
 * 0x34         角度(度)                float
 * 0x38         温度(摄氏度)            float
 * 0x3B         距离(千米)              float
 * 0x40         故障相别(无单位)        uint32_t
 *              0=AN, 1=BN, 2=CN, 3=AB
 *              4=BC, 5=CA, 6=ABC, 7=ABN
 *              8=BCN，9=CAN, 10=ABCN
 * 0x48         比例系数(无单位)        float
 * 0x4B         测距系数(千米/欧)       float
 * 0x50         补偿系数(无单位)        float
 * 0x54         有功功率(瓦)            float
 * 0x55         有功功率(千瓦)          float
 * 0x56         有功功率(兆瓦)          float
 * 0x57         有功功率(千兆瓦)        float
 * 0x5C         无功功率(乏)          float
 * 0x5D         无功功率(千乏)        float
 * 0x5E         无功功率(兆乏)        float
 * 0x5F         无功功率(千兆乏)      float
 * 0x60         16位整数                int32_t
 * 0x61         32位整数                int32_t
 * 0x64         实数                    float
 * 0x68         字符串                  uint32_t , 字符串长度
 * 0x6B         32位16进制方式字        uint32_t
 * 0x70         容量MVA                 float
 * 0x74         有功电度(度)            float
 * 0x75         有功电度(千度)          float
 * 0x76         有功电度(兆度)          float
 * 0x77         有功电度(千兆度)        float
 * 0x7C         无功电度(千乏时)            float
 * 0x7D         无功电度(兆乏时)          float
 * 0x7E         无功电度(千兆乏时)          float
 * 0x7F         无功电度(兆兆乏时)        float
 * 0x84         欧姆/千米               float
 * 其他         保留 */

/*同杆并架相关信息 2007-4-6日 张云*/
typedef  struct
{
    BOOL   bPoleDataIsValid;          /*同杆并架数据当前有效标志，若有效，为TRUE，若无效，为FALSE  */

} SAME_POLE_DATA_INFO;


/*智能操作箱相关信息  2007-4-6日 张云*/
typedef  struct
{
    BOOL   bHdlDataIsValid;          /*智能操作箱数据当前有效标志，若有效，为TRUE，若无效，为FALSE  */

} HDL_BOX_DATA_INFO;

typedef struct			/* 分图信息的数据结构 */
{

    /**************通用信息*******************/
    uint32_t  ulUserSetAiCnt;           /*保护开发人员自己设置的本任务本次扫描时的有效数据窗AICNT，用于平台的标志集记录
                                          ，保护开发人员必须在本任务的某算法函数中进行重新赋值  2006-11-2日张云*/
    BOOL bExistFatalErrFlag;            /* 存在严重错误标志，若有严重错误，则为真，否则为假，此为一个状态标志， 2006-11-2日 张云 */
    BOOL bHardTestEnterFlag;            /*硬件测试进入标志,若为真,表示此次硬件测试进入,保护退出运行，标志为真，下次进入就返回为假
                                          ，此为一个脉冲标志 2008-1-29日 张云 ，merge*/
    BOOL bHardTestExitFlag;             /*硬件测试退出标志,若为真,表示此次硬件测试退出,保护重新投入运行  */
    BOOL  bRecvNewFgCmdFlag;            /*接收到1个新的复归命令，若为真，表示此次扫描接收到一个新复归命令，下次进入，就返回为假
                                          此为一个脉冲标志  一个新的复归命令到来，变且只变一次。 2006-12-21日 */
    BOOL bSetChg;                       /* 本次扫描定值发生了改变 */
    BOOL bScanIntFlag;                       /* 扫描中断标志，扫描不连续 */
    uint32_t ulScnTime;                 /* 本次开始逻辑图扫描的时刻（us计数器值） */
    uint32_t ulScnInterval;             /* 本次扫描距上次扫描的时间间隔（us） */
    /* 本次开始逻辑图扫描的AI采样计数器值（32位自由运行递增） */
    uint32_t ulScnAiCnt;
    uint16_t unNewAi;                   /* 本次扫描距上次扫描来了多少个采样点 */
    /*当前系统错误状态信息指针ghx20060830*/
    uint16_t unDelayAi;                 /* 本次扫描进入时，数据窗AI采样计数器距此时实际最新的AI采样计数器延迟的采样点数 */

    uint32_t *pulSysErrSts;
    float *pfBase;                      /* 本次扫描AI逻辑0通道数据指针 */
    COMPLEX *pxBase;                    /* 本次扫描AI预处理0通道数据指针 */

    COMPLEX *pxMeaBase;                 /* 本次扫描测量AI的0通道数据指针 */


    /**************光纵通道1的信息**************/
    BOOL   bOptCh1ComValid;             /*光纵通道1通信有效与否，表示最近有接收数据,比如20毫秒以内，但不保证通信稳定和采样同步，适用于距离保护  */
    BOOL   bOptCh1ComStable;						/* 光纵通道1最近接收数据时，通信是否稳定，但不代表采样同步 */
    BOOL   bOptCh1DataIsCredible;						/* 光纵通道1最近接收的数据是否可信，但不代表数据采样同步 2009-3-5 ZY*/
    int    iOptCh1RcvSndDiffChgTime;        /* 光纵通道1的最近接收时，收发时间差变化值，单位US
                                               Value=本次通信稳定时收发时间差-上次通信稳定时收发时间差
                                               若为0，表示没有发生变化，或无法判定
                                               若非0，表示此次判定出来的收发时间差的变化值  2009-2-13 ZY*/
    BOOL   bOptCh1Valid;                /*光纵通道1最近接收的数据采样同步与否，但不代表通信稳定  */
    uint32_t    ulOptCh1ValidAiCnt;     /*光纵通道1最近接收数据，同步到本侧的AICNT
                                           只要通信有效，有最新的接收数据，则此AICNT有效*/

    uint32_t ulOptCh1NewestValidAiCnt; /* 最新有效节拍 */

    uint32_t    ulOptCh1MidResutValidAiCnt; /* 光纵通道1最近接收数据中，快速逻辑图任务中间结果数据同步到本侧的AICNT，
                                             只要通信有效，有最新的接收数据，则此AICNT有效
                                              2006-11-11 张云修改*/
    int32_t     lOptCh1Tsse;            /*光纵通道1最新AICNT和对端采样同步误差，单位us，
　　　　　　　　　　　　　　　　　　　　　若TSSE>0,则表示本侧采样领先，
　　　　　　　　　　　　　　　　　　　　　若TSSE<0,则表示本侧采样落后，  */
    BOOL        bOptCh1NewCalcLostFrmFlag;    /*光纵通道1新到来每秒丢帧统计数据标志，若为真,表示此次有新的统计数据，下次进入就为假
                                               ，此为一个脉冲标志，每一秒钟左右会产生一次脉冲  2006-12-11日张云，只在快速任务中提供*/
    uint32_t    ulOptCh1CalcLostFrmNumPerSec;/*光纵通道1每秒丢帧统计数据，每一秒钟左右会更新一次  */
    uint32_t    ulOptCh1ComTime;             /*光纵通道1的通道通信时间，单位微秒，只有当bOptCh1ComValid为真时，才有意义 2006-12-21   */

    uint32_t ulOptCh1ShowTime;		         /* 通道1界面显示时间 */


    /**************光纵通道2的信息**************/
    BOOL   bOptCh2ComValid;               /*光纵通道2通信有效与否，表示最近有接收数据，比如20毫秒以内，但不保证通信稳定和采样同步，适用于距离保护 */
    BOOL bOptCh2ComStable;													/* 光纵通道2最近接收数据时，通信稳定与否，但不代表采样同步 */
    BOOL   bOptCh2DataIsCredible;						/* 光纵通道2最近接收数据是否可信，但不代表数据采样同步 2009-3-5 ZY*/
    int    iOptCh2RcvSndDiffChgTime;        /* 光纵通道2的最近接收时，收发时间差变化值，单位US
                                               Value=本次通信稳定时真实收发时间差-上次通信稳定时真实收发时间差
                                               若为0，表示没有发生变化，或无法判定
                                               若非0，表示此次判定出来的收发时间差的变化值  2009-2-13 ZY*/
    BOOL   bOptCh2Valid;                 /*光纵通道2最近接收的数据采样同步与否，但不代表通信稳定  */
    uint32_t    ulOptCh2ValidAiCnt;       /*光纵通道2最近接收数据，同步到本侧的AICNT
                                           只要通信有效，有最新的接收数据，则此AICNT有效 */

    uint32_t ulOptCh2NewestValidAiCnt; /* 最新有效节拍 */
    uint32_t    ulOptCh2MidResutValidAiCnt; /* 光纵通道2最近接收数据中，快速逻辑图任务中间结果数据同步到本侧的AICNT，
                                              只要通信有效，有最新的接收数据，则此AICNT有效
                                              2006-11-11 张云修改 */
    int32_t    lOptCh2Tsse;             /*光纵通道2最新AICNT和对端采样同步误差，单位us，
　　　　　　　　　　　　　　　　　　　　  若TSSE>0,则表示本侧采样领先，
　　　　　　　　　　　　　　　　　　　　　若TSSE<0,则表示本侧采样落后，  */
    BOOL        bOptCh2NewCalcLostFrmFlag; /*光纵通道2新到来每秒丢帧统计数据标志，若为真,表示此次有新的统计数据，下次进入就为假
                                               ，此为一个脉冲标志，每一秒钟左右会产生一次脉冲  2006-11-16日张云，只在快速任务中提供*/
    uint32_t    ulOptCh2CalcLostFrmNumPerSec;/*光纵通道2每秒丢帧统计数据，每一秒钟左右会更新一次  */
    uint32_t    ulOptCh2ComTime;             /*光纵通道2的通道通信时间，单位微秒，只有当bOptCh2ComValid为真时，才有意义 2006-12-21  */

    uint32_t ulOptCh2ShowTime;			    /* 通道2界面显示时间 */


    /**************平台出错的信息，暂供低压和励磁使用**************/

    uint64_t ErrorFlag;	/* 系统出错标志*/			/* 32位改为64位DY 7/18/2007 */
    uint32_t EdpState;		/* 平台当前状态，DY 3/20/2007  */
    uint32_t InitErrFlag;		/* 初始化错误代码，DY 3/20/2007 */
    BOOL bErrorRelayContinue;				/* 出错后为TRUE，保护继续运行标志 */
    BOOL bErrorRelayStop;					/* 出错后为TRUE，停止保护运行标志 */
    BOOL ErrorGlobalFlag;		/* 系统出错全局变量 DY 3/20/2007 */
    BOOL bDebugBeginFlag;			/* 调试状态标志 */


    /***********同杆并架数据信息*************/
    SAME_POLE_DATA_INFO      SamePoleInfo;

    /***********智能操作箱数据信息*************/
    HDL_BOX_DATA_INFO        HdlBoxInfo;

    uint32_t ulDiUpdateCnt;    /* DI更新计数 */
    uint32_t ulSetChgCnt; /* 定值更新计数 */

} EP_CHART_MSG;

/* 图元的数据结构，算法元件是其中的一个特例 */
typedef struct tag_EP_ELEMENT
{
    EP_CHART_MSG *pchart;               /* 所属分图信息 */
    uint8_t ucType;                     /* 图元类型，对算法元件而言此值肯定为0 */
    uint8_t ucOutNum;                   /* 总的有效输出个数 */
    uint16_t unInNum;                   /* 总的输入个数 */
    /* 输入量索引数组，各元素为指向输入量的指针 */
    EP_ELEM_IO **ppioIn;
    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO aioOut[MAX_OUTPUT_NUM];
    /* 图元私有数组，可以由算法程序自行解释，保存自己的静态数据 */
    uint32_t aulUser[8];
    /* 图元私有指针，可以由算法程序自行解释。比如这些指针在执行图元自身的
     * 初始化功能时按照需求申请内存，在其中保存自己的数据结构 */
    void *apvUser[2];
    /* 扫描执行函数指针，此函数以指向该图元结构的指针作为参数。
     * 该指针的赋值工作由算法元件自身的初始化函数完成 */
    void (*Scan_Func)(struct tag_EP_ELEMENT *pelm);
} EP_ELEMENT;

/* 算法元件接口表定义 */
typedef struct
{
    /* 字符串标识，应该是PC侧逻辑图软件设定的符合规范的算法元件名称 */
    char acElemName[MAX_ID_LEN+1];
    /* 初始化该算法元件的函数入口，返回值：
     * EP_SUCCESS，成功执行
     * EP_BAD_DATA，输入不匹配
     * EP_BUF_ERR，内存不足 */
    EP_STATUS (*Init_Func)(EP_ELEMENT *pelm);
} EP_EXT_ELEM_MAP;

extern EP_EXT_ELEM_MAP aextmap[];

/* Get external elements number.
 * Parameters:
 *      None.
 * Return value:
 *      Elements number of aextmap array. */
int EP_Ext_Elem_Num(void);

/* 用户开发的算法图元入口函数指针类型定义 */

typedef void (*EP_DEBUG_PART_FUNC_TYPE)(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* LOGIC_H */
