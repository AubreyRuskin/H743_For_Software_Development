/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_PublicDataDef.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块所需要的公共定义的相关公共函数实现          */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.11.30             创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>
#include  "logic.h"
#include  "RE_PublicDataDef.h"
#include  "RE_RelayEngine.h"
#include "RE_SuanfaTuyuan.h"

/*************************供测试用的算法元件表信息**************************************/
EP_EXT_ELEM_MAP   *    SuanfaElemMapArrayAddr_g;
uint32_t    nSuanfaElemMapCount_g;
EP_DEBUG_PART_FUNC_TYPE     pSuanfaDebugEntryFunc_g;



/******************保护引擎全局公共变量定义******************/

/* 逻辑图通过EP_Restart_Lgc函数方式重启动的标志 */

BOOL   bLogrpIsRestarted_g;

/*  整个逻辑图扫描属性全局变量 */
LOGRP_ATTRIB_TYPE   LogrpAttrib_g;

/*  所有保护任务创建与否标志 */

BOOL   bAllRelayTaskCreateSuccess_g=FALSE;

/* 保护任务扫描计数器，供看门狗检测用 */
unsigned  long  RE_ulGrpScanTaskScanCounterArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*保存上次启动逻辑图注册过的采样驱动函数个数*/
unsigned  long   ulLastRegisteSamFuncCount_g;

/* 保存上次启动逻辑图注册过的采样函数信息数组 */
SAM_REGISTER_FUNC_INFO_TYPE   LastRegisterSamFuncInfoArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务首次被释放信号量标志  */
BOOL   RE_RelayTaskDriveSemFirstFreeFlagArr_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务首次被释放信号量时的AI计数器值  */
uint32_t   RE_RelayTaskDriveSemFirstFreeAICounterArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务进入硬件测试状态标志数组  */
BOOL   RE_bRelayTaskEnterHwTestModeFlagArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 逻辑图任务定值整定消息计数  */
uint32_t RE_ulRelayTaskSetAtSetCnt_g;

/* 逻辑图任务定值整定图元计数  */
/*uint32_t RE_ulRelayTaskAutoSetCnt_g;
 逻辑图任务是否含有自动整定图元  */
BOOL bulRelayTaskHasAutoSet_g=FALSE;


/******************逻辑图扫描任务的任务序号数组**********************************/
unsigned  long   RE_aulLogrpScanTaskNo_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*    所有作为独立任务运行的逻辑分图当前正在执行扫描的状态标志数组。

      该标志数组中的所有标志只能由保护功能模块修改，但可由采样节拍关联函数访问，但不能修改。

      该标志数组对将所有分图在1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,保护功能模块只修改0号数组成员标志

      若该标志数组某成员对应的某逻辑分图任务正在扫描，则该标志为真，
      否则若该逻辑分图任务还未进行首次次扫描状态时，该标志为假，。
**/

BOOL     RE_abLogrpScaningFlag_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];



/*    所有访问作为独立任务运行的逻辑分图扫描状态标志的互斥二进制信号量数组。

      该信号量数组的每个信号量初始化创建为已满，可用。
      当实时数据任务和1个或多个逻辑图扫描任务访问某个扫描标志时，
      则需获得和释放相应信号量，以保证互斥操作

      该信号量数组对将所有分图作为1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,
      由于只有1个BOOL量，不是结构，所以无须进行互斥操作
      目前暂未使用信号量

***/

SEM_ID    RE_aAccessLogrpScaningFlagSem_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    所有作为独立任务运行的逻辑分图的新1次扫描驱动同步计数器信号量数组。

      该信号量数组的每个信号量初始化创建为空，不可用。
      该信号量数组的每个信号量由采样节拍关联函数释放，由保护功能模块取该信号量

      该信号量数组对将所有分图作为1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,保护功能模块只取走0号数组成员信号量

***/

SEM_ID    RE_aInvokeLogrpNextScanSem_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    逻辑图扫描任务相关信息数组
      数组每个成员是与任务相关的逻辑图扫描任务即时信息

*/
EP_CHART_MSG   RE_aGrpScanTaskMsg[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*   逻辑图扫描任务的当前扫描定值刷新标志数组
     数组每个成员是与任务相关的当前扫描定值需刷新标志
 */

BOOL  RE_aGrpScanDingzhiRefreshFlag[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    所有独立保护分图图元初始化节点连表的数组，
      其本身并不占多少空间，因为其是对指针操作
*/
LIST   RE_aPartGrpInitNodeList[MAX_RELAY_FUNC_COUNT];



/*    所有独立保护分图图元扫描节点连表的数组，
      其本身并不占多少空间，因为其是对指针操作
*/
LIST   RE_aPartGrpScanNodeList[MAX_RELAY_FUNC_COUNT];




/*    所有独立保护分图的属性数组，

*/
PARTGRP_ATTRIB_TYPE   RE_aPartGrpAttribArr[MAX_RELAY_FUNC_COUNT] __attribute__((section(".bss_itcm")));

/* 扫描节点入口函数数组 */
SCAN_UNIT *RE_arrScanUnit[MAX_RELAY_FUNC_COUNT];


/*  每个保护任务的每周期资源消耗统计数据数组 2006-9-21  张云*/
LOGRP_COMSUME_TIME_TYPE  RE_aTaskPeriodTimeArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 每个保护任务的外部命令状态维护  2006-12-21日 张云 */
TASK_OUT_CMD_STS_TYPE   RE_aTaskOutCmdStsArr_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/***********************读取逻辑图顺序化文件时使用的公共函数的实现********************/

/*从当前文件指针处读取ByteCount个保留字节从供装置解析的顺序化文件*/

BOOL    ReadReserveBytesFromResloveSeqFile(FILE  *fp,short  nByteCount)
{

    unsigned   char   acBuf[1024];
    long   nReadBlock;

    if(nByteCount>1024)
    {

        LOG_Dbg_Msg("Read    Reserve   Bytes from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }
    nReadBlock=fread(acBuf,nByteCount,1,fp);/*读取字节*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  Reserve   Bytes from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }
    return  TRUE;
}




/*从当前文件指针处当字符串长度允许大小为1个字节时，读取字符串，*/
/*包括1字节字符串长度*/
/*先读取1字节字符串长度，再读取字符串内容*/

BOOL    ReadStringOfByteLenFromResloveSeqFile
( FILE  *fp,char  *   strRtRead,unsigned  long  *pnRtStrLen)
{
    unsigned   char   TempChar;
    long   nStrLen;
    long   nReadBlock;
    char   *pTailChar;
    nReadBlock=fread(&TempChar,1,1,fp);/*读取字符串长度字节*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read   String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }
    nStrLen=(long)TempChar;/*符号位零扩展  */

    if(nStrLen>MAX_STRLEN_OF_BYTE_AS_READ_FROM_RESLOVE_SEQFILE)
    {
        /*  若字节长度大于最大字节长度,则出错*/
        LOG_Dbg_Msg("Read    String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }
    *pnRtStrLen=nStrLen;/*设置返回长度  */
    if(nStrLen>0)
    {
        /*注意,在tornado2.2下面,读写长度不能==0,  */
        nReadBlock=fread(strRtRead,nStrLen,1,fp);/*读取字符串,*/
        if(nReadBlock!=1)
        {
            /*若读取不成功,则返回错误   */
            LOG_Dbg_Msg("Read   String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }
    }
    /*设置字符串末尾结束符   */
    pTailChar=strRtRead+nStrLen;
    *pTailChar='\0';

    return  TRUE;

}


/*从当前文件指针处当字符串长度允许大小为2个字节时，读取字符串，
  包括2字节字符串长度*/
/*先读取2字节字符串长度，再读取字符串内容*/

BOOL    ReadStringOfWordLenFromResloveSeqFile
(FILE  *fp,char  *   strRtRead,unsigned  long  *pnRtStrLen)
{
    unsigned   char   acBuf[2];
    unsigned   char   *pLenUpByte;

    unsigned  short   nStrLen;
    char   *pTailChar;

    long   nReadBlock;
    pLenUpByte=acBuf+1;
    nReadBlock=fread(pLenUpByte,1,1,fp);/*读取长度短整型低位*/

    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read    String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }

    nReadBlock=fread(acBuf,1,1,fp);/*读取长度短整型高位*/

    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read   String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }
    /*获得长度*/
    nStrLen=U8_TO_U16(acBuf[0], acBuf[1]);


    if(nStrLen>MAX_STRLEN_OF_WORD_AS_READ_FROM_RESLOVE_SEQFILE)
    {
        /*  若字节长度大于最大字节长度,则出错*/
        LOG_Dbg_Msg("Read   String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }
    *pnRtStrLen=nStrLen;/*设置返回长度  */
    if(nStrLen>0)
    {
        nReadBlock=fread(strRtRead,nStrLen,1,fp);/*读取字符串*/
        if(nReadBlock!=1)
        {
            /*若读取不成功,则返回错误   */
            LOG_Dbg_Msg("Read   String from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }
    }
    /*设置字符串末尾结束符   */
    pTailChar=strRtRead+nStrLen;
    *pTailChar='\0';

    return  TRUE;

}


/*从当前文件指针处读取1字节无符号字符型，从文件中*/

BOOL    ReadUnsignedCharFromResloveSeqFile
(FILE  *fp,unsigned  char * pucRtRead)
{

    long   nReadBlock;

    nReadBlock=fread(pucRtRead,1,1,fp);/*读取字节*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read   Char from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }

    return  TRUE;

}



/*从当前文件指针处读取2字节无符号短整型，从文件中，保存时是*/
/*先低字节，再高字节，读取时按PowerPC格式反序组合成short型*/

BOOL   ReadUnsignedShortFromResloveSeqFile
(FILE *fp,unsigned  short  *punRtRead)
{
    unsigned  char   acBuf[2];
    long   nReadBlock;
    unsigned   char  *pUpByte;

    pUpByte=acBuf+1;
    nReadBlock=fread(pUpByte,1,1,fp);/*读取短整型低位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned short  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }

    nReadBlock=fread(acBuf,1,1,fp);/*读取短整型高位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned short  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }
    /*设置返回值    */

    *punRtRead=U8_TO_U16(acBuf[0], acBuf[1]);


    return  TRUE;

}


/*从当前文件指针处读取4字节无符号长整型，到文件中，保存时先低字节，再高字节*/
/* 读取时按PowerPC格式反序组合成long型 */
BOOL   ReadUnsignedLongFromResloveSeqFile
(FILE  *fp,unsigned long  * pulRtRead)
{
    unsigned  char   acBuf[4];
    long   nReadBlock;
    unsigned   char  *pHighestOrder,*pHigherOrder,*pLowerOrder,*pLowestOrder;


    pHighestOrder=acBuf;
    pHigherOrder=acBuf+1;
    pLowerOrder=acBuf+2;
    pLowestOrder=acBuf+3;

    nReadBlock=fread(pLowestOrder,1,1,fp);/*读取长整型最低位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned long  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pLowerOrder,1,1,fp);/*读取长整型次低位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned long  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pHigherOrder,1,1,fp);/*读取长整型次高位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned long  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pHighestOrder,1,1,fp);/*读取长整型最高位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned long  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }

    /*设置返回值    */
    *pulRtRead=U8_TO_U32(acBuf[0], acBuf[1], acBuf[2],acBuf[3] );


    return  TRUE;

}



/*从当前文件指针处按PowerPC格式读取保存为Intel Float格式的单精度数，*/
/*从文件中，占4个字节。按反序组合成PowerPCfloat型*/

BOOL    ReadFloatFromResloveSeqFileInMotorolaType
(FILE  *fp,float * pfRtRead)
{
    unsigned  char   acBuf[4];
    long   nReadBlock;
    unsigned   char  *pHighestOrder,*pHigherOrder,*pLowerOrder,*pLowestOrder;


    pHighestOrder=acBuf;
    pHigherOrder=acBuf+1;
    pLowerOrder=acBuf+2;
    pLowestOrder=acBuf+3;

    nReadBlock=fread(pLowestOrder,1,1,fp);/*读取浮点最低位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned float  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pLowerOrder,1,1,fp);/*读取浮点次低位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned float  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pHigherOrder,1,1,fp);/*读取浮点次高位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned float  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }


    nReadBlock=fread(pHighestOrder,1,1,fp);/*读取浮点最高位*/
    if(nReadBlock!=1)
    {
        /*若读取不成功,则返回错误   */
        LOG_Dbg_Msg("Read  unsigned   float  from Logrp  SeqFile  error!\n",0,0,0,0,0,0);
        return  FALSE;
    }

    /*设置返回值    */
    *pfRtRead=U8_TO_FLT(acBuf[0], acBuf[1],
                        acBuf[2], acBuf[3]);


    return  TRUE;

}






/*****************保护功能模块所用到的公共函数定义********************/




/*初始化图元IO的即时值
   参数  pIO,待操作的IO指针
   返回值,无
*/

void   RE_InitElemIONowValue(EP_ELEM_IO  *pIO)
{
    switch(pIO->ucAttrib)
    {
        case  LOGIC_SIGNAL:  /*逻辑量信号,无单位*/
            pIO->now.bVal=FALSE;
            break;
        case  REAL_FORM_DIANLIU_SIGNAL   :/* 实数格式电流,单位为安，*/
            pIO->now.fVal=0.0;
            break;
        case   COMPLEX_FORM_DIANLIU_SIGNAL  :/* 复数格式电流,单位为安，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case   VALUE_ANGLE_FORM_DIANLIU_SIGNAL:/*幅角式电流,单位为安，，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case   REAL_FORM_DIANLIU_SIGNAL_KA:/*实数格式电流,单位为千安， */
            pIO->now.fVal=0.0;
            break;
        case   COMPLEX_FORM_DIANLIU_SIGNAL_KA:/*// 复数格式电流,单位为千安，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case    VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA:/*// 幅角式电流,单位为千安，，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case REAL_FORM_DIANLIU_SIGNAL_MA:   /* 实数格式电流,单位为毫安，*/
            pIO->now.fVal=0.0;
            break;
        case   REAL_FORM_DIANYA_SIGNAL:/*// 实数格式电压,单位为伏，*/

            pIO->now.fVal=0.0;
            break;
        case    COMPLEX_FORM_DIANYA_SIGNAL:/*// 复数格式电压,单位为伏，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case     VALUE_ANGLE_FORM_DIANYA_SIGNAL:/*// 幅角式电压,单位为伏，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case    REAL_FORM_DIANYA_SIGNAL_KV:/*// 实数格式电压,单位为千伏，*/

            pIO->now.fVal=0.0;
            break;
        case     COMPLEX_FORM_DIANYA_SIGNAL_KV:/*// 复数格式电压,单位为千伏，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case    VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV:/*// 幅角式电压,单位为千伏，*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case    REAL_FORM_ZUKANG_SIGNAL:/*// 实数格式阻抗,单位为欧，*/

            pIO->now.fVal=0.0;
            break;
        case   COMPLEX_FORM_ZUKANG_SIGNAL:/*// 复数格式阻抗,单位为欧*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case   VALUE_ANGLE_FORM_ZUKANG_SIGNAL:/*// 幅角式阻抗,单位为欧*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case    REAL_FORM_ZUKANG_SIGNAL_KO:/*// 实数格式阻抗,单位为千欧，*/

            pIO->now.fVal=0.0;
            break;
        case    COMPLEX_FORM_ZUKANG_SIGNAL_KO:/*// 复数格式阻抗,单位为千欧*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case   VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO:/*// 幅角式阻抗,单位为千欧*/
            pIO->now.xVal=0.0+0.0i;
            break;
        case   SHIJIAN_TYPE1_SIGNAL:/*//时间类型1,单位为秒*/

            pIO->now.fVal=0.0;
            break;
        case   SHIJIAN_TYPE2_SIGNAL:/*//时间类型2,单位为毫秒*/

            pIO->now.fVal=0.0;
            break;
        case    SHIJIAN_TYPE3_SIGNAL:/*//时间类型3,单位为微秒*/

            pIO->now.fVal=0.0;
            break;
        case    SHIJIAN_TYPE4_SIGNAL:/*//时间类型4,单位为小时*/

            pIO->now.fVal=0.0;
            break;
        case    PINLV_SIGNAL:/*//频率,单位为赫兹*/

            pIO->now.fVal=0.0;
            break;
        case   HAUACHA_SIGNAL:/*//滑差,单位为赫兹/秒*/

            pIO->now.fVal=0.0;
            break;
        case   DIANYA_BIANHUALV_SIGNAL:/*//电压变化率,单位为伏/秒*/

            pIO->now.fVal=0.0;
            break;
        case    JIAODU_SIGNAL:/*//角度,单位为度*/

            pIO->now.fVal=0.0;
            break;
        case    WENDU_SIGNAL:/*//温度,单位为摄氏度*/

            pIO->now.fVal=0.0;
            break;
        case    JULI_SIGNAL:/*//距离,单位为千米*/

            pIO->now.fVal=0.0;
            break;
        case    XIANGBIE_SIGAL:/*//故障相别，为枚举类型，无单位，*/
            pIO->now.ulVal=0XFF;     /* 初始化为FF,表示无故障  */
            break;
        case    BILIXISHU_SIGNAL:/*//比例系数,无单位*/

            pIO->now.fVal=0.0;
            break;
        case   CEJUXISHU_SIGNAL:/*//测距系数,单位为千米/欧*/

            pIO->now.fVal=0.0;
            break;
        case    BUCHANGXISHU_SIGNAL:/*//补偿系数,无单位*/

            pIO->now.fVal=0.0;
            break;
        case   GONGLV_TYPE1_SIGNAL:/*//有功功率类型1,单位为瓦*/

            pIO->now.fVal=0.0;
            break;
        case    GONGLV_TYPE2_SIGNAL:/*//有功功率类型2,单位为千瓦*/

            pIO->now.fVal=0.0;
            break;
        case    GONGLV_TYPE3_SIGNAL:/*//有功功率类型3,单位为兆瓦*/

            pIO->now.fVal=0.0;
            break;
        case    GONGLV_TYPE4_SIGNAL:/*//有功功率类型4,单位为千兆瓦*/

            pIO->now.fVal=0.0;
            break;
        case    WUGONG_GONGLV_TYPE1_SIGNAL:/*//无功功率类型1,单位为瓦*/

            pIO->now.fVal=0.0;
            break;
        case    WUGONG_GONGLV_TYPE2_SIGNAL:/*//无功功率类型2,单位为千瓦*/

            pIO->now.fVal=0.0;
            break;
        case    WUGONG_GONGLV_TYPE3_SIGNAL:/*//无功功率类型3,单位为兆瓦*/

            pIO->now.fVal=0.0;
            break;
        case    WUGONG_GONGLV_TYPE4_SIGNAL:/*//无功功率类型4,单位为千兆瓦*/

            pIO->now.fVal=0.0;
            break;
        case    SHORT_INT_SIGNAL:  /*//16位整数*/
            pIO->now.lVal=0;
            break;
        case    LONG_INT_SIGNAL:   /*//32位整数*/
            pIO->now.lVal=0;

            break;
        case   REAL_SIGNAL:       /*//实数*/

            pIO->now.fVal=0.0;
            break;
        case  VAR_STRING:        /* 字符串 2008-7-18日  张云支持字符串*/

            pIO->now.ulVal=0;
            break;
        case   HEX_MODE_WORD_SIGNAL:       /*//32位16进制方式字*/

            pIO->now.ulVal=0;
            break;
        case   CAPACITY_SIGNAL:       /*//容量*/

            pIO->now.fVal=0.0;
            break;
        case DIANDU_TYPE1_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case DIANDU_TYPE2_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case DIANDU_TYPE3_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case DIANDU_TYPE4_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case WUGONG_DIANDU_TYPE1_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case WUGONG_DIANDU_TYPE2_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case WUGONG_DIANDU_TYPE3_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case WUGONG_DIANDU_TYPE4_SIGNAL:
            pIO->now.fVal=0.0;
            break;
        case OHM_PER_METER:
            pIO->now.fVal=0.0;
            break;
        default:
            /* 若非以上类型,则告警   */
            LOG_Dbg_Msg("Tuyuan  ElemIO  Initial  error!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            break;
    }

}

/* 显示逻辑图配置属性 */
void showLogic(void)
{
    int i;
    long nGrpTuyuanCount;
    NODE *pCurScanNode = NULL;
    int m;
    Suanfa_Scan_Node_Type *pTuyuanNode = NULL;
    int nMapTableSize;
    EP_EXT_ELEM_MAP *pMapMember;

    pMapMember = SuanfaElemMapArrayAddr_g;
    nMapTableSize = nSuanfaElemMapCount_g;

    printf("逻辑图全局属性!\n");
    printf("维数(按扫描周期分)%d 任务个数%d\n",
           (int)LogrpAttrib_g.nNodeListArrDims,
           (int)LogrpAttrib_g.nAllowMaxTaskCount);

    /* 图元信息 */
    for (i = 0; i<LogrpAttrib_g.nNodeListArrDims; i++)
    {
        printf("第%d分图图元个数%d\n", i, RE_aPartGrpScanNodeList[i].count);
    }

    printf("算法总数%d\n", nMapTableSize);
    for (i = 0; i<nMapTableSize; i++, pMapMember++)
    {
        printf("算法%d %s %x\n", i, pMapMember->acElemName, (int)pMapMember->Init_Func);
    }

    for (i = 0; i<LogrpAttrib_g.nNodeListArrDims; i++)
    {
        nGrpTuyuanCount = RE_LstCount(RE_aPartGrpScanNodeList+i);
        pCurScanNode = RE_LstFirst(RE_aPartGrpScanNodeList+i);

        for (m = 0; m<nGrpTuyuanCount; m++)
        {
            if (pCurScanNode->ulTuyuanType == RE_SUANFA_SCAN)
            {
                pTuyuanNode = (Suanfa_Scan_Node_Type *)pCurScanNode->pTuyuan;
                printf("%x %d\n",
                       (int)pTuyuanNode->elem.Scan_Func,
                       (int)RE_aPartGrpAttribArr[i].nScanTaskNo);
            }
            pCurScanNode = RE_LstNext(pCurScanNode);
        }

        /* 图元个数 */
        printf("第%d分图图元统计****************************************\n\
               定值图元个数: %d\n开入图元个数: %d\n开入模件图元个数: %d\n开入集图元个数: %d\n实数AI图元个数: %d\n\
               复数AI图元个数: %d\n实数AI集图元个数: %d\n复数AI集图元个数: %d\n事件图元个数: %d\nDO图元个数: %d\n\
               DO集图元个数: %d\n指示灯图元个数: %d\n启动录波图元个数: %d\n停止录波图元个数: %d\n启动停止录波图元个数: %d\n\
               外部端口输入图元个数: %d\n外部端口输出图元个数: %d\n常量图元个数: %d\n恒0图元个数: %d\n恒1图元个数: %d\n\
               光差输出图元个数: %d\n算法图元个数: %d\nand图元个数: %d\nor图元个数: %d\nnot图元个数: %d\ntimer图元个数: %d\n\
               压板图元个数: %d\n控制字图元个数: %d\n大于图元个数: %d\n小于图元个数: %d\n等于图元个数: %d\n\
               多路开关图元个数: %d\n幅角图元个数: %d\n实虚图元个数: %d\n最大图元个数: %d\n最小图元个数: %d\n模式字图元个数: %d\n\
               外部命令图元个数: %d\n系统服务图元个数: %d\n保护启动图元个数: %d\n报告启动图元个数: %d\n",(int)i,\
               RE_aPartGrpAttribArr[i].SetScanNodeNum, RE_aPartGrpAttribArr[i].DIScanNodeNum, RE_aPartGrpAttribArr[i].DIModScanNodeNum,\
               RE_aPartGrpAttribArr[i].DISetScanNodeNum, RE_aPartGrpAttribArr[i].FloatAIScanNodeNum, RE_aPartGrpAttribArr[i].CmplxAIScanNodeNum,\
               RE_aPartGrpAttribArr[i].FloatAISetScanNodeNum, RE_aPartGrpAttribArr[i].CmplxAISetScanNodeNum,\
               RE_aPartGrpAttribArr[i].EventScanNodeNum, RE_aPartGrpAttribArr[i].DOScanNodeNum,\
               RE_aPartGrpAttribArr[i].DOSetScanNodeNum, RE_aPartGrpAttribArr[i].LampScanNodeNum,\
               RE_aPartGrpAttribArr[i].OnlyStartLuboScanNodeNum, RE_aPartGrpAttribArr[i].OnlyStopLuboScanNodeNum,\
               RE_aPartGrpAttribArr[i].StartStopLuboScanNodeNum, RE_aPartGrpAttribArr[i].externImportNum,\
               RE_aPartGrpAttribArr[i].externExportNum,\
               RE_aPartGrpAttribArr[i].constSource, RE_aPartGrpAttribArr[i].constZeroNum,\
               RE_aPartGrpAttribArr[i].constOneNum, RE_aPartGrpAttribArr[i].optAoNum,\
               RE_aPartGrpAttribArr[i].sfNum, RE_aPartGrpAttribArr[i].andNum,\
               RE_aPartGrpAttribArr[i].orNum,RE_aPartGrpAttribArr[i].notNum,\
               RE_aPartGrpAttribArr[i].timerNum, RE_aPartGrpAttribArr[i].ybNum,\
               RE_aPartGrpAttribArr[i].cwNum, RE_aPartGrpAttribArr[i].bNum,\
               RE_aPartGrpAttribArr[i].sNum, RE_aPartGrpAttribArr[i].eNum,\
               RE_aPartGrpAttribArr[i].mwNum, RE_aPartGrpAttribArr[i].apNum,\
               RE_aPartGrpAttribArr[i].riNum, RE_aPartGrpAttribArr[i].maxNum,\
               RE_aPartGrpAttribArr[i].minNum, RE_aPartGrpAttribArr[i].mwdNum,\
               RE_aPartGrpAttribArr[i].owNum, RE_aPartGrpAttribArr[i].soNum, RE_aPartGrpAttribArr[i].rltNum, RE_aPartGrpAttribArr[i].sptNum);
    }
}
