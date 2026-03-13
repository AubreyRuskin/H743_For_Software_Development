/* rec.c - This file contains program to record realtime data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01d, 6aug04, zy 注释修改.
01c, 4nov03, hdx Updated to version 1.0.
01b, 29jul03, hdx Verified version 0.1.
01a, 5apr03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains program to record realtime data.
INCLUDES: rec.h
说明:
	录波的主要操作过程：
	1、首先对原始通道录波量，是每次记录在每个通道实时数据缓冲中，
			对中间结果，是每次记录在db中的pxbuf中
	2、然后隔一段时间，将所有录波通道保存在通道缓冲中的录波数据倒到
			当前录波块中。
	3、然后将已满的录波块中的录波数据写到录波文件中。

	标志的主要操作过程：
	1、对每个任务，每次处理时，若有变位，将所有通道标志量和
			任务相关中间结果标志量记录到  该任务RC_TSK_FLAG的
	       ppfgblk缓冲中，每次标志记录算一个标志块 。
*/

/* includes */

#include "rec.h"
#include "realdata.h"
#include "miscfunc.h"
#include "filetool.h"
#include "view.h"
#include "sysinfo.h"
#include "RE_RelayEngine.h"
#include "swcfg.h"
#include <stdio_compat.h>
#include "string_compat.h"
#include <taskLib.h>
#include <msgQLib.h>
#include <ioLib.h>
#include <dirent_compat.h>
#include "FileSynPro.h"
#include "sys_statvfs_compat.h"

#if defined(EDP_01_02_BUILD)
#include "spiio.h"
#endif

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif

#include "bspinterface.h"
#include "OPT_Data.h"

/* defines */

#ifdef EXCITE_BUILD
#define MAX_REC_MS      30000L         /* 30 second.励磁录波段最大允许录波时间 30 second,但是要求录波频率放大（50） */
#else
#ifdef EDP02_PSR_BUILD
#define MAX_REC_MS      0L
#else
#define MAX_REC_MS      4000L          /* 5 second.录波段最大允许录波时间 4 second */
#endif
#endif

#define MAX_REC_MS_LONG 3600000L /* 最大允许录波时间 60min,但是要求录波频率放大(1) */
#define MAX_FORWARD_REC_MS_LONG 5000L  /* 前向录波最长5s */

#define MAX_REC_OVERLAP_LONG 1000L /* 前一录波和本次录波的最大重叠时间1s */

#ifdef EDP03_INTELBOX_BUILD			/* 智能操作箱使用小的录波缓冲 */
#define REC_MEM_BUF     0x80000L       /* 14M bytes.录波的最大缓冲大小2M */
#elif defined(EDP03_LOWPROTECT_BUILD) /* || defined(EDP03_STABCONTROL_BUILD) */		/* 低压分配1.5M */
#define REC_MEM_BUF     0x100000L       /* 1M bytes.录波的最大缓冲大小1M */		/* 原来为1.5M 10/17/2007 */
#elif defined(EDP03_STABCONTROL_BUILD)	/* 基于EDP03平台的智能操作箱 */
#define REC_MEM_BUF     0x200000L       /* 14M bytes.录波的最大缓冲大小2M */
#else		/* 其它平台 */
#ifdef EDP02_PSR_BUILD
#define REC_MEM_BUF      0L
#else
#define REC_MEM_BUF     0x600000L       /* 4M bytes.录波的最大缓冲大小6M */
#endif
#endif

#define SMALL_REC_MEM_BUF 0x200000L /* 较小内存分配 */

/* 内存不是问题 */
#define FLAG_MEM_BUF    0x100000L       /* 标志集最大缓冲大小1M */

#define MAX_FLAG_NUM    0X8000L         /* 32K,标志集缓冲最大允许记录个数 */
#define MAX_LUBO_SAMPLE_MS   20L           /* 即刻录波采样长度 20毫秒,1个周波 */


/*2011-12-28  ZY修改.以前这里对长录波模式有漏洞,当新录波很长,而磁盘空间已经不够时,老录波删除不掉,没办法记录*/

#define MIN_ALLOW_DISK_FREE_SPACE    1000000L    /* 录波时允许的最小空闲磁盘空间  1M */
#define MIN_ALLOW_DISK_WRITE_SPACE   500000L     /* 对DATA DISK 写操作时,最小空闲磁盘空间 500K */
#define MIN_ALLOW_DATA_DISK_SPACE    2000000L				/* 数据盘剩余空间，改为2M */

/* 其它 */
#define MIN_ALLOW_DISK_FREE_SPACE_OTHER 30000000L    /* 录波时允许的最小空闲磁盘空间,30M */
#define MIN_ALLOW_DISK_WRITE_SPACE_OTHER 500000L     /* 对DATA DISK 写操作时,最小空闲磁盘空间,500K */
#define MIN_ALLOW_DATA_DISK_SPACE_OTHER 30000000L				/* 数据盘剩余空间，改为30M */

#define REC_BLK_BUF_CYC     5     /* 每个录波块的记录周期数  2006-7-11 张云 */

/* 最大消息队列条数, 尽量保证消息不丢失,
 * 如果丢失了可能造成录波文件不能正常生成
 */
#define MAX_REC_MSG_QUE 1024

/* 为线路保护定制的开入录波量 */
#define LINE_CUST_DI_REC1 "远跳开入"
#define LINE_CUST_DI_REC2 "远传开入A"
#define LINE_CUST_DI_REC3 "远传开入B"

/* typedefs */

/* Infomation of every REC_AI. 每个AI录波的相关访问信息 */
typedef struct
{
    EP_ELEM_IO *pelmSrc;                /* Data source(if it's middle result). 中间结果的句柄 */
    void *pvHisHnd;                     /* Point to RD_AI_HND or RC_DB_INFO. 外部通道句柄或中间结果开始的任务相关信息句柄
                                         每个录波AI都有该指针，对原始通道和中间结果都有效*/
    BOOL bIsReal;                       /* Used when REC_AI is input channel. 外部通道是否是实通道*/
    int iChOfst;                        /* Offset of REC_AI in input channels. 外部通道相对通道基址（实或复）的句柄 */
    COMPLEX *pxBuf;                     /* Data buffer(if it's middle result). 中间结果的数据缓冲指针（实复合用），
                                          				对原始AI通道，是由实时数据缓冲保存的。
                                         					最大容量由对应的dbinfo的ibufLen决定*/
} RC_AI_DB;

/* Infomation of every REC_DI. 每个DI录波的相关访问信息*/
typedef struct
{
    EP_ELEM_IO *pelmSrc;                /* Data source(if it's middle result). 中间结果的句柄*/
    void *pvHisHnd;                     /* Point to RD_DI_HND or RC_DB_INFO. 外部通道句柄或中间结果的任务相关信息句柄*/
    BOOL *pbBuf;                        /* Data buffer(if it's middle result).中间结果的数据缓冲 */
    BOOL bDelay; /* 是否滞后录波标志 */
} RC_DI_DB;

/* Infomation of every REC_FLAG.每个标志，包括AI，DI，中间结果的相关访问信息 */
typedef struct
{
    enum
    {
        SIG_NOT_INIT,
        REAL_AI_CH,		/* 原始REAL AI */
        CPLX_AI_CH,
        ORG_DI_CH,
        ELEM_IO_AI,
        ELEM_IO_DI
    } sigtp;			/* 标志类别 */
    void *pvDatSrc;                     /* RD_AI_HND, RD_DI_HND or EP_ELEM_IO.标志的句柄，对外部通道而言，是HANDLE，
                                           				对中间结果而言，是元件指针 */
    int iChOfst;                        /* Offset of FLAG_AI in input channels. 对外部通道AI，DI标志而言，
                                          是外部模拟通道相对通道基址（实或复）的句柄，对中间结果标志而言，
                                          是该任务的任务号*/
    /* Page and item infomation for head init. */
    int iPage;                           /* 标志的页号 */
    int iItem;                          /* 标志的序号 */
} RC_FLAG_DB;

/* 标志的块信息，是与标志区相关的，对每个标志区而言，1个标志块大小包括该标志区相关的
    所有原始REAL AI，COMPLEX  AI，原始DI标志和中间结果AI，DI标志的1次记录 */
typedef struct
{
    uint16_t unRptSN;                 /* 标志块所在的报告号 */
    uint32_t ulRecTime;               /* 标志块的记录AI时刻 */
    uint32_t ulDatTime;               /* 标志块的数据窗AI时刻 */
    uint32_t aulFlagSet[1];             /* Malloc more memory in fact. */
} RC_FLAG_BLK;

/* 任务标志记录区相关的标志信息 */
typedef struct
{
    int iAiNum;	/* 该任务相关的AI标志总数，实际包括3组，原始REAL  AI，原始COMPLEX AI，中间结果AI */
    int iDiNum;		/* 该任务相关的DI标志总数 实际包括2组，原始DI，中间结果DI */
    int iModWordNum;/* 该任务相关的方式字标志总数 实际包括1组，中间结果方式字 2006-11-24日张云*/
    uint8_t *pucHead;			/* 该记录区标志头缓冲指针，申请得到 */
    int iHeadSz;				/* 该记录区标志头缓冲指针，大小为20+6*ptskfg->iAiNum+5*ptskfg->iDiNum */
    RC_FLAG_DB **pprfgDi;		/* 该任务的DI标志的访问信息指针的数组，第1层是申请的数组，第2层是指向对应的RC_FLAG_DB的指针  */
    RC_FLAG_DB **pprfgAi;				/* 该任务的AI标志的访问信息指针的数组，第1层是申请的数组，第2层是指向对应的RC_FLAG_DB的指针  */
    RC_FLAG_DB **pprfgModword;/*该任务的方式字标志的访问信息指针的数组，第1层是申请的数组，第2层是指向对应的RC_FLAG_DB的指针  2006-11-24日张云*/
    uint32_t   *pulLastModwordFlagVal;  /* 上次方式字标志记录保存的值数组  2006-11-24日张云*/
    RC_FLAG_BLK **ppfgblk;		/* 第1层是数组，第2层是申请的每个标志区的RC_FLAG_BLK块的指针，
                           									1个RC_FLAG_BLK块指该标志区1次的标志记录 */
    u_int uiFlagSz;        	/* 该记录区的标志的每次记录的字节数，每个AI标志占8个字节，每8个DI占1个字节 ，
                           						包括该任务相关的所有AI，DI标志 */
    int iBlkWrPos;         	/* 当前待写的标志块号  */
    int iUsedBlks;          			/* 已写到标志缓冲，但未读到报告文件中的待处理的标志块数*/
    int bInFault;           			/* 是否处于故障启动态 */
    uint16_t unLstRptSN;    /* 录波号 */
} RC_TSK_FLAG;

/* Infomation of REC_AIs/REC_DIs related to spectial logic scan task. 中间结果录波的相关任务信息*/
typedef struct
{
    /* Save data every how many AI pts(rate of task scan). 任务扫描间隔 */
    int iInterval;
    uint32_t ulBgnCnt;                  /* AI count of the first data in buffer.中间结果DB的buf中当第1次缓冲的AI计数 */
    int iBufLen;                        /* Max data points saved in buffer. 缓冲最大的点数，为最大录波周期数的任务录波点数*/
    int iWorkOfst;                      /* Next write position in buffer. 下次要写的点，指写到AIdb或DIDB中的pxbuf的位置，
                                         以ulBgnCnt为起点*/
    int iRdOfst;                        /* Current read position in buffer. 当前读的点,指在pxbuf中从pxbuf已搬到录波块的位置*/
} RC_DB_INFO;

/* Command of saving real-time data to fixed buffer.录波段中录波块的操作命令*/
typedef struct
{
    u_int uiFlag;    /*为0时，表示非启停状态的录波块  */
#define RC_RPT_BGN_FG   0x0001		/* 故障报告启动块 */
#define RC_RPT_END_FG   0x0002			/* 故障报告停止块 */
#define RC_REC_BGN_FG   0x0004				/* 录波段录波启动块 */
#define RC_REC_END_FG   0x0008		/* 录波段录波停止块 */
#define RC_HEAD_OK_FG   0x0010			/*  这是一个初始化过的录波块*/

    int iDatIntvl;                      /* Record data interval. 录波段时间间隔*/
    uint32_t ulFirstCnt;                /* AI count of the first record data. 录波块的要记录的
                                        数据第1点AI COUNT，若是第1次，则指前向录波的起点*/

    uint32_t ulNextCnt;                 /* Data of ulNextCnt not saved here. 录波块中下次待保存的录波数据的AI COUNT，
                                         也即ulNextCnt-1为此次记录的最后一点*/
    uint32_t ulBgnCnt;                  /* AI count of the start event. 所属的系统故障启动的AI COUNT*/
    uint32_t ulRecRefUs; /* 录波启动时间, 替代节拍 */
    uint16_t unRptSN;                   /* SN of the report. 报告号*/
    uint8_t ucRecSN;                    /* SN of the record. 录波号*/
} RC_BUF_CMD;

/* Fixed data buffer. 录波块的定义，实际申请时，每个录波块占0.5允许录波周期数的
   所有录波AI，DI数据的大小。比这个定义大  */
typedef struct
{
    RC_BUF_CMD head;                    /* 录波缓冲命令 */
    uint64_t aullDat[1];                /* Malloc more memory in fact. */
} RC_DAT_BLK;

/* locals */

static int iRecFileNum_g;               /* Total number of record file. */
static uint32_t ulRecRefCnt_g;          /* Refrence AI count of the start event. 故障启动的AI COUNT */
/* AI count of data already give record command. 当前录波已写到录波块中的截止AI COUNT-1，也即下次写的起点*/
static uint32_t ulRecRefUs_g; /* 故障启动参考us, 替代节拍 */

static uint32_t ulRecRunCnt_g;
static uint32_t ulRecToutCnt_g;         /* AI count when end of this segment. 当前录波段终止时的AI  COUNT*/
static int iRecIntvl_g;                 /* Record interval of this segment.录波段的录波点间隔点数（已AI周期为单位） */
int iRecAiNum_g;                        /* Total Number of configed REC_AIs. 配置的录波AI的总数，包括原始通道和中间结果*/
static int iRunRecAi_g;                 /* Number of really running REC_AIs. 实际录波的录波AI总数*/
static RC_AI_CFG *praicfg_g;            /* Configuration of all REC_AIs. 所有的录波AI的配置数组*/
static RC_AI_DB *praidb_g;              /* Infomation of running REC_AIs. 所有的录波AI的访问信息数组*/
/* Point to infomation of REC_AIs related to special logic scan task. */
static RC_AI_DB **appraidbTsk_g[MAX_SUB_LGC_NUM];/*该变量是一个数组，数组的每个成员是每个任务相关的指针数组，而该指针数组的
每个指针是该任务的每个相关中间结果AI录波量的访问信息指针，也就是说，第1层是固定数组，第2层是申请的数组首址，第3层是RC_AI_DB *指针  */

int iRecDiNum_g;                        /* Total Number of configed REC_DIs. 配置的录波DI的总数*/
static int iRunRecDi_g;                 /* Number of really running REC_DIs. 实际录波的录波DI总数*/
static RC_DI_CFG *prdicfg_g;            /* Configuration of all REC_DIs.所有的录波DI的配置数组 */
static RC_DI_DB *prdidb_g;              /* Infomation of running REC_DIs. 所有的录波DI的访问信息数组*/
/* Point to infomation of REC_DIs related to special logic scan task. */
static RC_DI_DB **apprdidbTsk_g[MAX_SUB_LGC_NUM];/*该变量是一个数组，数组的每个成员是每个任务相关的指针数组，而该指针数组的
每个指针是该任务的每个相关中间结果DI录波量的访问信息指针，也就是说，第1层是固定数组，第2层是申请的数组首址，
第3层是RC_AI_DB *指针  */

/* Infomation of REC_AIs/REC_DIs(indexed by number of the logic scan task). 任务相关的中间结果录波访问信息数组*/
static RC_DB_INFO adbinf_g[MAX_SUB_LGC_NUM];
/* Point to infomation of REC_AIs/REC_DIs related to running logic scan task.
 * Terminated by NULL. 录波运行任务相关的信息指针数组，未用的不在里面，NULL结束，所以需要+1*/
static RC_DB_INFO *apdbinfRec_g[MAX_SUB_LGC_NUM+1];

int iFgPgNum_g;                         /* Page number of REC_FLAG.标志集页数 */
static RC_FLAG_PAGE *pfgpg_g;           /* Infomation of REC_FLAG pages. 标志页配置数组*/

int iRecFgNum_g;                         /* 所有标志页的标志总数 */
static RC_FLAG_CFG *prfgcfg_g;           /* 所有标志配置的数组*/
static RC_FLAG_DB *prfgdb_g;            /* DB infomation of all configured flags.所有标志的访问信息数组 */
static int iFgOrgDi_g;                   /* 原始DI标志的个数 */
static int iFgRealAi_g;                  /* 原始REAL AI标志的个数 */
static int iFgCplxAi_g;                  /* 原始COMPLEX AI标志的个数 */
static RC_FLAG_DB **pprfgOrgDi_g;        /* 原始DI标志的访问信息指针的数组，第1层为数组，第2层为指向每个DI的RC_FLAG_DB的指针 */
static RC_FLAG_DB **pprfgRealAi_g;       /* 原始REAL AI标志的访问信息指针的数组，第1层为数组，第2层为指向每个DI的RC_FLAG_DB的指针 */
static RC_FLAG_DB **pprfgCplxAi_g;       /* 原始COMPLEX AI标志的访问信息指针的数组，第1层为数组，第2层为指向每个DI的RC_FLAG_DB的指针 */

static RC_TSK_FLAG atskfg_g[MAX_SUB_LGC_NUM];   /*标志任务区信息数组  */
static RC_TSK_FLAG *aptskfg_g[MAX_SUB_LGC_NUM+1];/*有标志记录的任务区信息指针数组  */
static int iFgTskNum_g;                           /* 有标志记录的任务区个数 */
static int iFgDatBlk_g;                           /* 标志记录块的允许记录个数 */

static RC_DAT_BLK **ppdatblk_g;         /* Index of all fixed data block.录波块的数组，每个录波块占0.5允许录波周期数的
   所有录波数据的大小。这是一个申请的数组，数组的成员为每个录波块的指针，为指向RC_DAT_BLK的指针 */
static int iBlkNum_g;                   /* Number of data blocks.录波块的允许个数 */
static int iPtsPerBlk_g;                /* Points of data in one block. 每个块的录波点数大小*/
/* Length of one point data in uint64_t unit.每个录波点的数据最大大小，以uint64_t为单位*/
static int iDatPerPts_g;
static int iBlkWrPos_g;                 /* Block for next write operation. 在整个录波缓冲录波块数组中录波块的当前写位置*/
static int iBlkRdPos_g;                 /* Block for current read operatin. 在整个录波缓冲录波块数组中录波块的读位置，
                                         这里读应是指读出，然后写到录波文件中*/
static int iUsedBlks_g;                 /* Number of blocks containning data.录波缓冲中当前已写到缓冲
                                          但还未读出（也就是写到录波文件中）的剩余录波块的数目，注意这里不是位置 */

static MSG_Q_ID queSaveRtDat_g=NULL;         /* Queue of command to save real-time data. 保存录波数据的消息号   张云 2008-1-22*/
static SEM_ID semMkRecFile_g=NULL;           /* Semorphore to wake up task of writing file.写录波数据文件的信号量 张云 2008-1-22*/

static uint8_t *pucFileBuf_g;           /* Temp buffer before write to file. 写故障报告文件的临时缓冲，
                                           */
static uint32_t ulFileBufSz_g;          /* Size of the temp file buffer.大小 */

static uint8_t *pucRecHead_g;           /* Fixed head of record information. 故障报告的录波信息头的缓冲，从录波信息大小开始*/
static int iRecHeadSz_g;                /* Size of the head. 缓冲大小*/

static int iPtsBytes_g;                 /* Size of one point record data. 录波段中每次录波点记录的字节大小，从录波信息大小开始*/

static uint8_t aucFlagHead_g[9];        /* Fixed head of flag information. 故障报告的标志集的头，从标志信息大小开始*/

static uint8_t *pucSetBuf_g;            /* Buffer to save recorded settings. 故障报告的定植缓冲，从定植信息大小开始*/
static uint32_t ulSetLen_g;             /* Size of the buffer. 缓冲大小*/
static uint8_t *pucLinkBuf_g;           /* Buffer to save recorded links. 故障报告的压板缓冲，从压板信息大小开始*/
static uint32_t ulLinkLen_g;            /* Size of the buffer.缓冲大小 */
static uint8_t *pucFuncBuf_g;           /* Buffer to save recorded protect functions. 故障报告的功能投退的缓冲，从功能投退信息大小开始*/
static uint32_t ulFuncLen_g;            /* Size of the buffer. 缓冲大小*/

/*为填加即刻录波采样而填加的变量  */
static   BOOL  bOnLuboSample_g;  /*是否处于即刻录波状态  */
static   uint32_t  ulLuboSamAICnt_g;   /*即刻录波长度持续的AI count数  */
/*为了监测录波两任务的正常与否，而设置的全局变量任务ID号  */
static uint32_t s_MaxRecMs = 0;  /* 最长录波时间 */
static uint32_t s_MinDiskSpace = 0;    /* 最小允许磁盘空间 */
static uint32_t s_MinWrDiskSpace = 0;    /* 写操作时最小允许磁盘空间 */
static uint32_t s_MinDataDiskSpace = 0;    /* 最小数据盘允许磁盘空间 */

static uint32_t s_ulRecRAMSpaceSize; /* 录波用RAM空间大小 */

/* globals */

int  nRecBufTaskID_g;
int  nRecFileTaskID_g;
BOOL    bRecBufTaskStartFlag_g=FALSE;
BOOL    bRecFileTaskStartFlag_g=FALSE;
uint8_t uWrRecDataErrSts;
BOOL bRecModIsInit_g = FALSE;	/* If rec module initialized. */
FLT_U32_UNION   FloatZero_g;  //浮点0.0,2010-1-11  ZY
/* global functions */

extern  BOOL    bRecDirExistFlag_g;/*录波目录创建成功标志  */

BOOL bRecBlkRewind = FALSE;  /* 录波块回卷 */

VALUE_TYPE gTmpRecbuf[MAX_REC_TEMP_BUFSIZE];

extern SEM_ID semCkCRCIni_g;

/* 中间录波量缓冲总点数 */
uint32_t ulRecBufPtsNum = 0;

/* 滞后录波点数 */
int32_t lDelayPstNum = 0;

/* 滞后录波标志 */
BOOL bDelayFillBuf = FALSE; /* 填写录波滞后标志 */

/*每个录波文件的故障开始时间*/
EP_DATE_TIME g_tRecBeginTm;
uint32_t g_ulRecBeginTmUs;
#if 0
BOOL g_bRecBeginTmBeforeLs = FALSE; /*故障开始时间是否是闰秒之前*/
BOOL g_bRecEndTmAfterLs = FALSE; /*故障结束时间是否是闰秒之后*/
#endif
uint32_t g_ulRecInter = 0; /*录波点间隔*/
uint32_t g_ulPtsToBeginInPiece = 0; /*该段录波第1点相对系统故障开始时间的相对时间间隔*/

/* global functions*/
void Init_Tmp_Recbuf()
{
    int i;
    for (i=0; i<MAX_REC_TEMP_BUFSIZE; i++)
    {
        gTmpRecbuf[i].xVal=0.0+0.0i;
    }
}

/* local functions */

/***********************************************************************
* RC_Save_Dat - 保存录波数据任务，指从db缓冲中写到录波块中，不允许浮点操作
*
* RETURNS: 无
*
*/
static int RC_Save_Dat(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

/***********************************************************************
* RC_Make_File - 形成录波文件任务  ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: 文件描述符
*
*/
static int RC_Make_File(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

/***********************************************************************
* RC_Init_Rec_Head - 填写录波部分的文件头
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Init_Rec_Head(void);

/***********************************************************************
* RC_Init_Flag_Head - 初始化故障报告的标志集头到缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Flag_Head(void);

/***********************************************************************
* RC_Init_Set_Buf - 填写定植头缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Set_Buf(void);

/***********************************************************************
* RC_Init_Link_Buf - 填写压板头缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Link_Buf(void);

/***********************************************************************
* RC_Init_Func_Buf - 填写保护功能投退头缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Func_Buf(void);

/***********************************************************************
* RC_Wr_Next_Blk - 写下一个录波块
*
* RETURNS: 下一个要写（指从db缓冲写到录波块）的位置, 同时释放信号量，驱动读一个录波块到报告文件中，
*
* Alert:
*		   若读录波块到报告文件中处理不过来，表示无空闲录波块可供写，则返回NULL
*
*/
static RC_DAT_BLK *RC_Wr_Next_Blk(void);

/* 清零录波块
 * Para:
 *     pblk, 录波块.
 *     ulAiCnt, 录波开始节拍.
 *     iSaveCnt, 录波点数.
 * Return:
 *     NONE.
 */
static void RC_Clear_Blk(RC_DAT_BLK *pblk, uint32_t ulAiCnt, int iSaveCnt);

/***********************************************************************
* RC_Fill_Blk - 将原始通道和中间结果db缓冲的录波量倒到录波块中，填充一个录波块，该录波块并不一定要填满
*
* RETURNS: 文件描述符
*
*/
static void RC_Fill_Blk(
    RC_DAT_BLK *pblk,
    uint32_t ulAiCnt, 		/* 表示当前开始倒的AI时刻 */
    int iSaveCnt							/* 表示倒到录波块中的AI COUNT持续时间长度 */
);

/***********************************************************************
* RC_Init_New_Blk - 初始化一个新的录波块
*
* RETURNS: 无
*
*/
static void RC_Init_New_Blk(
    RC_DAT_BLK *pblk,
    RC_BUF_CMD *pcmd,
    uint32_t ulInitCnt
);

/***********************************************************************
* RC_New_File - 创建新的故障报告文件，并写文件头和录波头  *pulMsgSz，录波头的长度 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: 文件描述符
*
*/
static int RC_New_File(
    RC_DAT_BLK *pblk,
    uint32_t *pulMsgSz
);

/***********************************************************************
* RC_End_File - 结束报告文件 ulMsgSz为录波信息长度 iPiece为录波段数 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static void RC_End_File(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t ulMsgSz,
    int iPiece
);

/***********************************************************************
* RC_File_Added - 若录波文件数超过最大值，则删除最久的录波文件
*
* RETURNS: 无
*
*/
static void RC_File_Added(void);

/***********************************************************************
* RC_New_Piece - 开始记录新的录波段，写录波段段头，录波段点数尚未填写 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_New_Piece(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t *pRtWrLen
);

/***********************************************************************
* RC_End_Piece - 结束本录波段的记录  lFilePos，本录波段的起始位置，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_End_Piece(
    int iFd,
    uint32_t lFilePos,
    uint32_t ulPts
);

/***********************************************************************
* RC_Wr_Rec_Dat - 记录一般的录波段中间的录波块数据 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Wr_Rec_Dat(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t  *pRtWrLen,
    uint8_t *puErrSts
);

/***********************************************************************
* RC_Append_Flag - 往故障报告文件中添加标志集记录  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Flag(
    int iFd,
    RC_DAT_BLK *pblk
);

/***********************************************************************
* RC_Wr_Flag_Dat - 往故障报告中添加某标志区的标志集的内容，返回标志块数  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Wr_Flag_Dat(
    int iFd,
    RC_TSK_FLAG *ptskfg,
    RC_DAT_BLK *pblk,
    uint32_t *pRtWrRecNum
);

/***********************************************************************
* RC_Append_Set - 往故障报告中添加定值信息  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Set(
    int iFd
);

/***********************************************************************
* RC_Append_Link - 往故障报告中添加压板信息  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Link(
    int iFd
);

/***********************************************************************
* RC_Append_Func - 往故障报告中添加保护功能投退信息  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Func(
    int iFd
);

/***********************************************************************
* RC_MakeFileTaskExecHandle - 为了处理录波文件异常时的处理函数，张云添加
*
* RETURNS: 无
*
*/
static void RC_MakeFileTaskExecHandle(
    int iFd,			/* 文件描述符 */
    int iExecReason		/* 异常原因 */
);

/***********************************************************************
* GetDataDiskLeftSize - 获取Data盘剩余空间大小
*
* RETURNS: OK, or ERROR
*
*/
STATUS GetDataDiskLeftSize(
    int *piSize		/* 剩余空间大小 */
);

/* 删除部分录波文件
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
static BOOL RC_DelOldRptFile(void);

/***********************************************************************
* GetAdjustTimeSuccessFlag - MMI_SOFT.C提供的对时成功标志,
*
* RETURNS:
*					TRUE: 04板初始化后对时成功
*					FALSE: 04板初始化后还没有对时成功
*
*/
extern BOOL GetAdjustTimeSuccessFlag();

/* functjions */

/***********************************************************************
* RC_FlagSet_Init - 初始化标志集相关信息，必须在逻辑图运行之前调用
*
* RETURNS: 无
* 注意: 无
*
*/
static EP_STATUS RC_FlagSet_Init(void);

/* functions */

/***********************************************************************
* RC_FlagSet_Init - 初始化标志集相关信息，必须在逻辑图运行之前调用
*
* RETURNS: 无
* 注意: 无
*
*/
EP_STATUS RC_FlagSet_Init(void)
{
    uint32_t ulBlkSize;
    RC_TSK_FLAG *ptskfg;
    RC_TSK_FLAG **pptskfg;
    int i;
    EP_STATUS sts;

    /* 更新标志区相关信息 */
    ulBlkSize=0;
    pptskfg=aptskfg_g;
    while ((ptskfg=*pptskfg++) != NULL)
    {
        /* 对每个被投入的标志的任务进行循环 */
        ptskfg->iAiNum += iFgRealAi_g+iFgCplxAi_g;		/* 该任务标志区AI FLAG的个数，包括原始REAL AI，COMPLEX AI，中间结果AI */
        ptskfg->iDiNum+=iFgOrgDi_g;    		/* 该任务标志区DI，FLAOG的个数，包括原始DI，中间结果DI */

        ptskfg->uiFlagSz=ptskfg->iAiNum*8+(ptskfg->iDiNum+7)/8;			/* 该标志区每次标志记录大小， */
        ulBlkSize+=ptskfg->uiFlagSz;     	/* 每个标志块的最大大小等于所有标志任务区的1次标志记录的和 。
                                          					 这里只为了计算标志块的允许个数，而实际这些任务的每次标志记录并没有合在一起，
                                           				    而是单独的 */
    }

    if(ulBlkSize==0)		/* 张云修改 */
        iFgDatBlk_g=0;
    else if (FLAG_MEM_BUF/ulBlkSize>MAX_FLAG_NUM)
        iFgDatBlk_g=MAX_FLAG_NUM;
    else
        iFgDatBlk_g=FLAG_MEM_BUF/ulBlkSize;			/* 获得标志块允许个数 */

    pptskfg=aptskfg_g;
    while ((ptskfg=*pptskfg++) != NULL)
    {
        /* 对每个标志区分配所有标志块，每次记录1次标志，算1个标志块 */
        if ((ptskfg->ppfgblk=calloc(iFgDatBlk_g,
                                    sizeof(*ptskfg->ppfgblk))) == NULL)
            return EP_BUF_ERR;

        for (i=0; i<iFgDatBlk_g; i++)
        {
            /* 为该标志区分配所有标志块 */
            /* Malloc memory for every block(4 bytes more for
                * using uint32_t dealing with DI). */
            if ((ptskfg->ppfgblk[i]=calloc(1, sizeof(RC_FLAG_BLK)+
                                           ptskfg->uiFlagSz))==NULL)
                return EP_BUF_ERR;
        }
    }

    sts=RC_Init_Flag_Head();
    if (sts != EP_SUCCESS)
        return sts;

    return   EP_SUCCESS;
}

/***********************************************************************
* RC_Initialize - This function should be called after logic intialization. 录波模块初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
* 注意: 在逻辑图初始化之后，逻辑图运行之前，被调用，此时有些数据结构已申请过了
*
*/
EP_STATUS RC_Initialize(void)
{
    RC_AI_DB *praidb;
    RC_DI_DB *prdidb;
    uint32_t ulBlkSize;
    int i;
    EP_STATUS sts;

#if defined(EDP_01_02_BUILD)
    int iNandFlashSize;
    int iMaxRecNumSet;		/* 根据flash大小得到的录波文件个数 */
#endif

    FloatZero_g.fVal=0.0;//2010-1-11 ZY

    /* 根据应用设置参数 */
    if (uiAppType_g == APP_STAB_CONTROL)
    {
        s_MaxRecMs = MAX_REC_MS_LONG;
        s_MinDiskSpace = MIN_ALLOW_DISK_FREE_SPACE_OTHER;
        s_MinWrDiskSpace = MIN_ALLOW_DISK_WRITE_SPACE_OTHER;
        s_MinDataDiskSpace = MIN_ALLOW_DATA_DISK_SPACE_OTHER;

        s_ulRecRAMSpaceSize = SMALL_REC_MEM_BUF;
    }
    else
    {
        s_MaxRecMs = MAX_REC_MS;
        s_MinDiskSpace = MIN_ALLOW_DISK_FREE_SPACE;
        s_MinWrDiskSpace = MIN_ALLOW_DISK_WRITE_SPACE;
        s_MinDataDiskSpace = MIN_ALLOW_DATA_DISK_SPACE;

        s_ulRecRAMSpaceSize = REC_MEM_BUF;
    }

    rptsts_g.bAllocBlk = TRUE;  /* 允许发送录波消息 */

    queSaveRtDat_g =msgQCreate(MAX_REC_MSG_QUE, sizeof(RC_BUF_CMD), MSG_Q_PRIORITY); /* 原始为16 */
    assert(queSaveRtDat_g != NULL);

    semMkRecFile_g=semCCreate(SEM_Q_PRIORITY, 0);
    assert(semMkRecFile_g != NULL);

    if(RC_FlagSet_Init() != EP_SUCCESS)		/* 初始化标志集，必须在逻辑图运行之前 */
    {
        return EP_CFG_ERR;
    }

    if(queSaveRtDat_g == NULL)
    {
        /* 2006-2-8日修改 */
        return EP_SUCCESS;
    }

    rptsts_g.iMaxRecNum=MAX_REC_FILE_NUM;

#if defined(EDP_01_02_BUILD)
    iNandFlashSize=Ffx_Get_Nand_Size_In_MegaByte();
    LOG_Dbg_Msg("可用NandFlash大小为%d M!\n", iNandFlashSize, 0, 0, 0, 0, 0);
    if(iNandFlashSize>200)
    {
        iMaxRecNumSet=MAX_REC_FILE_NUM;    /* 注意不要太大，不然搜索太慢 */
    }
    else if(iNandFlashSize>50)
    {
        iMaxRecNumSet=MAX_REC_FILE_NUM;
    }
    else if(iNandFlashSize>28)
    {
        iMaxRecNumSet=16;
    }
    else  if(iNandFlashSize>14)
    {
        iMaxRecNumSet=16;		/* 最少为16份 */
    }
    else
    {
        LOG_Write(LOG_KERNEL, "存储空间过小!!\n", NULL);

        iMaxRecNumSet=16;	/* 最少为16份 */
    }

    if(rptsts_g.iMaxRecNum>iMaxRecNumSet)
    {
        /* 当设定值大于实际能容纳值时，使用实际能容纳值 */
        rptsts_g.iMaxRecNum=iMaxRecNumSet;
    }
#endif

    /* 获得真正被录波的AI量，未被投入的任务的录波AI量未被计入 */
    iRunRecAi_g=iRecAiNum_g;
    for (praidb=praidb_g; praidb<praidb_g+iRecAiNum_g; praidb++)
    {
        if (!praidb->pvHisHnd)
            iRunRecAi_g--;
    }

    /* 获得真正被录波的DI量，未被投入的任务的录波DI量未被计入 */
    iRunRecDi_g=iRecDiNum_g;
    for (prdidb=prdidb_g; prdidb<prdidb_g+iRecDiNum_g; prdidb++)
    {
        if (!prdidb->pvHisHnd)
            iRunRecDi_g--;
    }

    /* Every block can save  cycles data in sample rate. 每个录波块的采样点数，每个录波块允许大小为一定周期数 */
    iPtsPerBlk_g=uiAiPts_g*REC_BLK_BUF_CYC;    /* 2006-7-11日 张云 */

    bOnLuboSample_g=FALSE;  		/* 设置不处于即刻录波状态 */
    ulLuboSamAICnt_g=MAX_LUBO_SAMPLE_MS*uiAiRate_g/1000;   						/* 即刻录波持续的AI COUNT数 */
    assert(ulLuboSamAICnt_g<iPtsPerBlk_g);

    assert(sizeof(uint64_t)==sizeof(COMPLEX));
    iDatPerPts_g=iRunRecAi_g+
                 (iRunRecDi_g*sizeof(BOOL)+sizeof(uint64_t)-1)/sizeof(uint64_t)+1;
    /* 每个点的实际录波数据大小，以uint64_t为单位,包括AI,DI,DOBUF,和实际的记录的AICNT */

    ulBlkSize=sizeof(RC_BUF_CMD)+
              (uint32_t)iPtsPerBlk_g*iDatPerPts_g*sizeof(uint64_t);		/* 每个录波块的字节大小 */

    if (s_ulRecRAMSpaceSize/ulBlkSize>INT_MAX)
        iBlkNum_g=INT_MAX;
    else
        iBlkNum_g=s_ulRecRAMSpaceSize/ulBlkSize;				/* 计算可分配的录波块数 */
    /* 分配所有录波块 */
    if ((ppdatblk_g=calloc(iBlkNum_g, sizeof(*ppdatblk_g)))==NULL)
        return EP_BUF_ERR;

    for (i=0; i<iBlkNum_g; i++)
    {
        /* Malloc memory for every block. */
        if ((ppdatblk_g[i]=calloc(1, ulBlkSize))==NULL)
            return EP_BUF_ERR;
    }
    /* 初始化故障报告中的相应条目的头信息 */
    sts=RC_Init_Rec_Head();
    if (sts!=EP_SUCCESS)
        return sts;

    sts=RC_Init_Set_Buf();
    if (sts!=EP_SUCCESS)
        return sts;

    sts=RC_Init_Link_Buf();
    if (sts!=EP_SUCCESS)
        return sts;

    sts=RC_Init_Func_Buf();
    if (sts!=EP_SUCCESS)
        return sts;

    /* 启动录波记录任务 */
    nRecBufTaskID_g=taskSpawn("tRecBuf", TSK_PRI_REC_BUF, 0, 100000, RC_Save_Dat,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    assert(nRecBufTaskID_g!=ERROR);
    bRecBufTaskStartFlag_g=TRUE;

    if(!bRecDirExistFlag_g)
    {
        /*若录波目录未存在,则不创建录波文件任务  */
        return EP_NOT_INIT;
    }

    bRecModIsInit_g = TRUE;

    return EP_SUCCESS;
}

/***********************************************************************
* RC_After_Relay_Init - This function should be called after logic intialization. 录波模块保护启动之后初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
* 注意: 在逻辑图初始化之后，逻辑图运行之前，被调用，此时有些数据结构已申请过了
*
*/
EP_STATUS RC_After_Relay_Init(void)
{
    if (!bRecModIsInit_g)
    {
        return EP_ERROR;
    }

    iRecFileNum_g = lstCount(pmRecFileList_g);

    /* 启动故障报告文件生成任务 */
    nRecFileTaskID_g = taskSpawn("tRecFile", TSK_PRI_REC_FILE, 0, 100000, RC_Make_File,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    assert (nRecFileTaskID_g != ERROR);
    bRecFileTaskStartFlag_g = TRUE;

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Cfg_Flag - 读取软件配置文件中的标志集配置，并初始化原始通道标志访问信息，中间结果标志在逻辑图初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Flag(
    uint8_t *pucCfg, 		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
)
{
    uint8_t *puc;
    uint8_t *pucPgBgn;
    int i, j;
    RC_FLAG_PAGE *pfgpg;
    RC_FLAG_CFG *pflag;
    RC_FLAG_DB *prfgdb;
    int iPgCfgLen;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    int iItemCfgLen;
    RC_MODWORD_BIT_FLAG_CFG *pBitFlagCfg;


    iFgPgNum_g=*pucCfg;		/* 页数 */

    if ((pfgpg_g=calloc(iFgPgNum_g, sizeof(*pfgpg_g))) == NULL)
        return EP_BUF_ERR;

    pucPgBgn=pucCfg+5;

    for (pfgpg=pfgpg_g; pfgpg<pfgpg_g+iFgPgNum_g; pfgpg++)
    {
        /* 循环读取每一标志页的公共信息 */
        puc=pucPgBgn;

        iPgCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc += 7+puc[2];

        pfgpg->bIsPub=*puc++ ? TRUE:FALSE;
        if (!pfgpg->bIsPub)
            puc += 1+puc[0];

        pfgpg->iFlagNum=U8_TO_U16(puc[1], puc[0]);
        pfgpg->iFlagPos=iRecFgNum_g;
        iRecFgNum_g += pfgpg->iFlagNum;		/* 将所有页的标志个数相加 */

        pucPgBgn += 2+iPgCfgLen;
    }

    if (pucPgBgn-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    if ((prfgcfg_g=calloc(iRecFgNum_g, sizeof(*prfgcfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((prfgdb_g=calloc(iRecFgNum_g, sizeof(*prfgdb_g)))==NULL)
        return EP_BUF_ERR;

    if ((pprfgOrgDi_g=calloc(iRecFgNum_g+1, sizeof(*pprfgOrgDi_g)))==NULL)
        return EP_BUF_ERR;

    if ((pprfgRealAi_g=calloc(iRecFgNum_g+1, sizeof(*pprfgRealAi_g)))==NULL)
        return EP_BUF_ERR;

    if ((pprfgCplxAi_g=calloc(iRecFgNum_g+1, sizeof(*pprfgCplxAi_g)))==NULL)
        return EP_BUF_ERR;

    puc=pucCfg+5;
    pflag=prfgcfg_g;
    prfgdb=prfgdb_g;

    for (pfgpg=pfgpg_g; pfgpg<pfgpg_g+iFgPgNum_g; pfgpg++)
    {
        /*读取标志页中的信息  */
        iPgCfgLen=U8_TO_U16(puc[1], puc[0]);

        iPgCfgLen-=puc[2];
        EP_ID_Copy(pfgpg->aucName, puc+3, puc[2]);
        puc+=3+puc[2];

        pfgpg->aucABRV[0]=*puc++;
        pfgpg->aucABRV[1]=*puc++;
        pfgpg->aucABRV[2]=*puc++;
        pfgpg->aucABRV[3]=*puc++;

        puc++;                          /* Skip IsPub. */

        if (!pfgpg->bIsPub)
        {
            /*若是专用标志页  */
            iPgCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];

            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    /*查找逻辑图名  */
                    pfgpg->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (!pfgpg->psublgc)
            {
                /*若找不到，则出错  */
                LOG_Dbg_Msg("ERROR: can't find protect %s for link.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }

        puc+=6;                         /* Skip FlagCount & 4 reserved bytes. */

        for (i=0; i<pfgpg->iFlagNum; i++, pflag++, prfgdb++)
        {
            /* 读取标志页中的标志 */
            iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
            puc+=2;

            iPgCfgLen-=2+iItemCfgLen;

            iItemCfgLen-=puc[0];
            EP_ID_Copy(pflag->aucId, puc+1, puc[0]);
            puc+=1+puc[0];

            iItemCfgLen-=puc[0];
            EP_ID_Copy(pflag->aucName, puc+1, puc[0]);
            puc+=1+puc[0];

            pflag->aucABRV[0]=*puc++;
            pflag->aucABRV[1]=*puc++;
            pflag->aucABRV[2]=*puc++;
            pflag->aucABRV[3]=*puc++;

            if((*puc)==1)
            {
                /* 若是方式字标志 */
                puc++;

                pflag->bHexword=TRUE;
                if ((pflag->pModWordBitFlagArr=calloc(32, sizeof(*(pflag->pModWordBitFlagArr)))) == NULL)
                {
                    return EP_BUF_ERR;
                }
                pBitFlagCfg=pflag->pModWordBitFlagArr;
                for(j=0; j<32; j++)
                {
                    iItemCfgLen -= (puc[0]+1+4);
                    EP_ID_Copy(pBitFlagCfg->aucName, puc+1, puc[0]);
                    puc += 1+puc[0];

                    pBitFlagCfg->aucABRV[0]=*puc++;
                    pBitFlagCfg->aucABRV[1]=*puc++;
                    pBitFlagCfg->aucABRV[2]=*puc++;
                    pBitFlagCfg->aucABRV[3]=*puc++;
                    pBitFlagCfg++;
                }
            }
            else
            {
                puc++;
                pflag->bHexword=FALSE;
                pflag->pModWordBitFlagArr=NULL;
            }
            puc += 3;

            pflag->ucUnit=*puc++;
            if(pflag->bHexword)
            {
                /* 若是方式字标志，则进行信号类型检测 */
                assert(pflag->ucUnit==0x6B);
            }

            assert(iItemCfgLen==11);

            /* Initialize database of REC_FLAG. */
            if (IS_REAL_AI(pflag->ucUnit) || IS_CPLX_AI(pflag->ucUnit))
            {
                prfgdb->pvDatSrc=RD_Flag_AI_Hnd(pflag->aucId);
                if (prfgdb->pvDatSrc)
                {
                    /*若是原始AI标志  */
                    if (pflag->ucUnit!=AI_HND_TO_UNIT(prfgdb->pvDatSrc))
                    {
                        LOG_Dbg_Msg("ERROR: unit mismatch for FLAG_AI \"%s\".\n",
                                    (int)pflag->aucId, 0, 0, 0, 0, 0);

                        return EP_PARM_ERR;
                    }

                    if (IS_REAL_AI(pflag->ucUnit)) /* 是瞬时值 */
                    {
                        prfgdb->sigtp=REAL_AI_CH;
                        prfgdb->iChOfst=RD_Lgc_AI_Ofst(prfgdb->pvDatSrc);
                        pprfgRealAi_g[iFgRealAi_g++]=prfgdb;/* 获得原始AI标志的个数 */
                    }
                    else
                    {
                        prfgdb->sigtp=CPLX_AI_CH;
                        prfgdb->iChOfst=RD_Calc_AI_Ofst(prfgdb->pvDatSrc);
                        pprfgCplxAi_g[iFgCplxAi_g++]=prfgdb; /* 获得预处理AI标志的个数 */
                    }

                    prfgdb->iPage=pfgpg-pfgpg_g;
                    prfgdb->iItem=i;
                }
            }
            else if (IS_BOOL_SIG(pflag->ucUnit))
            {
                /*  */
                prfgdb->pvDatSrc=RD_Flag_DI_Hnd(pflag->aucId);
                if (prfgdb->pvDatSrc)
                {
                    /*若是原始DI标志，初始化  */
                    prfgdb->sigtp=ORG_DI_CH;

                    pprfgOrgDi_g[iFgOrgDi_g++]=prfgdb;

                    prfgdb->iPage=pfgpg-pfgpg_g;
                    prfgdb->iItem=i;
                }
            }
        }

        assert(iPgCfgLen==12);
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Cfg_Rec_AI - 读取软件配置文件中的录波AI配置，初始化原始通道录波AI的访问信息，中间结果录波AI在逻辑图中初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Rec_AI(
    uint8_t *pucCfg, 		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
)
{
    uint8_t *puc;
    RC_AI_CFG *praicfg;
    RC_AI_DB *praidb;
    int iItemCfgLen;

    puc=pucCfg;

    iRecAiNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=6;

    if ((praicfg_g=calloc(iRecAiNum_g, sizeof(*praicfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((praidb_g=calloc(iRecAiNum_g, sizeof(*praidb_g)))==NULL)
        return EP_BUF_ERR;

    for (praicfg=praicfg_g, praidb=praidb_g;
            praicfg<praicfg_g+iRecAiNum_g; praicfg++, praidb++)
    {
        /* 对每个录波AI进行初始化 */
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(praicfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(praicfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        praicfg->bNotNeedPrint=((*puc)&0x01)?TRUE:FALSE ;
        praicfg->bNotNeedUpSend=((*puc)&0x02)?TRUE:FALSE ;

        puc+=1;
        praicfg->ucPageNum=*puc;
        puc+=3;

        praicfg->ucUnit=*puc++;

        assert(iItemCfgLen==7);

        /* Initialize database of REC_AI. */
        praidb->pvHisHnd=RD_Rec_AI_Hnd(praicfg->aucId);
        if (praidb->pvHisHnd)
        {
            /* 若是原始通道录波AI，则初始化访问信息 */
            if (praicfg->ucUnit!=AI_HND_TO_UNIT(praidb->pvHisHnd))
            {
                LOG_Dbg_Msg("ERROR: unit mismatch for REC_AI \"%s\".\n",
                            (int)praicfg->aucId, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }

            if (IS_REAL_AI(AI_HND_TO_UNIT(praidb->pvHisHnd)))
            {
                /*若是实型 */
                praidb->bIsReal=TRUE;
                praidb->iChOfst=RD_Lgc_AI_Ofst(praidb->pvHisHnd);
            }
            else/* 若是复数*/
                praidb->iChOfst=RD_Calc_AI_Ofst(praidb->pvHisHnd);

            praicfg->bSrcType=0;		/* 外部通道来源 */
        }
        else
        {
            praicfg->bSrcType=1;		/* 中间计算结果来源 */
        }
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Cfg_Rec_DI - 读取软件配置文件中的录波DI配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Rec_DI(
    uint8_t *pucCfg,		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
)
{
    uint8_t *puc;
    RC_DI_CFG *prdicfg;
    RC_DI_DB *prdidb;
    int iItemCfgLen;

    puc=pucCfg;

    iRecDiNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=6;

    if ((prdicfg_g=calloc(iRecDiNum_g, sizeof(*prdicfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((prdidb_g=calloc(iRecDiNum_g, sizeof(*prdidb_g)))==NULL)
        return EP_BUF_ERR;

    for (prdicfg=prdicfg_g, prdidb=prdidb_g;
            prdicfg<prdicfg_g+iRecDiNum_g; prdicfg++, prdidb++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(prdicfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(prdicfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        prdicfg->bNotNeedPrint=((*puc)&0x01)?TRUE:FALSE ;
        prdicfg->bNotNeedUpSend=((*puc)&0x02)?TRUE:FALSE ;
        prdicfg->bNotDORec=((*puc)&0x04)?TRUE:FALSE ;

        puc+=1;
        prdicfg->ucPageNum=*puc;
        puc+=3;

        assert(iItemCfgLen==6);

        /* Initialize database of REC_DI. 若不是原始DI，则该handle此处表示为NULL*/
        prdidb->pvHisHnd=RD_Rec_DI_Hnd(prdicfg->aucId);

        if(prdidb->pvHisHnd)
        {
            prdicfg->bSrcType=0;		/* 外部 */
        }
        else
        {
            prdicfg->bSrcType=1;		/* 中间结果 */
        }
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}


/****************************************录波记录功能的访问函数接口定义********************************/

/****************************************录波记录功能的访问函数接口定义********************************/


/*     逻辑图中添加1个新的中间结果录波量到录波量集中

       参数：   strID  , 该录波量的逻辑标识
                pLuboSignal,  该录波量的数据访问指针,用于录波记录时,访问该录波量
                ulScanTaskNo, 该录波量所在的逻辑图扫描任务号。
                              用于录波时区分不同任务的录波量

       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的录波量
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的录波量
                   EP_PARA_ERR,因录波量数据指针参数和调试配置模块中的录波量配置信息
                               不一致,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Add_New_Lubo_Signal(
    uint8_t *strID, 			/* 该录波量的逻辑标识 */
    EP_ELEM_IO *pLuboSignal,				/* 该录波量的数据访问指针,用于录波记录时,访问该录波量 */
    uint32_t ulScanTaskNo			/* 该录波量所在的逻辑图扫描任务号。
                              								 用于录波时区分不同任务的录波量 */
)
{
    RC_DB_INFO **ppdbinf;
    RC_AI_CFG *praicfg;
    RC_AI_DB *praidb;
    RC_DI_CFG *prdicfg;
    RC_DI_DB *prdidb;
    RC_AI_DB **ppraidbTsk;
    RC_DI_DB **pprdidbTsk;
    int i;
    BOOL bFind;

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pLuboSignal);
    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    /* Init database infomation when this scan task first reach here. */
    if (!adbinf_g[ulScanTaskNo].iInterval)
    {
        /* 若该任务相关的录波第1次进入，初始化任务相关信息 */
        i=RE_Lgc_Scan_Interval(ulScanTaskNo);
        assert(i);

        adbinf_g[ulScanTaskNo].iInterval=i;

        /* i=((g_RdBufCyc+1)*uiAiPts_g+i-1)/i; */	/* i-1，获取最接近的值 */
        i=(g_RdBufCyc+1)*uiAiPts_g;              /* 消除锯齿录波，使用采样值所使用的缓冲大小 */
        adbinf_g[ulScanTaskNo].iBufLen=i;			/* 设置该任务允许的最大录波点数，与扫描任务相关，时间为g_RdBufCyc */
        ulRecBufPtsNum = i;
        if (bDelayFillBuf)
        {
            lDelayPstNum = (LINE_REC_DELAY_TIME*uiAiPts_g)/(1000000/uiPwrFreq_g);
        }
        else
        {
            lDelayPstNum = 0;
        }

        /* Make offset pointing to the -1 position.
         * So ulBgnCnt will be correct at first time logic scan. */
        adbinf_g[ulScanTaskNo].iWorkOfst=i-1;

        for (ppdbinf=apdbinfRec_g;
                ppdbinf<apdbinfRec_g+MAX_SUB_LGC_NUM; ppdbinf++)
        {
            if (!*ppdbinf)
            {
                /* 往apdbinfRec_g数组中未使用的成员中添加该指针，然后跳出 */
                *ppdbinf=adbinf_g+ulScanTaskNo;
                break;
            }
        }
        assert(ppdbinf<apdbinfRec_g+MAX_SUB_LGC_NUM);
    }

    bFind=FALSE;
    for (praicfg=praicfg_g; praicfg<praicfg_g+iRecAiNum_g; praicfg++)
    {
        /* 在录波AI配置表中查询 */
        if (!strcmp(strID, praicfg->aucId))
        {
            /* 若找到同名的 */
            praidb=praidb_g+(praicfg-praicfg_g);
            /* 找到相应的录波AI访问信息DB */
            if (praidb->pelmSrc || praidb->pvHisHnd)
            {
                /* 若已经用过了，出错返回 */
                LOG_Dbg_Msg("ERROR: using REC_AI \"%s\" more than once.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }

            if (praicfg->ucUnit != pLuboSignal->ucAttrib)
            {
                /* 若属性不对，出错返回 */
                LOG_Dbg_Msg("ERROR: unit mismatch for REC_AI \"%s\".\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }

            if (!bFind)
            {
                bFind=TRUE;
                if (!appraidbTsk_g[ulScanTaskNo])
                {
                    /* 若头一回进入该任务，则为该任务申请指向所有录波AI通道的访问信息的指针数组 */
                    if ((appraidbTsk_g[ulScanTaskNo]=
                                calloc(iRecAiNum_g+1, sizeof(*appraidbTsk_g[0])))==NULL)
                        return EP_SYS_ERR;
                }

                for (ppraidbTsk=appraidbTsk_g[ulScanTaskNo];
                        ppraidbTsk<appraidbTsk_g[ulScanTaskNo]+iRecAiNum_g;
                        ppraidbTsk++)
                {
                    /* 对该指针数组进行循环，若找到一个未使用的，则将该指针进行赋值为该AI的访问信息DB */
                    if (!*ppraidbTsk)
                    {
                        *ppraidbTsk=praidb;
                        break;
                    }
                }
                assert(ppraidbTsk<appraidbTsk_g[ulScanTaskNo]+iRecAiNum_g);

                {
                    /*为节省内存，而改为动态申请*/
                    VALUE_TYPE *p;
                    if ((p=calloc(adbinf_g[ulScanTaskNo].iInterval-1,
                                  sizeof(VALUE_TYPE)))==NULL)
                        return EP_SYS_ERR;
                    else
                    {
                        int k;
                        pLuboSignal->recbuf=p;
                        for (k=0; k<adbinf_g[ulScanTaskNo].iInterval-1; k++)
                        {
                            pLuboSignal->recbuf[k].xVal=0.0+0.0i;
                        }
                    }
                }

                praidb->pelmSrc=pLuboSignal;/*设置相关信息  */
                praidb->pvHisHnd=adbinf_g+ulScanTaskNo;

                /* Malloc data buffer. 为中间结果申请缓冲*/
                if ((praidb->pxBuf=calloc(adbinf_g[ulScanTaskNo].iBufLen,
                                          sizeof(*praidb->pxBuf)))==NULL)
                    return EP_SYS_ERR;
            }
            else
            {
                /* 若找到多个同名的录波AI，则退出 */
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in REC_DAT.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    for (prdicfg=prdicfg_g; prdicfg<prdicfg_g+iRecDiNum_g; prdicfg++)
    {
        /* 若录波AI中未查到，则继续在录波DI配置表中查 */
        if (!strcmp(strID, prdicfg->aucId))
        {
            /* 若找到 */
            prdidb=prdidb_g+(prdicfg-prdicfg_g);

#if 0   /* 根据应用需求，不再使用延时功能 */
            prdidb->bDelay = FALSE;
            if (((!strcmp(strID, LINE_CUST_DI_REC1))
                    || (!strcmp(strID, LINE_CUST_DI_REC2))
                    || (!strcmp(strID, LINE_CUST_DI_REC3)))
                    && (uiAppType_g == APP_LINE))
            {
                bDelayFillBuf = TRUE;
                prdidb->bDelay = TRUE;
            }
#endif

            if (prdidb->pelmSrc || prdidb->pvHisHnd)
            {
                LOG_Dbg_Msg("ERROR: using REC_DI \"%s\" more then once.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }

            if (!bFind)
            {
                bFind=TRUE;

                if (!apprdidbTsk_g[ulScanTaskNo])
                {
                    /* 若该任务的DI，头一回使用，则申请 */
                    if ((apprdidbTsk_g[ulScanTaskNo]=
                                calloc(iRecDiNum_g+1, sizeof(*apprdidbTsk_g[0])))==NULL)
                        return EP_SYS_ERR;
                }

                for (pprdidbTsk=apprdidbTsk_g[ulScanTaskNo];
                        pprdidbTsk<apprdidbTsk_g[ulScanTaskNo]+iRecDiNum_g;
                        pprdidbTsk++)
                {
                    if (!*pprdidbTsk)
                    {
                        /* 若找到未用的，则添加 */
                        *pprdidbTsk=prdidb;
                        break;
                    }
                }
                assert(pprdidbTsk<apprdidbTsk_g[ulScanTaskNo]+iRecDiNum_g);

                prdidb->pelmSrc=pLuboSignal;
                prdidb->pvHisHnd=adbinf_g+ulScanTaskNo;

                /* Malloc data buffer. */
                if ((prdidb->pbBuf=calloc(adbinf_g[ulScanTaskNo].iBufLen,
                                          sizeof(*prdidb->pbBuf)))==NULL)
                    return EP_SYS_ERR;
            }
            else
            {
                /* 若在录波DI中找到多个，则出错 */
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in REC_DAT.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        /* 若在录波AI量表和录波DI量表都未查找到，则出错 */
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of REC_DAT.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/*     设置录波启动标志
       用于逻辑图扫描过程中触发录波启动
       参数：     pLuboStartInfo, 启动录波的信息
                  ulScnAiCnt,进行本次逻辑图扫描时的AI采样计数器值

       返回值：    无
*/
void SCI_Set_Lubo_Start_Flag(
    SCI_LUBO_START_INFO_TYPE *pLuboStartInfo,			/* 启动录波的信息 */
    uint32_t ulScnAiCnt		/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{

    STATUS vxsts;
    uint32_t ulRecLen;
    RC_BUF_CMD cmd;
    int iNewIntvl;
    uint32_t ulNewRecToutCnt;
    uint32_t ulRecToutCntTemp;
    uint32_t ulFirstCntTemp;


    if (!bRecModIsInit_g)
    {
        return;
    }

    /* 获得录波点间隔 */
    iNewIntvl=uiAiRate_g/pLuboStartInfo->unLuboFreq;
    if (!iNewIntvl)
        iNewIntvl=1;

    taskLock();
    /*若已在录波，且录波频率同新的设置不一致，则停止上次录波 */
    if (rptsts_g.bRecOn && iRecIntvl_g != iNewIntvl)
        SCI_Set_Lubo_Stop_Flag(ulScnAiCnt);

    iRecIntvl_g=iNewIntvl;/* 设置新的录波间隔 */

    if (!pLuboStartInfo->ucBackwardLuboTimeType &&
            pLuboStartInfo->ulBackwardLuboTime<s_MaxRecMs)
        ulRecLen=pLuboStartInfo->ulBackwardLuboTime*uiAiRate_g/1000;/*获得该录波段的录波点数，  */
    else
        ulRecLen=s_MaxRecMs*uiAiRate_g/1000;

    ulNewRecToutCnt=ulScnAiCnt+ulRecLen;/*2006-11-12日 张云修改BUG  */

    if (!rptsts_g.bRecOn)
    {
        /*若当前未录波,则启动新的录波段，否则延续现在和以前合并的录波段  */
        ulRecToutCntTemp=ulRecToutCnt_g;
        ulRecToutCnt_g=ulNewRecToutCnt;/* 设置当前录波段的终止时刻 *//*2006-11-12日 张云修改BUG  */

        if (rptsts_g.bRecInRpt)/*若当前正在生成报告 ，设置录波启动命令*/
            cmd.uiFlag=RC_REC_BGN_FG;
        else
        {
            /* 否则，设置录波启动和报告启动命令，和故障启动时刻 */
            if (rptsts_g.iFault)/*若处于故障启动态  */
            {
                ulRecRefCnt_g=rptsts_g.ulEvtBgnCnt;
                ulRecRefUs_g = RD_AI_Cnt_To_us(rptsts_g.ulEvtBgnCnt); /* 立即转化为us */
            }
            else/*若是独立录波段  */
            {
                ulRecRefCnt_g=ulScnAiCnt;
                ulRecRefUs_g = RD_AI_Cnt_To_us(ulScnAiCnt); /* 立即转化为us */
            }
            rptsts_g.bRecInRpt=TRUE;/*设置录波启动和报告启动命令  */
            cmd.uiFlag=RC_RPT_BGN_FG | RC_REC_BGN_FG;
        }

        cmd.iDatIntvl=iRecIntvl_g;

        /* 稳控装置和励磁装置 */
        if ((uiAppType_g == APP_STAB_CONTROL)
                || (uiAppType_g == APP_EXCITE))
        {
            if(pLuboStartInfo->unForwardLuboTime<MAX_FORWARD_REC_MS_LONG)
                ulRecLen=pLuboStartInfo->unForwardLuboTime*uiAiRate_g/1000;
            else
                ulRecLen=MAX_FORWARD_REC_MS_LONG*uiAiRate_g/1000;
        }
        else
        {
            ulRecLen=pLuboStartInfo->unForwardLuboTime*uiAiRate_g/1000;
            if (ulRecLen>iPtsPerBlk_g)
                ulRecLen=iPtsPerBlk_g;      /* Maximal backward rec one data block.前向录波时间不超过1个录波块， */
        }

        ulFirstCntTemp=ulScnAiCnt-ulRecLen;
        if((ulRecToutCntTemp-ulFirstCntTemp)<MAX_REC_OVERLAP_LONG*uiAiRate_g/1000)
            cmd.ulFirstCnt=ulRecToutCntTemp;
        else
            cmd.ulFirstCnt=ulScnAiCnt-ulRecLen;/*获得该录波段的起始时刻，从前向录波起点开始  */

        cmd.ulNextCnt=ulScnAiCnt;/*录波的下一个时刻  */
        if((int)(cmd.ulFirstCnt-cmd.ulNextCnt)>0)
            cmd.ulFirstCnt=cmd.ulNextCnt;

        cmd.ulBgnCnt=ulRecRefCnt_g;/* 故障启动时刻 */
        cmd.ulRecRefUs = ulRecRefUs_g; /* 使用us数, 防止转换精度损失 */

        /* These can't be ommitted because they are useful when
         * the block is forced end for buffer reason. */
        cmd.unRptSN=rptsts_g.unRptSN;  /* 当前报告和录波号 */
        cmd.ucRecSN=rptsts_g.ucRecSN;

        /* if(bViewModIsInit_g) */
        {
            /*2006-8-2日张云修改，为了防止生成报告号还没有初始化的录波报告  */
            vxsts=msgQSend(queSaveRtDat_g, (char*)&cmd, sizeof(cmd),/*释放消息，驱动从db缓冲中写前向录波的一块录波数据到录波块中 */
                           NO_WAIT, MSG_PRI_NORMAL);
            if (vxsts!=OK)
            {
                /*若队列已满  */
                static   uint32_t   ulMsgCnt=0;
                if(ulMsgCnt%2000==0)
                {
                    LOG_Dbg_Msg("ERROR: no time to save REC_AI to buffer.\n",
                                0, 0, 0, 0, 0, 0);

                    LOG_Write(LOG_RUN, "启动录波时发送录波消息失败!\n", NULL);
                }
                ulMsgCnt++;
            }
        }

        ulRecRunCnt_g=ulScnAiCnt;/* 当前录波记录运行时刻 */
        rptsts_g.bRecOn=TRUE;
    }
    else/*2006-11-12日 张云修改BUG  */
    {
        if(((int)(ulNewRecToutCnt-ulRecToutCnt_g))>0)
        {
            /* 若处于录波状态，且新的录波段结束时刻比以前的设置结束时刻晚，则以新的为准 延续现在和以前合并的录波段 */
            ulRecToutCnt_g=ulNewRecToutCnt;
        }

    }

    taskUnlock();

}

/*     设置录波停止标志
       用于逻辑图扫描过程中触发录波停止
       参数：     ulAiCnt, 停止录波时的AI采样计数器值


       返回值：    无
*/
void SCI_Set_Lubo_Stop_Flag(
    uint32_t ulAiCnt			/* 停止录波时的AI采样计数器值 */
)
{
    /* 停止当前录波段的录波 */
    STATUS vxsts;
    RC_BUF_CMD cmd;

    if (!bRecModIsInit_g)
    {
        return;
    }

    taskLock();

    ulRecToutCnt_g=ulAiCnt;/* ZY  2011-5-26   */
    if (rptsts_g.bRecOn)
    {
        /* 若当前处于录波启动态 */
        /* Command to save data. */
        cmd.uiFlag=RC_REC_END_FG;
        cmd.iDatIntvl=iRecIntvl_g;
        cmd.ulFirstCnt=ulRecRunCnt_g;			/* 本次的起点 */
        cmd.ulNextCnt=ulAiCnt;
        cmd.ulBgnCnt=ulRecRefCnt_g;
        cmd.ulRecRefUs = ulRecRefUs_g;

        cmd.unRptSN=rptsts_g.unRptSN;
        cmd.ucRecSN=rptsts_g.ucRecSN++;			/* 为下一个录波段号加1 */

        if (!rptsts_g.iFault)
        {
            /* 若当前不是故障启动态，则是独立录波报告，可形成故障报告 */
            /* Not in a fault. Only one piece in one rec file. */
            cmd.uiFlag |= RC_RPT_END_FG;								/* 设置故障报告形成命令 */
            rptsts_g.bRecInRpt=FALSE;							/* 报告启动结束 */
            rptsts_g.unRptSN++;  							/* 为下一报告号+1 */
            rptsts_g.ucRecSN=0;			/* 为下一报告的录波段号重新设为0 */
        }

        rptsts_g.bRecOn=FALSE;			/* 停止录波 */

        if(bOnLuboSample_g)
        {
            /* 若当前处于即刻录波态，即认为该录波段结束为该即刻录波的结束 */
            bOnLuboSample_g=FALSE;
        }

        /* if(bViewModIsInit_g)  */
        {
            /* AI模块是否已初始化 */
            vxsts=msgQSend(queSaveRtDat_g, (char*)&cmd, sizeof(cmd),			/* 释放消息，将DB中还未记录到录波块中的信息记录到录波块中 */
                           NO_WAIT, MSG_PRI_NORMAL);
            if (vxsts!=OK)
            {
                /* 若队列已满 2008-1-22日 张云扩大*/
                static uint32_t ulMsgCnt=0;
                if(ulMsgCnt%10000==0)
                {
                    LOG_Dbg_Msg("ERROR: no time to save REC_AI to buffer.\n", 0, 0, 0, 0, 0, 0);
                    LOG_Write(LOG_RUN, "停止录波时发送录波消息失败!\n", NULL);
                }
                ulMsgCnt++;
            }
        }
    }

    taskUnlock();
}

/***********************************************************************
* RC_End_Wave -  若所有故障都返回了，停止整个故障报告，供故障停止函数VI_End_Fault调用
*
* RETURNS: 无
*
*/
void RC_End_Wave(
    uint32_t ulAiCnt
)
{
    RC_BUF_CMD cmd;
    STATUS vxsts;

    if (!bRecModIsInit_g)
    {
        return;
    }

    ulRecToutCnt_g=ulAiCnt;/* ZY  2011-5-26   */
    if (rptsts_g.bRecInRpt)
    {
        /* 若处于报告启动态，则停止故障报告，否则空操作 */
        /* Command to save data. */
        if (rptsts_g.bRecOn)
        {
            /* 若处于录波启动态，则设置录波停止和报告停止命令 */
            cmd.uiFlag=RC_REC_END_FG | RC_RPT_END_FG;
            cmd.ulFirstCnt=ulRecRunCnt_g;
            rptsts_g.bRecOn=FALSE;
        }
        else
        {
            /* 否则，只设置报告停止命令 */
            cmd.uiFlag=RC_RPT_END_FG;
            cmd.ulFirstCnt=ulAiCnt;
        }

        cmd.iDatIntvl=iRecIntvl_g;

        cmd.ulNextCnt=ulAiCnt;
        cmd.ulBgnCnt=ulRecRefCnt_g;
        cmd.ulRecRefUs = ulRecRefUs_g;

        cmd.unRptSN=rptsts_g.unRptSN;
        cmd.ucRecSN=rptsts_g.ucRecSN;

        /* if(bViewModIsInit_g) */
        {
            /* AI模块是否已初始化 */
            vxsts=msgQSend(queSaveRtDat_g, (char*)&cmd, sizeof(cmd),		/* 释放消息，将DB中还未记录到录波块中的信息记录到录波块中 */
                           NO_WAIT, MSG_PRI_NORMAL);
            if (vxsts!=OK)
            {
                /* 若队列已满  2008-1-22日 张云扩大 */
                static uint32_t ulMsgCnt=0;
                if(ulMsgCnt%10000==0)
                {
                    LOG_Dbg_Msg("ERROR: no time to save REC_AI to buffer.\n", 0, 0, 0, 0, 0, 0);
                    LOG_Write(LOG_RUN, "故障返回时发送录波消息失败!\n", NULL);
                }
                ulMsgCnt++;
            }
        }

        rptsts_g.bRecInRpt=FALSE;
    }
}

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Lubo - 处理本扫描任务的本次逻辑图扫描周期的录波
*
* RETURNS: 无
*
* Alert: 实现时，若上次录波尚未停止,或此次触发新的录波,则进行本任务相关的录波,否则此次不录波
*
*
*/
void SCI_Process_Cur_Logrp_Period_Lubo(
    uint32_t ulScanTaskNo,		/* 任务号 */
    uint32_t ulScnAiCnt							/* 采样节拍 */
)
{
    RC_DB_INFO *pdbinf;
    RC_DI_DB **pprdidb;
    RC_DI_DB *prdidb;
    RC_AI_DB **ppraidb;
    RC_AI_DB *praidb;
    static uint32_t ulLastAiCnt;
    RC_BUF_CMD cmd;
    int i;
    int iSaveCnt;
    int k;
    BOOL bCurValue;
    int *pDiOfstBytes;
    int *pAiOfstBytes;
    uint32_t ulTempAiCnt;

    int   iForwordOfst; /* 2008-8-14日，张云修改 */

    STATUS vxsts;


#ifdef EXCITE_BUILD
#define MAX_REC_OFST_BUF_NUM 48
#else
#define MAX_REC_OFST_BUF_NUM 32
#endif
    int aiDiOfstBytes[MAX_REC_OFST_BUF_NUM];
    int aiAiOfstBytes[MAX_REC_OFST_BUF_NUM];

    if (!bRecModIsInit_g)
    {
        return;
    }

    pdbinf=adbinf_g+ulScanTaskNo;
    i=pdbinf->iInterval;
    if(i>MAX_REC_OFST_BUF_NUM)
    {
        /* 若逻辑图扫描间隔点数太多，则录波不处理 */
        return;
    }

    iSaveCnt=i;
    pDiOfstBytes=aiDiOfstBytes;
    pAiOfstBytes=aiAiOfstBytes;
    ulTempAiCnt=ulScnAiCnt-i;

    /* 2008-8-14日，张云修改，中间结果AI录波，后向写入录波缓冲，
       对中间结果DI和SPIBUF录波，必须前向写入录波缓冲，这样才和实际出口行为符合 */

    for(k=0; k<iSaveCnt; k++)
    {
        ulTempAiCnt++;
        if (++(pdbinf->iWorkOfst)>=pdbinf->iBufLen)
        {
            /* 若计数超出缓冲允许长度，则从零重新计数 ，这实际可认为是一个循环队列 */
            pdbinf->iWorkOfst=0;
            pdbinf->ulBgnCnt=ulTempAiCnt;
        }
        *pAiOfstBytes++=pdbinf->iWorkOfst*sizeof(COMPLEX);
    }

    iForwordOfst=pdbinf->iWorkOfst;
    for(k=0; k<iSaveCnt; k++)
    {
        *pDiOfstBytes++=iForwordOfst*sizeof(BOOL);
        if (++iForwordOfst>=pdbinf->iBufLen)
        {
            /* 若计数超出缓冲允许长度，则从零重新计数 ，这实际可认为是一个循环队列 */
            iForwordOfst=0;
        }
    }


    /* Save ralated real-time mid-result data to buffer. */
    if ((pprdidb=apprdidbTsk_g[ulScanTaskNo]) != NULL)
    {
        /* 若有中间结果DI录波 */
        while ((prdidb=*pprdidb++) != NULL)
        {
            /* 循环读取每个DI录波的db信息，将该DI录波信息写入对应DB信息的缓冲 */

            bCurValue=prdidb->pelmSrc->now.bVal;
            pDiOfstBytes=aiDiOfstBytes;

            for(k=0; k<iSaveCnt; k++)
            {
                *(BOOL*)((uint8_t*)prdidb->pbBuf+*pDiOfstBytes++)=bCurValue;
            }
        }
    }

    if ((ppraidb=appraidbTsk_g[ulScanTaskNo]) != NULL)
    {
        /* 若有中间结果AI录波，包括REAL和COMPLEX型 */
        while ((praidb=*ppraidb++) != NULL)
        {
            /* 循环读取每个AI录波的db信息，将该AI录波信息写入对应DB信息的缓冲 */

            pAiOfstBytes=aiAiOfstBytes;
            //if(pdbinf->iInterval<=MAX_REC_RESULT_NUM)
            {
                /* 逻辑图间隔不大于缓冲池，则保存缓冲池中的历史值 */

                for(k=0; k<iSaveCnt-1; k++)
                {
                    *(COMPLEX*)((uint8_t*)praidb->pxBuf+*pAiOfstBytes++)=
                        praidb->pelmSrc->recbuf[iSaveCnt-2-k].xVal;
                }
                *(COMPLEX*)((uint8_t*)praidb->pxBuf+*pAiOfstBytes++)=
                    praidb->pelmSrc->now.xVal;
            }
            /*else
            {	// 否则保存最新值
            for(k=0; k<iSaveCnt; k++)
            	{
             		*(COMPLEX*)((uint8_t*)praidb->pxBuf+*pAiOfstBytes++)=
               		praidb->pelmSrc->now.xVal;
             	}
            }*/
        }
    }

    ulScnAiCnt=ulScnAiCnt-i;

    while(i--)              /* Has data related to this task. */
    {
        /* 若该任务有中间结果录波数据相关联，则将当前点的中间结果存入缓冲,依采样点保存 */

        ulScnAiCnt++;/*2006-7-10,*/

        taskLock();


        if ((int32_t)(ulScnAiCnt-ulLastAiCnt)>0)/* 这个判别是什么意思，这个判别正常运行时
                                             的不会出现FALSE，因为除非首次引用AIcount */
        {
            ulLastAiCnt=ulScnAiCnt;

            /* Judge if it is the time to record data. */
            if (rptsts_g.bRecOn)
            {
                /*若正在录波  */
                if (!bOnLuboSample_g&&(((int)(ulScnAiCnt-ulRecRunCnt_g))>=((int)(iPtsPerBlk_g+uiAiPts_g))))
                {
                    /* 若录波时间大于1个录波块，，则发命令记录录波块，将db缓冲的信息写1个录波块长度的点到录波块 */
                    /* Delay one cycle then record(avoid missing slow protect data). */
                    cmd.uiFlag=0;/*表示处于非启停录波状态  */
                    cmd.iDatIntvl=iRecIntvl_g;
                    cmd.ulFirstCnt=ulRecRunCnt_g;/*此次记录块的起始点 */

                    ulRecRunCnt_g+=iPtsPerBlk_g;

                    if (ulRecRunCnt_g >= ulRecToutCnt_g)
                    {
                        ulRecRunCnt_g = ulRecToutCnt_g;
                    }

                    cmd.ulNextCnt=ulRecRunCnt_g;/*下次记录块的起点，即此次记录的终点的后一点 */

                    cmd.ulBgnCnt=ulRecRefCnt_g;/* 故障启动时刻 */
                    cmd.ulRecRefUs = ulRecRefUs_g;

                    /* These can't be ommitted because they are useful when
                     * the block is forced end for buffer reason. */
                    cmd.unRptSN=rptsts_g.unRptSN;
                    cmd.ucRecSN=rptsts_g.ucRecSN;

                    /* if(bViewModIsInit_g) */
                    {
                        /*2006-8-2日张云修改，为了防止生成报告号还没有初始化的录波报告  */
                        vxsts=msgQSend(queSaveRtDat_g, (char*)&cmd, sizeof(cmd),/*释放消息，将db缓冲的信息写1个录波块长度的点到录波块*/
                                       NO_WAIT, MSG_PRI_NORMAL);
                        if (vxsts!=OK)
                        {
                            /*若队列已满 2008-1-22日 张云扩大 */
                            static   uint32_t   ulMsgCnt=0;
                            if(ulMsgCnt%10000==0)
                            {
                                LOG_Dbg_Msg("ERROR: no time to save REC_AI to buffer.\n",
                                            0, 0, 0, 0, 0, 0);

                                LOG_Write(LOG_RUN, "逻辑图扫描时发送录波消息失败!\n", NULL);
                            }
                            ulMsgCnt++;
                        }
                    }
                }
                else  if(bOnLuboSample_g&&((ulScnAiCnt-ulRecRunCnt_g)>=ulLuboSamAICnt_g+uiAiPts_g))
                {
                    /*若是即刻录波，且录波持续时间超过录波采样时间  */

                    cmd.uiFlag=0;/*表示处于非启停录波状态  */
                    cmd.iDatIntvl=iRecIntvl_g;
                    cmd.ulFirstCnt=ulRecRunCnt_g;/*此次记录块的起始点 */

                    ulRecRunCnt_g+=(ulLuboSamAICnt_g+1);
                    cmd.ulNextCnt=ulRecRunCnt_g;/*下次记录块的起点，即此次记录的终点的后一点 */

                    cmd.ulBgnCnt=ulRecRefCnt_g;/* 故障启动时刻 */
                    cmd.ulRecRefUs = ulRecRefUs_g;

                    /* These can't be ommitted because they are useful when
                     * the block is forced end for buffer reason. */
                    cmd.unRptSN=rptsts_g.unRptSN;
                    cmd.ucRecSN=rptsts_g.ucRecSN;

                    /* if(bViewModIsInit_g) */
                    {
                        /*2006-8-2日张云修改，为了防止生成报告号还没有初始化的录波报告  */
                        vxsts=msgQSend(queSaveRtDat_g, (char*)&cmd, sizeof(cmd),/*释放消息，将db缓冲的信息写1个录波块长度的点到录波块*/
                                       NO_WAIT, MSG_PRI_NORMAL);
                        if (vxsts!=OK)
                        {
                            /*当队列已满时,则返回ERROR  2008-1-22日 张云扩大*/
                            static   uint32_t   ulMsgCnt=0;
                            if(ulMsgCnt%10000==0)
                            {
                                LOG_Dbg_Msg("ERROR: no time to save REC_AI to buffer.\n",
                                            0, 0, 0, 0, 0, 0);

                                LOG_Write(LOG_RUN, "逻辑图扫描时发送录波消息失败!\n", NULL);
                            }
                            ulMsgCnt++;
                        }
                    }
                }
                if ((int32_t)(ulLastAiCnt-ulRecToutCnt_g) >= 0)/* 若当前段的已经到时，则停止录波 */
                {
                    /* LOG_Dbg_Msg("ulRecRunCnt_g=%d ulLastAiCnt=%d\n", ulRecRunCnt_g, ulLastAiCnt, 0, 0, 0, 0); */
                    SCI_Set_Lubo_Stop_Flag(ulLastAiCnt);
                }
            }/*if (rptsts_g.bRecOn)结束  */
        }/*if ((int32_t)(ulScnAiCnt-ulLastAiCnt)>0)结束*/
        taskUnlock();


    }/*while(i--)结束  */

}

/****************************************标志集记录的访问函数接口定义********************************/

/*     逻辑图中添加1个新的中间结果标志量到标志量集中

       参数：    strID  , 该标志量的逻辑标识
                pFlagSignal,  该标志量的数据访问指针,用于标志记录时,访问该标志量
                ulScanTaskNo, 该标志量所在的逻辑图扫描任务号。
                              用于标志集记录时区分不同任务的标志量

       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的标志量
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的标志量
                   EP_PARA_ERR,因标志量数据指针参数和调试配置模块中的标志量配置信息
                               不一致,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Add_New_Flag_Signal(
    uint8_t *strID,			/* 该标志量的逻辑标识 */
    EP_ELEM_IO *pFlagSignal,			/* 该标志量的数据访问指针,用于标志记录时,访问该标志量 */
    uint32_t ulScanTaskNo			/* 该标志量所在的逻辑图扫描任务号。
                              								 用于标志集记录时区分不同任务的标志量 */
)
{
    RC_FLAG_PAGE *pfgpg;
    RC_FLAG_CFG *pflag;
    RC_FLAG_DB *prfgdb;
    RC_TSK_FLAG *ptskfg;
    int i;
    BOOL bFind;

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pFlagSignal);
    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    bFind=FALSE;
    pflag=prfgcfg_g;

    for (pfgpg=pfgpg_g; pfgpg<pfgpg_g+iFgPgNum_g; pfgpg++)
    {
        /* 在配置表中查找 */
        for (i=0; i<pfgpg->iFlagNum; i++, pflag++)
        {
            if (!strcmp(strID, pflag->aucId))
            {
                /* 若找到同名的逻辑标识 */
                prfgdb=prfgdb_g+(pflag-prfgcfg_g);

                if (prfgdb->pvDatSrc)
                {
                    LOG_Dbg_Msg("ERROR: using FLAG_AI \"%s\" more than once.\n", (int)strID, 0, 0, 0, 0, 0);

                    return EP_NOT_INIT;
                }

                if (pflag->ucUnit != pFlagSignal->ucAttrib)
                {
                    LOG_Dbg_Msg("ERROR: unit mismatch for FLAG_AI \"%s\".\n", (int)strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }

                if (!bFind)
                {
                    bFind=TRUE;

                    prfgdb->iChOfst=ulScanTaskNo;/* 更新对应的DB参数  */
                    prfgdb->pvDatSrc=pFlagSignal;
                    prfgdb->iPage=pfgpg-pfgpg_g;
                    prfgdb->iItem=i;

                    ptskfg=atskfg_g+ulScanTaskNo;
                    if (!ptskfg->iDiNum && !ptskfg->iAiNum)
                    {
                        /* 若是头一回往该任务添加标志量，则往全局变量中添加 */
                        aptskfg_g[iFgTskNum_g++]=ptskfg;
                        assert(iFgTskNum_g<=MAX_SUB_LGC_NUM);

                        /* 若该任务头一回添加标志量，则为该任务申请DB指针数组 */
                        if ((ptskfg->pprfgDi=calloc(iRecFgNum_g+1,
                                                    sizeof(*ptskfg->pprfgDi)))==NULL)
                            return EP_BUF_ERR;

                        if ((ptskfg->pprfgAi=calloc(iRecFgNum_g+1,
                                                    sizeof(*ptskfg->pprfgAi)))==NULL)
                            return EP_BUF_ERR;

                        if ((ptskfg->pprfgModword=calloc(iRecFgNum_g+1,
                                                         sizeof(*ptskfg->pprfgModword)))==NULL)
                            return EP_BUF_ERR;

                        if ((ptskfg->pulLastModwordFlagVal=calloc(iRecFgNum_g+1,
                                                           sizeof(*ptskfg->pulLastModwordFlagVal)))==NULL)
                            return EP_BUF_ERR;
                    }

                    if (IS_BOOL_SIG(pflag->ucUnit))
                    {
                        /* 若是BOOL 信号 */
                        prfgdb->sigtp=ELEM_IO_DI;
                        ptskfg->pprfgDi[ptskfg->iDiNum++]=prfgdb;		/* 将DB信息给全局变量，本任务区的中间标志DI数+1 */
                    }
                    else
                    {
                        /* 若是AI信号 */
                        prfgdb->sigtp=ELEM_IO_AI;
                        ptskfg->pprfgAi[ptskfg->iAiNum++]=prfgdb;		/* 将DB信息给全局变量，本任务区的中间标志AI数+1 */
                    }
                    if(pflag->bHexword)
                    {
                        ptskfg->pprfgModword[ptskfg->iModWordNum++]=prfgdb;		/* 将DB信息给全局变量，本任务区的方式字标志数+1  2006-11-24日张云 */
                    }
                }
                else
                {
                    LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in FLAG_DAT.\n", (int)strID, 0, 0, 0, 0, 0);

                    return EP_NOT_INIT;
                }
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        /* 若找不到，则出错， */
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of FLAG_DAT.\n", (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Flagset_Record - 处理本扫描任务的本次逻辑图扫描周期的标志集记录,该函数在每次逻辑图扫描任务的
        																			  最后进行调用。
*
* RETURNS: 无
*
* Alert:
*        实现时，若当前处于事故报告期间,且在本任务本次扫描时若有逻辑量标志发生变位，则
*        进行本任务相关的标志集记录。
*
*/
void SCI_Process_Cur_Logrp_Period_Flagset_Record(
    uint32_t ulScanTaskNo,			/* 进行本次标志记录的扫描任务号 */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{
    RC_TSK_FLAG *ptskfg;
    RC_FLAG_DB **pprfgdb;
    RC_FLAG_DB *prfgdb;
    RC_FLAG_BLK *pfgblk;
    uint32_t ulDi;
    uint32_t ulBit;
    uint32_t *pul;
    uint32_t *pulNowDi;
    uint32_t *pulLastDi;
    uint16_t unTemp;
    float *pfLgcAi;
    COMPLEX *pxCalcAi;
    BOOL bNeedRec;

    ptskfg=atskfg_g+ulScanTaskNo;

    if (!ptskfg->iDiNum)                /* No data related to this task. 若没有DI标志相关，包括原始通道DI和中间结果DI，则返回 */
        return;

    if (!rptsts_g.iFault)
    {
        /* 若没有故障，则退出 */
        ptskfg->bInFault=FALSE;
        return;
    }

    if (ptskfg->iUsedBlks>=(iFgDatBlk_g-1))
    {
        /*若标志记录个数超出块的大小，则不记录 2008-5-26日张云 修改，解决标志集太多，覆盖以前记录的问题  */
        return ;
    }
    /* 此时表示有故障 */
    unTemp=rptsts_g.unRptSN;

    bNeedRec=FALSE;
    if (!ptskfg->bInFault || ptskfg->unLstRptSN!=unTemp)
    {
        /* 若上次没故障，而此次有故障，或报告号不同，表示进入一次新故障，表示是此次故障的头一回记录，需要记录标志 */
        bNeedRec=TRUE;
        ptskfg->bInFault=TRUE;
        ptskfg->unLstRptSN=unTemp;
    }

    pfgblk=ptskfg->ppfgblk[ptskfg->iBlkWrPos];		/* 此次标志记录待记录的位置 */
    pfgblk->ulDatTime=ulScnAiCnt;			/* 数据窗时刻 */
    /* 获得DI标志在任务块记录中的开始位置 */
    pulNowDi=pfgblk->aulFlagSet+2*ptskfg->iAiNum;			/* ，获得DI标志记录的位置，DI的以4字节，uint32t为单位的偏移，
                                                  											 每32位开关标志，比较处理一次，提高效率，
                                                  									        标志记录时是先8字节为单位的AI标志先记录，再记录DI标志 */

    pul=pulNowDi;
    ulBit=1;
    ulDi=0;
    pprfgdb=pprfgOrgDi_g;			/* 原始通道DI标志的db数组 */
    while ((prfgdb=*pprfgdb++)!=NULL)
    {
        /* 对每个原始通道DI进行 */
        if (RD_His_DI(prfgdb->pvDatSrc, ulScnAiCnt))				/* 该DI输入为TURE，则将1置入ulDi相应位中 */
            ulDi |= ulBit;

        ulBit<<=1;			/* ulBit移位 */
        if (!ulBit)
        {
            /* 若已操作了32个通道，则位置清零，且将该DI的数据写入缓冲 */
            *pul++=ulDi;

            ulBit=1;
            ulDi=0;
        }
    }

    pprfgdb=ptskfg->pprfgDi;			/* 中间结果DI标志的db数组 */
    while ((prfgdb=*pprfgdb++)!=NULL)
    {
        if (((EP_ELEM_IO*)prfgdb->pvDatSrc)->now.bVal)
            ulDi |= ulBit;

        ulBit<<=1;
        if (!ulBit)
        {
            /* 将32位开关量标志写入 */
            *pul++=ulDi;

            ulBit=1;
            ulDi=0;
        }
    }

    if (ulBit!=1)		/* 若不止32的倍数位DI标志，则将剩余的标志记录下来，
          							 注意这里也是32位，而不是8位，最后有效的记录还是以8位来计数的 */
        *pul++=ulDi;

    if (!bNeedRec)
    {
        /* 若前面判定不需要记录，，获得前一标志记录点 以待当前的标志记录和以前的标志记录比较 */
        if (ptskfg->iBlkWrPos)
        {
            /* 若当前记录位置不是第1点 */
            pulLastDi=ptskfg->ppfgblk[ptskfg->iBlkWrPos-1]->aulFlagSet+
                      2*ptskfg->iAiNum;
        }
        else
        {
            /* 若当前记录位置是第1点，则其前一点标志为缓冲的最后一点 */
            pulLastDi=ptskfg->ppfgblk[iFgDatBlk_g-1]->aulFlagSet+
                      2*ptskfg->iAiNum;
        }

        while (pulNowDi<pul)			/* 若没到末尾 */
        {
            /* 将当前待记录的DI标志点和已记录的前一DI标志点比较，每32位比较一次  */
            if (*pulNowDi++!=*pulLastDi++)
            {
                /* 若发生变化，则表示需要记录 */
                bNeedRec=TRUE;
                break;
            }
        }
    }

    /* 增加方式字拷贝 */
    /* if (!bNeedRec) */
    {
        /* 2006-11-24日　张云，判定方式字标志是否变化 */
        int k;
        uint32_t *pulLastVal;
        uint32_t ulCurVal;

        pulLastVal=ptskfg->pulLastModwordFlagVal;
        pprfgdb=ptskfg->pprfgModword;
        for(k=0; k<ptskfg->iModWordNum; k++)
        {
            ulCurVal=((EP_ELEM_IO*)((*pprfgdb)->pvDatSrc))->now.ulVal;
            if((*pulLastVal)!=ulCurVal)
            {
                /* 若发生变化，则表示需要记录 */
                bNeedRec=TRUE;
                *pulLastVal=ulCurVal;
            }

            pprfgdb++;
            pulLastVal++;
        }
    }

    if (bNeedRec)
    {
        /* 若需要记录,则记录AI标志量，因为DI前面已记录了，否则前面记录的DI作废 */
        /* Find DI change. Record the flag. */
        pul=pfgblk->aulFlagSet;

        /* 根据采样节拍获得逻辑通道地址 */
        pfLgcAi=RD_Base_Lgc_AI_P(ulScnAiCnt);
        if(!pfLgcAi)			/* 2006-7-5日，张云 */
        {
            return  ;
        }
        pprfgdb=pprfgRealAi_g;
        while ((prfgdb=*pprfgdb++)!=NULL)
        {
            /* 获得原始REAL AI通道标志，8字节记录它一次 */
            *(float*)pul=*(pfLgcAi+prfgdb->iChOfst);
            pul += 2;
        }

        /* logMsg("SCI_Process_Cur_Logrp_Period_Flagset_Record Wr!\n",0,0,0,0,0,0); */ /* Modified by DY 4/6/2006 */
        /* 根据采样节拍获得预处理通道地址 */
        pxCalcAi=RD_Base_Calc_AI_P(ulScnAiCnt); 		/* 根据当前Ai节拍，获得预处理通道地址 */
        if(!pxCalcAi)			/* 2006-7-5日，张云 */
        {
            return  ;
        }
        pprfgdb=pprfgCplxAi_g;
        while ((prfgdb=*pprfgdb++)!=NULL)		/* 获得原始COMPLEX AI通道标志，并记录它 */
        {
            *(COMPLEX*)pul=*(pxCalcAi+prfgdb->iChOfst);
            pul += 2;
        }

        pprfgdb=ptskfg->pprfgAi;
        while ((prfgdb=*pprfgdb++)!=NULL)			/* 获得中间结果AI标志，并统一用COMPLEX方式记录它 */
        {
            *(COMPLEX*)pul=((EP_ELEM_IO*)prfgdb->pvDatSrc)->now.xVal;

            pul += 2;
        }

        pfgblk->unRptSN=ptskfg->unLstRptSN;			/* 记录此次故障的报告号 */
        pfgblk->ulRecTime=RD_AI_Cnt();								/* 实际记录时刻 */

        if (++ptskfg->iBlkWrPos>=iFgDatBlk_g)			/* 若此次写的位置到缓冲末尾，则回到缓冲的的头 */
            ptskfg->iBlkWrPos=0;

        if (++ptskfg->iUsedBlks>=iFgDatBlk_g)
            ptskfg->iUsedBlks=iFgDatBlk_g-1;
    }
}


/***********************************************************************
* RC_Get_Flag_Pg_Attr - Get REC_FLAG page attribution.
*
* RETURNS:
*				  Pointer to the flag attribution structure.
*              NULL if iIdx is invalid(>=iRecFgNum_g).
*
*/
const RC_FLAG_PAGE *RC_Get_Flag_Pg_Attr(
    int iIdx		/* flag page number(from 0). */
)
{
    if (iIdx<iFgPgNum_g)
        return pfgpg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* RC_Get_Flag_Attr - Get REC_FLAG attribution.
*
* RETURNS:
*				  Pointer to the flag attribution structure.
*              NULL if iIdx is invalid(>=iRecFgNum_g).
*
*/
const RC_FLAG_CFG *RC_Get_Flag_Attr(
    int iIdx		/* index of the flag(from 0). */
)
{
    if (iIdx<iRecFgNum_g)
        return prfgcfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* RC_Get_Rec_AI_Attr - Get record AI attribution.
*
* RETURNS:
*				  Pointer to the REC_AI attribution structure.
*              NULL if iIdx is invalid(>=iRecAiNum_g).
*
*/
const RC_AI_CFG *RC_Get_Rec_AI_Attr(
    int iIdx		/* index of the REC_AI(from 0). */
)
{
    if (iIdx<iRecAiNum_g)
        return praicfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* RC_Get_Rec_DI_Attr - Get record DI attribution.
*
* RETURNS:
*				  Pointer to the REC_DI attribution structure.
*              NULL if iIdx is invalid(>=iRecDiNum_g).
*
*/
const RC_DI_CFG *RC_Get_Rec_DI_Attr(
    int iIdx		/* index of the REC_DI(from 0). */
)
{
    if (iIdx<iRecDiNum_g)
        return prdicfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* RC_Save_Dat - 保存录波数据任务，指从db缓冲中写到录波块中，不允许浮点操作
*
* RETURNS: 无
*
*/
static int RC_Save_Dat(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    RC_BUF_CMD cmd;
    int i;
    RC_DAT_BLK *pblk; 			/* 录波块，主要是一些录波有关的参数以及命令 */
    RC_DAT_BLK *pblkNew;
    uint32_t ulDealCnt;
    int iSaveCnt;

    /* It is not allowed to use float operation in this task.  So make sure
     * width of the data types is fit. */
    assert (sizeof(uint32_t) == sizeof(float) && sizeof(uint64_t) == sizeof(COMPLEX));

    pblk=ppdatblk_g[0]; 		/* 录波块是一个数组，有多个录波命令 */
    while (1)
    {
        i=msgQReceive(queSaveRtDat_g, (char*)&cmd, sizeof(cmd), WAIT_FOREVER); 		/* 从消息队列接受一个消息，消息队列编号为queSaveRtDat_g */
        /* 从消息队列中接受消息 */
        /* 消息也就是命令，按照命令来录波 */
        assert (i == sizeof(cmd));

        if (!pblk)
        {
            /* 当前待录波的块为空，即从录波块中读到文件中处理不过来，则继续读录波块到故障文件中，
               丢掉本次消息,直到有录波块空出，则重新开始一个故障报告 */
            /* Last time buffer full. */
            pblk=RC_Wr_Next_Blk();
            if (pblk)
            {
                /* Begin a new report after buffer ready.但录波块未初始化 */
                cmd.uiFlag |= (RC_REC_BGN_FG | RC_RPT_BGN_FG);				/* 重新开始一个故障报告和录波报告 */
            }
            else
                continue;				/* 继续写到文件，丢掉本次消息, */
        }

        if ((pblk->head.uiFlag & RC_HEAD_OK_FG) && 				/* 这是一个初始化过的录波块 */
                (cmd.iDatIntvl != pblk->head.iDatIntvl || 							/* 当前间隔不相等 */
                 cmd.ulFirstCnt != pblk->head.ulNextCnt)) 						/* 或者是AI采样节拍不相等 */
        {
            /* 若已初始化过的块，则是上次处理过的，但未满的块，但块的信息同命令不配对，则表示消息丢失过了 */
            /* This should because of msgQ queSaveRtDat_g overflow. */
            /* assert(FALSE); */
            /* Force end the old report. */
            pblk->head.uiFlag |= (RC_REC_END_FG | RC_RPT_END_FG); 			/* 故障报告停止块 */
            /* RC_REC_END_FG 录波停止标志 */ 		/* 异常时，将本块
            （上次未处理完的块，数据上次已记录部分了）作为报告结束块 */

            /* Then begin a new report from the next block. */
            pblk=RC_Wr_Next_Blk();			/* 将块写入文件中，获得下一块 */ 		/* 获得一个新的块 */
            if (pblk)
                cmd.uiFlag |= (RC_REC_BGN_FG | RC_RPT_BGN_FG);				/* 将下一块作为新的故障起始块 */
            else
                continue;					/* 等待新空出的块，丢掉本次消息,回到while处 */
        }

        assert(pblk);                   	/* End dealing with buffer overflow.前面已保证,这里不可能出现 */

        if (!(pblk->head.uiFlag & RC_HEAD_OK_FG)) 				/* 判断是不是初始化块 */
        {
            /* 若待写的块是还未初始化，则初始化该块，比如是前面处理异常时申请的 */
            /* Last block normal end. Begin with one new block. */
            RC_Init_New_Blk(pblk, &cmd, cmd.ulFirstCnt); 					/* 初始化一个新的块 */

            /* Keep begin information. End info is dealed after data saved. */
            pblk->head.uiFlag &= ~(RC_RPT_END_FG | RC_REC_END_FG);
        }
        /* 将CMD要求的时间长度的DB缓冲的录波数据记录到多个录波块中，
           对于启动命令，只有第1块有，对于停止命令，只有最后一块有 */
        /* 根据命令来写录波块 */
        for (ulDealCnt=cmd.ulFirstCnt; (int32_t)(ulDealCnt-cmd.ulNextCnt)<0; )
        {
            /* 循环，填写当前录波命令要录波的数据到多个录波块中，可能只有１个块，或多个块 */

            if(!((int32_t)(pblk->head.ulNextCnt-pblk->head.ulFirstCnt)>=0
                    &&(pblk->head.ulNextCnt-pblk->head.ulFirstCnt)<pblk->head.iDatIntvl*iPtsPerBlk_g)) 		/* 最后一项表示了最多能保存的AI节拍数 */
            {
                /* 若录波记录有异常,则结束本次录波和报告,退出小循环 */
                pblk->head.uiFlag |= (RC_REC_END_FG | RC_RPT_END_FG);
                pblk->head.unRptSN=cmd.unRptSN;
                pblk->head.ucRecSN=cmd.ucRecSN;
                break;
            }
            iSaveCnt=pblk->head.iDatIntvl*iPtsPerBlk_g-
                     (pblk->head.ulNextCnt-pblk->head.ulFirstCnt);				/* 当前块能记录的AI COUNT剩余容量 */
            if (iSaveCnt>cmd.ulNextCnt-ulDealCnt)
            {
                /* 若当前块的剩余容量大于待处理的count,则可记录到当前录波块，但该块还未满 */
                /* Data can be saved in this block. */
                iSaveCnt=cmd.ulNextCnt-ulDealCnt; 			/* 要保存的AI节拍数 */

                RC_Fill_Blk(pblk, ulDealCnt, iSaveCnt);   			/* Modified by DY 4/5/2006 */ /* 填充到该块 */
                ulDealCnt+=iSaveCnt; /* 已经处理的 */
            }
            else
            {
                /* 否则该块容量不够，还需申请新块 */
                /* The block space is not enough. */
                RC_Fill_Blk(pblk, ulDealCnt, iSaveCnt);   	/* Modified by DY 4/5/2006 */ /* 填充当前块的剩余空间，该块被添满 */
                ulDealCnt+=iSaveCnt;

                pblkNew=RC_Wr_Next_Blk();
                if (pblkNew)
                {
                    /* 若申请到新块，则初始化该块 */
                    /* Head continue with this block. */
                    RC_Init_New_Blk(pblkNew, &cmd, ulDealCnt);

                    /* The new block is normal.因为该块是同一保存任务的非第１块，则去掉命令信息，表示既非启动，也非终止块，
                       但实际上该块的头信息还是可能被后面的终止信息重写
                       这样可以保证，对录波和故障启动的第１块，启动命令在块初始化时被填入，而同一启动命令的其他块，则无此信息
                       对录波和故障终止的最后块，终止信息在最后一块被填写 */
                    pblkNew->head.uiFlag &= ~(RC_RPT_BGN_FG | RC_REC_BGN_FG |
                                              RC_RPT_END_FG | RC_REC_END_FG);

                    pblk=pblkNew;       /* Daoxu fix bug found by ZHANG Yun. */
                }
                else
                {
                    /* 若申请不到新块，则终止此次故障报告，设置被添满的块属性,跳出小循环 */
                    /* Force end this report.可能DB中的某些信息不会被记录，被丢失， */
                    pblk->head.uiFlag |= (RC_REC_END_FG | RC_RPT_END_FG);

                    pblk->head.unRptSN=cmd.unRptSN;
                    pblk->head.ucRecSN=cmd.ucRecSN;

                    break;              /* Daoxu fix bug found by ZHANG Yun. */
                }

            }		/* else结束 */
        }			/* 处理完该命令对应的数据的FOR循环 */


        if ((cmd.uiFlag & (RC_REC_END_FG | RC_RPT_END_FG)) && pblk) 			/* 停止命令 */
        {
            /* 若是录波段或故障停止命令，且最后录波块非空，则填写最后录波块的终止信息。 */
            pblk->head.uiFlag |= cmd.uiFlag;
            pblk->head.unRptSN=cmd.unRptSN;
            pblk->head.ucRecSN=cmd.ucRecSN;

            pblk=RC_Wr_Next_Blk(); /* 申请一个新的块 */
        }
    }		/* while循环结束 */
}

/***********************************************************************
* RC_Wr_Next_Blk - 写下一个录波块
*
* RETURNS: 下一个要写（指从db缓冲写到录波块）的位置, 同时释放信号量，驱动读一个录波块到报告文件中，
*
* Alert:
*		   若读录波块到报告文件中处理不过来，表示无空闲录波块可供写，则返回NULL
*
*/
static RC_DAT_BLK *RC_Wr_Next_Blk(void)
{
    STATUS vxsts;
    RC_DAT_BLK *pblk;

    vxsts=taskLock();
    assert(vxsts==OK);

    /* 包含数据的块数, 同时允许分配
     * 首先满足块数要求, 然后调用是否允许分配命令, 同时进行更新
     * 暂不执行RC_Jg_Allot_Blk
     */
    if (iUsedBlks_g<iBlkNum_g) 			/* 包含数据的块数 */
    {
        /* 若已写到缓冲，但未读到文件中的录波块数，还未溢出，表示还有空闲录波块 */
        iUsedBlks_g++;		/* 待写到录波文件中的录波块数 */

#if 0
        /* 缓冲区满, 则后续不能分配录波块 */
        if (iUsedBlks_g == iBlkNum_g)
        {
            static BOOL bWrLogFlag = FALSE;

            if (!bWrLogFlag)
            {
                bWrLogFlag = TRUE;
                LOG_Write(LOG_KERNEL, "录波块缓冲满.\n", NULL);
            }
            rptsts_g.bAllocBlk = FALSE;
        }
#endif
        iBlkWrPos_g++;			/* 写位置加1 */ 		/* 递推提供另外一个录波块 */
        if (iBlkWrPos_g >= iBlkNum_g)
        {
            iBlkWrPos_g=0;
            bRecBlkRewind = TRUE;  /* 录波块回卷 */
        }

        pblk=ppdatblk_g[iBlkWrPos_g]; 		/* 提供一个数据块 */

        pblk->head.uiFlag &= ~RC_HEAD_OK_FG;				/* 表示重新使用前，还未初始化 */

        vxsts=semGive(semMkRecFile_g);		/* 释放可录波信号 */
        assert(vxsts==OK);
    }
    else
    {
        static BOOL bWrLogFlag = FALSE;

        if (!bWrLogFlag)
        {
            bWrLogFlag = TRUE;
            LOG_Write(LOG_KERNEL, "CPU负荷过重, tRecFile任务不能及时响应, 丢掉当前录波块.\n", NULL);
        }

        pblk=NULL;			/* 否则读处理不过来，则返回NULL，表示无录波块可写 */
    }
    vxsts=taskUnlock();
    assert(vxsts==OK);

    return pblk;
}

/***********************************************************************
* RC_Rd_Next_Blk - 读取下一个录波块
*
* RETURNS: 当前应读到录波文件中的录波块指针
*
*/
static RC_DAT_BLK *RC_Rd_Next_Blk(void)
{
    STATUS vxsts;
    RC_DAT_BLK *pblk;

    vxsts=taskLock();
    assert(vxsts==OK);

    assert(iUsedBlks_g>0);

    iUsedBlks_g--;		/* 未处理的录波块数目减1 */

    iBlkRdPos_g++;				/* 当前待写到文件中的录波块的位置 */
    if (iBlkRdPos_g >= iBlkNum_g)
        iBlkRdPos_g=0;

    pblk=ppdatblk_g[iBlkRdPos_g];			/* 返回待读出的指针 */

    vxsts=taskUnlock();
    assert(vxsts==OK);

    return pblk;
}

/* 判断是否允许分配录波块.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RC_Jg_Allot_Blk(void)
{
    /* 占用缓冲块已经读取结束, 同时文件形成结束
     */
    if ((iUsedBlks_g == 0) && (rptsts_g.bRecWrFileOn == FALSE))
    {
        rptsts_g.bAllocBlk = TRUE;
    }

    return rptsts_g.bAllocBlk;
}

/***********************************************************************
* RC_Init_New_Blk - 初始化一个新的录波块
*
* RETURNS: 无
*
*/
static void RC_Init_New_Blk(
    RC_DAT_BLK *pblk,
    RC_BUF_CMD *pcmd,
    uint32_t ulInitCnt
)
{
    pblk->head.uiFlag=pcmd->uiFlag | RC_HEAD_OK_FG;			/* 表示是一个新的录波块 */
    pblk->head.iDatIntvl=pcmd->iDatIntvl; 			/* 录波间隔 */
    pblk->head.ulFirstCnt=ulInitCnt; 									/* 录波开始节拍 */
    pblk->head.ulNextCnt=ulInitCnt;     		/* New block, no data. */
    pblk->head.ulBgnCnt=pcmd->ulBgnCnt;
    pblk->head.ulRecRefUs = pcmd->ulRecRefUs;
    pblk->head.unRptSN=pcmd->unRptSN;
    pblk->head.ucRecSN=pcmd->ucRecSN;
}

/* 清零录波块
 * Para:
 *     pblk, 录波块.
 *     ulAiCnt, 开始清零节拍.
 *     iSaveCnt, 清零点数.
 * Return:
 *     NONE.
 */
static void RC_Clear_Blk(RC_DAT_BLK *pblk, uint32_t ulAiCnt, int iSaveCnt)
{
    int iStep = 0;
    int i;
    uint64_t *pullPtsBgn = NULL;
    uint32_t *pulWork = NULL;
    BOOL *pbWork;

    /* 无有效填充点,录波块没有反转,录波块为空,或当前录波点小于录波块初始点
     * 非零时至少处理1个点
    */
    if ((iSaveCnt <= 0)
            || (!bRecBlkRewind)
            || (!pblk)
            || ((int32_t)(ulAiCnt-pblk->head.ulFirstCnt)<0)
       )
    {
        return;
    }

    /* 确定录波块中写的开始字节位置
     */
    iStep = pblk->head.iDatIntvl;
    pullPtsBgn = pblk->aullDat+(ulAiCnt-pblk->head.ulFirstCnt)/iStep*iDatPerPts_g;

    while (1)
    {
        /* 存放地址 */
        pulWork = (uint32_t *)pullPtsBgn;

        /* 清零模拟量 ,2013-7-20 修改数据写越界缺陷，ZY*/
        for (i = 0; i<iRunRecAi_g; i++)
        {
            *pulWork++ = FloatZero_g.ulVal;
            *pulWork++ = FloatZero_g.ulVal;
        }

        /* 清零DI  ,2013-7-20 修改数据写越界缺陷，ZY*/
        pbWork = (BOOL *)pulWork;
        for (i = 0; i<iRunRecDi_g; i++)
        {
            *pbWork++ = 0;
        }

        /* 清零DOBUF中的信息,仅填写一个0时间信息
         */
        pulWork = (uint32_t *)pbWork;
        *pulWork = 0;

        if ((iSaveCnt -= iStep) <= 0) /* Effectively finish the cycle. */
        {
            break;
        }

        pullPtsBgn += iDatPerPts_g;  /* 一个点所保存的数据个数,以uint64_t为单位 */
    }
}

/* 填写录波块
 * Para:
 *     pblk, 录波块.
 *     ulAiCnt, 录波开始节拍.
 *     iSaveCnt, 录波点数.
 * Return:
 *     NONE.
 */
static void RC_Fill_Blk(RC_DAT_BLK *pblk, uint32_t ulAiCnt, int iSaveCnt)
{
    BOOL *pbWork;
    int iStep;
    uint64_t *pullPtsBgn;
    COMPLEX *pxCalcAi;
    float *pfLgcAi;
    uint32_t *pulWork;
    uint32_t *pulSrc;
    RC_AI_DB *praidb;
    RC_DI_DB *prdidb;
    RC_DB_INFO *pdbinf;
    RC_DB_INFO **ppdbinfRec;
    int i;
    int32_t ntmp;
    BOOL bOptChIsComNormal[2] = {FALSE, FALSE};    /* 光差通道是否通信正常 */
    uint32_t  ulOptAIVldCnt;
    void   *  pvAiMod;
    uint32_t  ulDiff;/*2013-7-20 ZY */


    ntmp =(int32_t)iSaveCnt;

    //assert (pblk && (int32_t)(ulAiCnt-pblk->head.ulFirstCnt) >= 0 && iSaveCnt > 0);

    /*2013-7-20  ZY修改，防止数据写越界，实际不会发生 */
    //assert (pblk && (int32_t)(ulAiCnt-pblk->head.ulFirstCnt) >= 0 && iSaveCnt > 0);
    if(!(pblk && (int32_t)(ulAiCnt-pblk->head.ulFirstCnt) >= 0 && iSaveCnt > 0))
    {
        //非法或前向越界
        return  ;
    }

    ulDiff=(pblk->head.ulFirstCnt+(uint32_t)(pblk->head.iDatIntvl*iPtsPerBlk_g))-
           (ulAiCnt+(uint32_t)iSaveCnt);
    if(((int32_t)ulDiff)<0)
    {
        //后向越界
        return ;
    }

    iStep=pblk->head.iDatIntvl;			/* 录波间隔 */
    pullPtsBgn=pblk->aullDat+(ulAiCnt-pblk->head.ulFirstCnt)/iStep*iDatPerPts_g;			/* 确定录波块中写的开始字节位置 */

    pblk->head.ulNextCnt = ulAiCnt+iSaveCnt;  /* 录波块中的下次写的位置 */

    /* 若待录波的数据,还没有到(处理光差数据滞后问题),则最多延迟40毫秒
     * 超过40ms实际上录波会出错
     */
    for(i=0; i<4; i++)
    {
        if ((int)(RD_GetAllAIValidCnt()-(pblk->head.ulNextCnt-1)) >= lDelayPstNum)
        {
            /* 待录波数据已到 */
            break; /* 进入正常处理 */
        }
        taskDelay(1);
    }

    /* 根据节拍获得地址
     * 预处理
     */
    pxCalcAi = RD_Base_Calc_AI_P(ulAiCnt);
    if(!pxCalcAi)
    {
        /* 可能因CPU忙,导致数据失效,则空操作 */
        static uint32_t ulCnt=0;
        static BOOL bWrLogFlag = FALSE;

        if (!bWrLogFlag)
        {
            bWrLogFlag = TRUE;
            LOG_Write(LOG_KERNEL, "数据窗已滑出(1).\n", NULL);
        }

        if((ulCnt&0x7FF) == 0)
        {
            LOG_Dbg_Msg("ERROR: Getting the address saving the proprocessing result is OverTime when Rec Fill Blk.\n", 0, 0, 0, 0, 0, 0);
        }
        ulCnt++;

        /* 录波块翻转时应进行录波数据清零 */
        RC_Clear_Blk(pblk, ulAiCnt, iSaveCnt);

        return;
    }

    /* 根据节拍获得地址
     * 瞬时值
     */
    pfLgcAi = RD_Base_Lgc_AI_P(ulAiCnt);
    if(!pfLgcAi)
    {
        static uint32_t ulCnt=0;
        static BOOL bWrLogFlag = FALSE;

        if (!bWrLogFlag)
        {
            bWrLogFlag = TRUE;
            LOG_Write(LOG_KERNEL, "数据窗已滑出(2).\n", NULL);
        }

        if((ulCnt&0x7FF) == 0)
        {
            LOG_Dbg_Msg("ERROR: Getting the address saving the realtime result is OverTime when Rec Fill Blk.\n", 0, 0, 0, 0, 0, 0);
        }
        ulCnt++;

        /* 录波块翻转时应进行录波数据清零 */
        RC_Clear_Blk(pblk, ulAiCnt, iSaveCnt);

        return;
    }

    /* 判定光差通道数据当前通信是否正常 */
    bOptChIsComNormal[0]=OPT_ChIsComNormal(0,&ulOptAIVldCnt);
    bOptChIsComNormal[1]=OPT_ChIsComNormal(1,&ulOptAIVldCnt);

    /* 保存iSaveCnt AI时间到录波块中 */
    while (1)
    {
        BOOL bDbinfoValid=TRUE;
        ppdbinfRec=apdbinfRec_g;
        while ((pdbinf=*ppdbinfRec++) != NULL) 					/* 所有的录波运行任务 */
        {
            /* 确定不同任务的中间结果dbBUF中的最接近的读位置 */

            i=ulAiCnt-pdbinf->ulBgnCnt;
            if (i<0) /* 若当前的点比DB缓冲的当前起点还早,则需要将该点定位为以前保存的点 */
                i = i+pdbinf->iBufLen;

            if(!(i>=0 && i<pdbinf->iBufLen))				/* 初始化时,或得不到运行时,这里可能溢出,数据失效 */
            {
                static uint32_t ulCnt=0;
                static BOOL bWrLogFlag = FALSE;

                if (!bWrLogFlag)
                {
                    bWrLogFlag = TRUE;
                    LOG_Write(LOG_KERNEL, "数据窗已滑出(3).\n", NULL);
                }

                if(ulCnt%2000 == 0)
                {
                    LOG_Dbg_Msg("ERROR: Get logicScan Task Mid Result DB info is OverTime when Rec Fill Blk.\n", 0, 0, 0, 0, 0, 0);
                }
                ulCnt++;
                bDbinfoValid=FALSE;

                /* 本点中间结果不再继续处理 */

                break;
            }
            pdbinf->iRdOfst = i;	/* dbBUF中的读出位置,从零开始 */
        }

        if(!bDbinfoValid)
        {
            /* 录波块翻转时应进行剩余录波数据清零 */
            RC_Clear_Blk(pblk, ulAiCnt, iSaveCnt);

            break;
        }

        /* 填写模拟量 */

        pulWork=(uint32_t*)pullPtsBgn; 		/* 存放的地址 */
        for (praidb=praidb_g; praidb<praidb_g+iRecAiNum_g; praidb++)
        {
            /* 对每个录波所有AI录波量进行 */
            if (praidb->pelmSrc) 			/* 表明本配置用于保存中间结果 */
            {
                /* 若是中间结果录波量,则从DBBUF中倒到录波块中 */
                pdbinf=(RC_DB_INFO*)praidb->pvHisHnd; 		/* 中间结果保存地址 */

                pulSrc=(uint32_t*)(praidb->pxBuf+pdbinf->iRdOfst);
                *pulWork++=*pulSrc++;			/* 写前4个字节 */
                *pulWork++=*pulSrc;						/* 写后4个字节 */
            }
            else if (praidb->pvHisHnd)
            {
                pvAiMod=(void  *)(((RD_LGC_AI_CH *)(praidb->pvHisHnd))->phwai->paimod);
                if (praidb->bIsReal)
                {
                    /* 否则若是REAL AI原始通道 */
                    if(((pvAiMod==&(aimodOpt_g[0]))&&(!bOptChIsComNormal[0]))
                            ||((pvAiMod==&(aimodOpt_g[1]))&&(!bOptChIsComNormal[1])))
                    {
                        /* 若是光差AI通道,且通信异常,则添浮点零 */
                        *pulWork++ = FloatZero_g.ulVal;
                    }
                    else
                    {
                        *pulWork++ = *(uint32_t *)(pfLgcAi+praidb->iChOfst);  /* 为了避免保存浮点寄存器,不用浮点,直接用整型数保存 */
                    }
                    pulWork++;
                }
                else
                {
                    /* 否则若是COMPLEX  AI原始通道 */
                    if(((pvAiMod==&(aimodOpt_g[0]))&&(!bOptChIsComNormal[0]))
                            ||((pvAiMod==&(aimodOpt_g[1]))&&(!bOptChIsComNormal[1])))
                    {
                        /* 若是光差AI通道,且通信异常,则添浮点零 */
                        *pulWork++=FloatZero_g.ulVal;
                        *pulWork++=FloatZero_g.ulVal;
                    }
                    else
                    {
                        pulSrc=(uint32_t*)(pxCalcAi+praidb->iChOfst);
                        *pulWork++=*pulSrc++;
                        *pulWork++=*pulSrc;
                    }
                }
            }
        }

        /* 填写DI */

        pbWork=(BOOL*)pulWork;
        for (prdidb=prdidb_g; prdidb<prdidb_g+iRecDiNum_g; prdidb++)
        {
            /*对所有DI录波变量进行记录  */
            if (prdidb->pelmSrc)
            {
                /* 若是中间结果DI录波量 */
                pdbinf=(RC_DB_INFO*)prdidb->pvHisHnd;

#if 0   /* 根据应用需求，不再使用延时功能 */
                if (prdidb->bDelay)
                {
                    *pbWork++ = prdidb->pbBuf[(pdbinf->iRdOfst+lDelayPstNum)%ulRecBufPtsNum];
                }
                else
#endif
                {
                    *pbWork++=prdidb->pbBuf[pdbinf->iRdOfst];
                }
            }
            else if (prdidb->pvHisHnd)			/* 否则是原始DI通道 */
            {
                *pbWork++=RD_His_DI(prdidb->pvHisHnd, ulAiCnt); 					/* 获得原始通道的DI值 */
            }
        }

        /* 记录DOBUF中的信息,仅填写一个0时间信息
         */
        pulWork = (uint32_t *)pbWork;
        *pulWork=0;


        if ((iSaveCnt -= iStep) <= 0) /* Effectively finish the cycle. */
        {
            break;
        }

        ulAiCnt += iStep; 		/* 保存步长度,跟随采样节拍 */
        pullPtsBgn += iDatPerPts_g;  /* 一个点所保存的数据个数,以uint64_t为单位 */

        /* 必须用节拍重新获取采样指针,直接推移地址有被覆盖的可能
         * 瞬时值
         */
        pfLgcAi = RD_Base_Lgc_AI_P(ulAiCnt);
        if (!pfLgcAi)
        {
            static uint32_t ulCnt=0;
            static BOOL bWrLogFlag = FALSE;

            if (!bWrLogFlag)
            {
                bWrLogFlag = TRUE;
                LOG_Write(LOG_KERNEL, "数据窗已滑出(4).\n", NULL);
            }

            if ((ulCnt & 0x7FF) == 0)
            {
                LOG_Dbg_Msg("ERROR: Getting the address saving the realtime result is OverTime when Rec Fill Blk.\n", 0, 0, 0, 0, 0, 0);
            }
            ulCnt++;

            /* 录波块翻转时应进行录波数据清零 */
            RC_Clear_Blk(pblk, ulAiCnt, iSaveCnt);

            break;
        }

        /* 必须用节拍重新获取采样指针,直接推移地址有被覆盖的可能
         * 预处理
         */
        pxCalcAi = RD_Base_Calc_AI_P(ulAiCnt);
        if (!pxCalcAi)
        {
            /* 可能因CPU忙,导致数据失效,则空操作 */
            static uint32_t ulCnt=0;
            static BOOL bWrLogFlag = FALSE;

            if (!bWrLogFlag)
            {
                bWrLogFlag = TRUE;
                LOG_Write(LOG_KERNEL, "数据窗已滑出(5).\n", NULL);
            }

            if ((ulCnt & 0x7FF) == 0)
            {
                LOG_Dbg_Msg("ERROR: Getting the address saving the proprocessing result is OverTime when Rec Fill Blk.\n", 0, 0, 0, 0, 0, 0);
            }
            ulCnt++;

            /* 录波块翻转时应进行录波数据清零 */
            RC_Clear_Blk(pblk, ulAiCnt, iSaveCnt);

            break;
        }
    }
}

/***********************************************************************
* RC_Make_File - 形成录波文件任务
*
* RETURNS:
*   文件描述符
*
* Alert:
*   假设在用报告号标志的同一个录波文件周期内,
*   文件开始、段开始、纯数据、段结束、文件结束按先后顺序到达
*
*/
static int RC_Make_File(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    STATUS vxsts;
    RC_DAT_BLK *pblk;
    int iFd;
    int iPieceNum;  /* 录波段数 */
    uint32_t ulPiecePts;  /* 单个录波段录波数据点数 */
    int32_t lPieceBgn;   /* 单个录波段在录波文件的起始位置 */
    uint32_t ulMsgSz;  /* 录波文件中录波信息总长度 */
    uint32_t ulWrLen;       /* 写入文件数据长度 */
    uint32_t ulWrPiecePts;      /* 写入录波数据点数 */
    EP_STATUS StsWr;
    uint16_t unLstRptSN = 0;  /* the last SN of the report. */
    uint8_t ucLstRecSN = 0;     /* the last SN of the record. */
    uint32_t ulLstNextCnt = 0;		/* the next point of the last point of this record piece. */
    BOOL bNewFileFlag = TRUE;    /* flag for new file. */
    BOOL bRecPicecOverFlag = TRUE;     /* if the last record piece is over. */
    BOOL bRecFileOverFlag = TRUE;   /* if the last record file is over. */

    iFd=-1;
    iPieceNum=0;
    ulPiecePts=0;
    lPieceBgn=0;
    ulMsgSz=0;

    while (1)
    {
#if defined(EDP_01_02_BUILD)
        if (GetAdjustTimeSuccessFlag()) 				/* Must get the time adjustment flag, otherwise do not record the event. */
#elif defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        if (TRUE)			/* adjust the time in local CPU, so record the event directly. */
#endif
        {
            break;
        }

        /* waiting for 1 second. */
        taskDelay(SYS_SEC);
    }

    for (pblk=ppdatblk_g[0]; TRUE; pblk=RC_Rd_Next_Blk())
    {
        vxsts=semTake(semMkRecFile_g, WAIT_FOREVER);		/* 若缓冲区中有一个新的录波块写满，则将当前待读的录波块写到到录波文件 */
        assert(vxsts==OK);

        /* 包括以下几种情况
         *
         * 第一次写录波文件或文件读写出错
         * 报告号不一致
         * 录波段号不一致
         * 录波段内不连续
         */

        if (bNewFileFlag)
        {
            /* 第一次写录波文件或文件读写出错, 需新建文件，如有临时文件后面将删除 */
            bNewFileFlag = FALSE;

            /* 新建文件,必须预先生成文件头
             * 对于录波段头,如果当前录波块包含数据但无录波段头标志,则需添加
             * 如果只传递录波文件结束,无数据,亦添加,但多一个录波段
             */
            pblk->head.uiFlag |= RC_RPT_BGN_FG | RC_REC_BGN_FG;

            /* 对于新文件,录波文件结束,则录波段亦结束
             * 否则会出现录波段不能结束的现象
             */

            if (pblk->head.uiFlag & RC_RPT_END_FG)
            {
                pblk->head.uiFlag |= RC_REC_END_FG;
            }
        }
        else if ((unLstRptSN != pblk->head.unRptSN)
                 && (pblk->head.uiFlag & RC_RPT_BGN_FG))
        {
            /* 报告号不同,同时需新建一个录波文件 */
            /* 上一个录波文件没有正常结束 */
            if (!bRecFileOverFlag)
            {
                /* 作为异常情况删除文件 */
                RC_MakeFileTaskExecHandle(iFd, 0);
                lPieceBgn = 0;
                iFd = -1;
                iPieceNum = 0;
                ulPiecePts = 0;
                ulMsgSz = 0;
                bRecPicecOverFlag = TRUE;
                bRecFileOverFlag = TRUE;
            }

            /* 以下与新文件处理一致 */
            pblk->head.uiFlag |= RC_RPT_BGN_FG | RC_REC_BGN_FG;

            if (pblk->head.uiFlag & RC_RPT_END_FG)
            {
                pblk->head.uiFlag |= RC_REC_END_FG;
            }
        }
        else if ((ucLstRecSN != pblk->head.ucRecSN)
                 || (unLstRptSN != pblk->head.unRptSN))
        {
            /* 录波段号不一致,说明需新建录波段,但还在同一个文件中
             * 或者报告号不同,但无需新建录波文件
             */

            /* 上一个录波段没有正常结束 */
            if (!bRecPicecOverFlag)
            {
                StsWr = RC_End_Piece(iFd, lPieceBgn, ulPiecePts);
                if (StsWr == EP_SUCCESS)
                {
                    ulMsgSz += ulPiecePts*iPtsBytes_g;
                    iPieceNum++;	/* 段号加1 */
                    lPieceBgn = 0;
                    ulPiecePts = 0;
                    bRecPicecOverFlag = TRUE;
                }
                else
                {
                    /* 若未写成功, 则整个录波报告文件都认为无效, 等待下一个故障报告重新开始 */
                    RC_MakeFileTaskExecHandle(iFd, 4);
                    iFd = -1;
                    lPieceBgn = 0;
                    iPieceNum = 0;
                    ulPiecePts = 0;
                    ulMsgSz = 0;
                    bRecPicecOverFlag = TRUE;
                    bRecFileOverFlag = TRUE;
                    /* 建立新文件 */
                    pblk->head.uiFlag |= RC_RPT_BGN_FG;
                }
            }

            /* 如果当前录波块包含数据但无录波段头标志,则需添加
             * 如果只传递录波段结束,无数据,亦添加,但多一个录波段
             */
            pblk->head.uiFlag |= RC_REC_BGN_FG;

            /* 录波文件结束,则录波段亦结束
             * 由于还在同一文件,则假设该录波块不包含文件开始标志
             */
            if (pblk->head.uiFlag & RC_RPT_END_FG)
            {
                pblk->head.uiFlag |= RC_REC_END_FG;
            }
        }
        else if (ulLstNextCnt != pblk->head.ulFirstCnt)
        {
            /* 录波点号不连续,说明录波段中丢失录波块,需新建录波段
             * 同时直接结束上一录波段
             */
            StsWr = RC_End_Piece(iFd, lPieceBgn, ulPiecePts);
            if (StsWr == EP_SUCCESS)
            {
                ulMsgSz += ulPiecePts*iPtsBytes_g;
                iPieceNum++;	/* 段号加1 */
                lPieceBgn = 0;
                ulPiecePts = 0;
                bRecPicecOverFlag = TRUE;
            }
            else
            {
                /* 若未写成功, 则整个录波报告文件都认为无效, 等待下一个故障报告重新开始 */
                RC_MakeFileTaskExecHandle(iFd, 4);
                iFd = -1;
                lPieceBgn = 0;
                iPieceNum = 0;
                ulPiecePts = 0;
                ulMsgSz = 0;
                bRecPicecOverFlag = TRUE;
                bRecFileOverFlag = TRUE;
                /* 建立新文件 */
                pblk->head.uiFlag |= RC_RPT_BGN_FG;
            }

            /* 以下与录波段号不同一致 */
            pblk->head.uiFlag |= RC_REC_BGN_FG;

            if (pblk->head.uiFlag & RC_RPT_END_FG)
            {
                pblk->head.uiFlag |= RC_REC_END_FG;
            }
        }
        else
        {
            /* 连续的录波 */

            /* 保持录波命令的开放性,对后向的命令不作限制
             * 但如果文件结束命令已经到达
             * 则必须考查段命令是否结束
             */
            if (pblk->head.uiFlag & RC_RPT_END_FG)
            {
                /* 录波文件结束 */
                if (!bRecPicecOverFlag)
                {
                    /* 前一录波段没有正常结束 */
                    pblk->head.uiFlag |= RC_REC_END_FG;
                }
            }
        }

        unLstRptSN = pblk->head.unRptSN;
        ucLstRecSN = pblk->head.ucRecSN;
        ulLstNextCnt = pblk->head.ulNextCnt;

        /* 同一录波块，可能既有报告开始，也有报告结束，且若报告和录波的启动和结束不配对，记录错误，则文件格式可能会错误 */
        if (pblk->head.uiFlag & RC_RPT_BGN_FG)
        {
            /* 若是新形成的报告的录波块启动 */
            if(!(iFd<0))          	/* Last file must be finished. */
            {
                /* 若原来的文件还未结束,则处理之 */
                RC_MakeFileTaskExecHandle(iFd, 0);
            }
            lPieceBgn = 0;
            iFd=-1;
            iPieceNum=0;
            ulPiecePts=0;
            ulMsgSz=0;
            iFd=RC_New_File(pblk, &ulMsgSz);					/* 新创建文件，并写故障报告文件的文件头 */
            bRecFileOverFlag = FALSE;
        }

        if (iFd<0)
        {
            /* 若无文件，则可能因为录波块中，报告开始和结束没有完全配对，这是可能的，
            	 因为录波可能比较忙,导致命令丢失,从而录波块丢失，导致记录不全，则空操作，等待新的录波文件命令 */
            /* This rec file has not been created.  One report rec data
               is lost(untill next RC_PRT_BGN_FG). */
            static uint32_t ulCnt=0;
            if(ulCnt%2000==0)
            {
                LOG_Dbg_Msg("ERROR: One report rec data is lost.\n", 0, 0, 0, 0, 0, 0);
            }
            ulCnt++;
            bNewFileFlag = TRUE; /* 处理录波块时必定有录波文件打开 */
            continue;
        }

        /************处理录波段开始命令**********/
        if (pblk->head.uiFlag & RC_REC_BGN_FG)
        {
            /* 若是录波段的启动，则开始新一段录波 */

            /*比如当前一段结束消息丢失或前一段结束命令和新段开始命令在同一块时，就出现该情况 */
            if(!bRecPicecOverFlag)
            {
                /*若前一录波段未结束，则补偿段结束  2013-7-20 ZY 添加*/

                StsWr = RC_End_Piece(iFd, lPieceBgn, ulPiecePts);
                if (StsWr == EP_SUCCESS)
                {
                    ulMsgSz += ulPiecePts*iPtsBytes_g;
                    iPieceNum++;	/* 段号加1 */
                    lPieceBgn = 0;
                    ulPiecePts = 0;
                    bRecPicecOverFlag = TRUE;
                }
                else
                {
                    /* 若未写成功, 则整个录波报告文件都认为无效, 等待下一个故障报告重新开始 */
                    RC_MakeFileTaskExecHandle(iFd, 4);
                    iFd = -1;
                    lPieceBgn = 0;
                    iPieceNum = 0;
                    ulPiecePts = 0;
                    ulMsgSz = 0;
                    bNewFileFlag = TRUE;
                    bRecPicecOverFlag = TRUE;
                    bRecFileOverFlag = TRUE;
                    continue;
                }
            }

            //开始新录波段
            ulPiecePts=0;
            lPieceBgn=lseek(iFd, 0, SEEK_CUR);
            StsWr=RC_New_Piece(iFd, pblk,&ulWrLen);			/* 写新录波段的段头  张云修改 */
            bRecPicecOverFlag = FALSE;
            if(StsWr==EP_SUCCESS)
            {
                ulMsgSz=ulMsgSz+ulWrLen;
            }
            else
            {
                /* 若未写成功,则整个录波报告文件都认为无效,等待下一个故障报告重新开始 */
                RC_MakeFileTaskExecHandle(iFd, 2);
                iFd=-1;
                lPieceBgn = 0;
                iPieceNum = 0;
                ulPiecePts = 0;
                ulMsgSz = 0;
                bNewFileFlag = TRUE;
                bRecPicecOverFlag = TRUE;
                bRecFileOverFlag = TRUE;
                continue;
            }
        }

        /************写入当前录波块数据**********/

        if(bRecPicecOverFlag)
        {
            /*若前一录波段结束，则补偿新段开始 2013-7-20  ZY*/

            ulPiecePts=0;
            lPieceBgn=lseek(iFd, 0, SEEK_CUR);

            StsWr=RC_New_Piece(iFd, pblk,&ulWrLen);			/* 写新录波段的段头  张云修改 */
            bRecPicecOverFlag = FALSE;
            if(StsWr==EP_SUCCESS)
            {
                ulMsgSz=ulMsgSz+ulWrLen;
            }
            else
            {
                /* 若未写成功,则整个录波报告文件都认为无效,等待下一个故障报告重新开始 */
                RC_MakeFileTaskExecHandle(iFd, 2);
                iFd=-1;
                lPieceBgn = 0;
                iPieceNum = 0;
                ulPiecePts = 0;
                ulMsgSz = 0;
                bNewFileFlag = TRUE;
                bRecPicecOverFlag = TRUE;
                bRecFileOverFlag = TRUE;
                continue;
            }
        }

        StsWr=RC_Wr_Rec_Dat(iFd, pblk,&ulWrPiecePts, &uWrRecDataErrSts);			/* 填写该录波块的内容 张云修改 */

        if(StsWr==EP_SUCCESS)
        {
            ulPiecePts=ulPiecePts+ulWrPiecePts;
        }
        else
        {
            /* 若未写成功,则整个录波报告文件都认为无效,等待下一个故障报告重新开始 */

            RC_MakeFileTaskExecHandle(iFd, 3);
            iFd=-1;
            lPieceBgn = 0;
            iPieceNum = 0;
            ulPiecePts = 0;
            ulMsgSz = 0;
            bNewFileFlag = TRUE;
            bRecPicecOverFlag = TRUE;
            bRecFileOverFlag = TRUE;
            continue;
        }

        if (pblk->head.uiFlag & RC_REC_END_FG)
        {
            /* 若是录波段结束，则结束该录波段    张云修改 */
            if(bRecPicecOverFlag)
            {
                /*若前一录波段结束，则补偿新段开始 2013-7-20  ZY*/

                ulPiecePts=0;
                lPieceBgn=lseek(iFd, 0, SEEK_CUR);

                StsWr=RC_New_Piece(iFd, pblk,&ulWrLen);			/* 写新录波段的段头  张云修改 */
                bRecPicecOverFlag = FALSE;
                if(StsWr==EP_SUCCESS)
                {
                    ulMsgSz=ulMsgSz+ulWrLen;
                }
                else
                {
                    /* 若未写成功,则整个录波报告文件都认为无效,等待下一个故障报告重新开始 */
                    RC_MakeFileTaskExecHandle(iFd, 2);
                    iFd=-1;
                    lPieceBgn = 0;
                    iPieceNum = 0;
                    ulPiecePts = 0;
                    ulMsgSz = 0;
                    bNewFileFlag = TRUE;
                    bRecPicecOverFlag = TRUE;
                    bRecFileOverFlag = TRUE;
                    continue;
                }
            }
            //录波段结束
            StsWr=RC_End_Piece(iFd, lPieceBgn, ulPiecePts);
            if(StsWr==EP_SUCCESS)
            {
                ulMsgSz+=ulPiecePts*iPtsBytes_g;
                iPieceNum++;				/* 段号加1 */
                lPieceBgn = 0;
                ulPiecePts = 0;
                bRecPicecOverFlag = TRUE;
            }
            else
            {
                /* 若未写成功,则整个录波报告文件都认为无效,等待下一个故障报告重新开始 */

                RC_MakeFileTaskExecHandle(iFd, 4);
                iFd=-1;
                lPieceBgn = 0;
                iPieceNum = 0;
                ulPiecePts = 0;
                ulMsgSz = 0;
                bNewFileFlag = TRUE;
                bRecPicecOverFlag = TRUE;
                bRecFileOverFlag = TRUE;
                continue;
            }
        }

        if (pblk->head.uiFlag & RC_RPT_END_FG)
        {
            /* 若是报告结束，则结束报告    张云修改 */
            RC_End_File(iFd, pblk, ulMsgSz, iPieceNum);			/* 这里形成正常的录波文件，ulMsgSz为总的数据长度，iPieceNum为段数 */

            iFd=-1;
            lPieceBgn = 0;
            iPieceNum = 0;
            ulPiecePts = 0;
            ulMsgSz = 0;
            bRecFileOverFlag = TRUE;
        }
    }
}

/***********************************************************************
* RC_New_File - 创建新的故障报告文件，并写文件头和录波头  *pulMsgSz，录波头的长度 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: 文件描述符
*
*/
static int RC_New_File(
    RC_DAT_BLK *pblk,
    uint32_t *pulMsgSz
)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    int iFd;
    int i;
    u_int uiTemp;
    EP_DATE_TIME dttm;
    EP_STATUS sts;

    assert(pblk && pulMsgSz);

    g_ulRecInter = 1;

    rptsts_g.bRecWrFileOn=TRUE;		/* 开始写文件 */

    if (FT_Is_File(EP_WAVE_REC_DIR "/edptmp.frw.bak"))
    {
        remove(EP_WAVE_REC_DIR "/edptmp.frw.bak");
    }

    iFd=creatInDataDisk(EP_WAVE_REC_DIR "/edptmp.frw.bak", O_RDWR);

    /* assert(iFd!=ERROR); */
    /* 张云添加 */
    if(iFd==ERROR)
    {
        static uint32_t ulInfoCnt=0;
        ulInfoCnt++;
        if (FT_Is_File(EP_WAVE_REC_DIR "/edptmp.frw.bak"))
        {
            remove(EP_WAVE_REC_DIR "/edptmp.frw.bak");
        }
        if(ulInfoCnt%10==1)
        {
            logMsg("WARNING,Create  New  Record  File  failure  for  busy  or  error!\n", 0, 0, 0, 0, 0, 0);
            if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "WARNING,Create New  Record  File  failure  for  busy  or  error!\n", NULL);
            else if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "告警，创建新的录波文件失败\n", NULL);
        }
        RC_MakeFileTaskExecHandle(iFd, 1); 	/* 可能会因为磁盘空间不够,而导致创建不成功 ,需要删除老文件 */
        return  ERROR;
    }


    memset(aucBuf, 0, sizeof(aucBuf));
    /* 写文件头 */
    aucBuf[0]=0xB1;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x4E;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());
    aucBuf[6]=0x07;
    aucBuf[7]=LO8(VER_ExtGetSwCfgCRC());
    aucBuf[8]=HI8(VER_ExtGetSwCfgCRC());
    aucBuf[9]=LO8(pblk->head.unRptSN);
    aucBuf[10]=HI8(pblk->head.unRptSN);

    /* sts=TM_To_Dttm(RD_AI_Cnt_To_us(pblk->head.ulBgnCnt), &dttm); */
    sts = TM_To_Dttm(pblk->head.ulRecRefUs, &dttm);
    if (sts!=EP_SUCCESS)
        dttm.ucHour |= 0x80;
    SYN_CpyDttm(&g_tRecBeginTm, &dttm);
    g_ulRecBeginTmUs = pblk->head.ulRecRefUs;
    /*是否正闰秒的60s时刻*/
    if((dttm.ucIrigbLSFlag & IRIGB_PLS_60SEC) == IRIGB_PLS_60SEC)
    {
        aucBuf[6] |= REC_IRIGB_PLS_60SEC_FLAG;
    }
    /*该时刻是否正闰秒的0秒后的时间*/
    if((dttm.ucIrigbLSFlag & IRIGB_AFTER_PLS_0SEC) == IRIGB_AFTER_PLS_0SEC)
    {
        aucBuf[6] |= REC_IRIGB_AFTER_PLS_0SEC;
    }
    /*该时刻是否负闰秒的0秒后的时间*/
    if((dttm.ucIrigbLSFlag & IRIGB_AFTER_NLS_0SEC) == IRIGB_AFTER_NLS_0SEC)
    {
        aucBuf[6] |= REC_IRIGB_AFTER_NLS_0SEC;
    }
    /*该时刻时间是否进行了调整*/
    if(((dttm.ucIrigbLSFlag & IRIGB_NLS_TIME_ADJUST) == IRIGB_NLS_TIME_ADJUST)
            || ((dttm.ucIrigbLSFlag & IRIGB_PLS_TIME_ADJUST) == IRIGB_PLS_TIME_ADJUST))
    {
        aucBuf[6] |= REC_IRIGB_LS_TIME_ADJUST;
    }

    /* 写时间 */
    aucBuf[11]=LO8(dttm.unYear);
    aucBuf[12]=HI8(dttm.unYear);
    aucBuf[13]=dttm.ucMonth;
    aucBuf[14]=dttm.ucDate;
    aucBuf[15]=dttm.ucHour;
    aucBuf[16]=dttm.ucMinute;

    uiTemp=dttm.ucSec*1000U+dttm.unMSEL;
    aucBuf[17]=LO8(uiTemp);
    aucBuf[18]=HI8(uiTemp);

    aucBuf[19]=LO8(dttm.unMicroSec);
    aucBuf[20]=HI8(dttm.unMicroSec);

    i=writeInDataDisk(iFd, aucBuf, 21);		/* 写入报告文件头 */
    /* assert(i==21);*/
    /* 张云添加 */
    if(i!=21)
    {
        RC_MakeFileTaskExecHandle(iFd, 1);
        return ERROR;
    }
    /* 写入录波头 */
    i=writeInDataDisk(iFd, pucRecHead_g, iRecHeadSz_g);				/* 写入录波头，pucRecHead_g为内容，iRecHeadSz_g为大小 */
    /* assert(i==iRecHeadSz_g); */
    /* 张云添加 */
    if(i != iRecHeadSz_g)
    {
        RC_MakeFileTaskExecHandle(iFd, 1);
        return ERROR;
    }

    *pulMsgSz=iRecHeadSz_g-4;			/* 获得录波头的长度，不包括录波大小4字节 */

    return iFd;
}

/***********************************************************************
* RC_End_File - 结束报告文件 ulMsgSz为录波信息长度 iPiece为录波段数 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static void RC_End_File(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t ulMsgSz,
    int iPiece
)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    uint8_t aucFullBuf[FULL_NAME_LEN+1];
    int i;
    STATUS vxsts;
    int32_t lLastPos;
    EP_STATUS stsHandle;			/* 张云添加 */
    uint8_t *pucBuf;
    uint32_t ulLen;
    uint8_t ucFastDelAttr = 0x01;     /* delete attribution. */

    if(iFd<0)
    {
        return;
    }
    assert(pblk);

    lLastPos=lseek(iFd, 0, SEEK_CUR);			/* 保存当前位置 */
    /* assert(lLastPos>0); */
    /* 张云添加 */
    if(!(lLastPos>0))
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    i = lseek(iFd, 6, SEEK_SET); /* 删除属性位置 */
    if (i != 6)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return;
    }

    if (read(iFd, aucBuf, 1) != 1)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return;
    }

    if (!VI_RdEventDelAttr(pblk->head.unRptSN))
    {
        aucBuf[0] |= FAST_DEL_BIT;
        ucFastDelAttr = 0x01;
    }
    else
    {
        ucFastDelAttr = 0;
    }

    i = lseek(iFd, 6, SEEK_SET); /* 删除属性位置 */
    if (i != 6)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return;
    }

    i = writeInDataDisk(iFd, aucBuf, 1);
    if (i != 1)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return;
    }

    /*2013-7-9 ZY修改，删除录波内容长度非法的录波 */
    if(ulMsgSz!=(lLastPos-25))
    {
        RC_MakeFileTaskExecHandle(iFd, 11);
        return  ;
    }


    i=lseek(iFd, 21, SEEK_SET);			/* 回到录波长度相应位置 */
    /* assert(i==21); */
    /* 张云添加 */
    if(i != 21)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    U32_TO_BYTES(aucBuf, ulMsgSz);     /* 重新填写录波信息的长度，为intel 次序 */

    aucBuf[4]=0;                        /* TOV: 0=sample value. 采样值类型 */

    i=writeInDataDisk(iFd, aucBuf, 5);
    /* assert(i==5); */
    /* 张云添加 */
    if(i != 5)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    i=lseek(iFd, 28, SEEK_SET);			/*  */
    /* assert(i==28);  */
    /* 张云添加 */
    if(i != 28)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    /*2013-8-20 ZY  */
    if(iPiece>255)
    {
        RC_MakeFileTaskExecHandle(iFd, 12);
        return  ;
    }
    aucBuf[0]=iPiece;

    i=writeInDataDisk(iFd, aucBuf, 1);
    /* assert(i==1); */
    /* 张云添加 */
    if(i != 1)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    i=lseek(iFd, lLastPos, SEEK_SET);			/* 回到保存位置 */
    /* assert(i==lLastPos); */
    /* 张云添加 */
    if(i != lLastPos)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    /* 添加该报告的标志集 */
    stsHandle=RC_Append_Flag(iFd, pblk);
    /* 张云添加 */
    if(stsHandle != EP_SUCCESS)
    {
        /*  */
        RC_MakeFileTaskExecHandle(iFd, 6);
        return  ;
    }
    /* 添加该报告的定植 */
    stsHandle=RC_Append_Set(iFd);
    /* 张云添加 */
    if(stsHandle != EP_SUCCESS)
    {
        /*  */
        RC_MakeFileTaskExecHandle(iFd, 7);
        return  ;
    }
    /* 添加该报告的压板 */
    stsHandle=RC_Append_Link(iFd);
    /* 张云添加 */
    if(stsHandle != EP_SUCCESS)
    {
        /*  */
        RC_MakeFileTaskExecHandle(iFd, 8);
        return  ;
    }
    /* 添加该报告的保护功能投退 */
    stsHandle=RC_Append_Func(iFd);
    /* 张云添加 */
    if(stsHandle != EP_SUCCESS)
    {
        /*  */
        RC_MakeFileTaskExecHandle(iFd, 9);
        return  ;
    }
    aucBuf[0]=0xB9;
    aucBuf[1]=0x00;
    aucBuf[2]=0x00;
    aucBuf[3]=0x46;
    aucBuf[4]=0x00;
    aucBuf[5]=0x00;
    aucBuf[6]=0x00;
    aucBuf[7]=0x00;

    i=writeInDataDisk(iFd, aucBuf,8);
    if(i != 8)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    if ((pucBuf=FT_File_To_Mem(EP_INNER_SET_FILE, &ulLen))==NULL)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    U32_TO_BYTES(aucBuf,ulLen);
    i=writeInDataDisk(iFd, aucBuf,4);
    if(i != 4)
    {
        EP_free(pucBuf);
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    i=writeInDataDisk(iFd, pucBuf,ulLen);
    if(i != ulLen)
    {
        EP_free(pucBuf);
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    EP_free(pucBuf);
    /*2013-7-29 ZY,进行互斥操作 */
    semTake(semCkCRCIni_g, WAIT_FOREVER);
    if ((pucBuf=FT_File_To_Mem(EP_CK_SET_FILE, &ulLen))==NULL)
    {
        semGive(semCkCRCIni_g);
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    semGive(semCkCRCIni_g);

    U32_TO_BYTES(aucBuf,ulLen);
    i=writeInDataDisk(iFd, aucBuf,4);
    if(i != 4)
    {
        EP_free(pucBuf);
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    i=writeInDataDisk(iFd, pucBuf,ulLen);

    if(i != ulLen)
    {
        EP_free(pucBuf);
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }
    EP_free(pucBuf);


    vxsts=close(iFd);
    /* assert(vxsts==OK); */
    /* 张云添加 */
    if(vxsts != OK)
    {
        RC_MakeFileTaskExecHandle(iFd, 5);
        return  ;
    }

    sprintf(aucFullBuf, EP_WAVE_REC_DIR "/edp%04x.frw", pblk->head.unRptSN);

    if (FT_Is_File(aucFullBuf))
    {
        vxsts = FS_RemoveFile(aucFullBuf, REC_DIR);
        if (vxsts == ERROR)
        {
            LOG_Dbg_Msg("删除同名录波文件失败!\n", 0, 0, 0, 0, 0, 0);
        }
        else
        {
            LOG_Dbg_Msg("删除同名录波文件成功!\n", 0, 0, 0, 0, 0, 0);
        }
    }

    EP_SetNewestSN(pblk->head.unRptSN);

    sprintf(aucBuf, "edp%04x.frw", pblk->head.unRptSN);

    vxsts = FS_SearchInsertFile(EP_WAVE_REC_DIR "/edptmp.frw.bak", aucBuf, REC_DIR, ucFastDelAttr);

    /* assert(vxsts==OK); */
    /* 张云添加 */
    if(vxsts != OK)
    {
        /* 这是有可能的,因为可能因为CPU繁忙,丢失消息队列信息,信息不全,导致部分生成多个同一个报告号的录波文件,所以重命名失败
        */
        if(FT_Is_File(aucFullBuf))
        {
            /* 若以前同名文件存在，则需要删除 2008-1-23日 张云 */
            vxsts=FS_RemoveFile(aucFullBuf, REC_DIR);
            /*
            assert(vxsts==OK);
            */
        }
        RC_MakeFileTaskExecHandle(iFd, 10);
        return  ;
    }

    RC_File_Added();		/* 删除最老的报告文件 */
    rptsts_g.bRecWrFileOn=FALSE;
}

/***********************************************************************
* RC_New_Piece - 开始记录新的录波段，写录波段段头，录波段点数尚未填写 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_New_Piece(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t *pRtWrLen
)
{
    uint8_t aucBuf[20];
    int i;
    EP_DATE_TIME tFirstPieceFirstPointTime;
    uint32_t ulFirstPieceFirstPointTimeUs;
    int iPtsToBegin = 0;
    int iTmUsInter = 0;

    if(iFd<0)
    {
        *pRtWrLen=0;
        return EP_ERROR;
    }

    assert(pblk);

    memset(aucBuf, 0, sizeof(aucBuf));

    aucBuf[4]=LO8(uiAiRate_g);
    aucBuf[5]=HI8(uiAiRate_g);
    aucBuf[6]=LO8(pblk->head.iDatIntvl);
    aucBuf[7]=HI8(pblk->head.iDatIntvl);

    g_ulRecInter = pblk->head.iDatIntvl;

    iPtsToBegin = pblk->head.ulFirstCnt-pblk->head.ulBgnCnt;
    U32_TO_BYTES(aucBuf+12, iPtsToBegin);

    /*计算第一个段第一个点的绝对时间*/
    iTmUsInter = (1000000L/uiAiRate_g) * iPtsToBegin;
    /*TM_Calc_Time(&g_tRecBeginTm, &tFirstPieceFirstPointTime, iTmUsInter%1000000, iTmUsInter/1000000);*/
    ulFirstPieceFirstPointTimeUs = g_ulRecBeginTmUs + iTmUsInter;
    TM_To_Dttm(ulFirstPieceFirstPointTimeUs, &tFirstPieceFirstPointTime);
    aucBuf[10] = tFirstPieceFirstPointTime.ucIrigbLSFlag;

    if(tFirstPieceFirstPointTime.ucIrigbLSFlag == IRIGB_PLS_60SEC)
    {
        aucBuf[11] = 60;
    }
    else
    {
        aucBuf[11] = tFirstPieceFirstPointTime.ucSec;
    }

#if 0
    /*第一个点均是小于0的情况，无大于0的情况*/
    if(iTmUsInter <= 0)
    {
        /*正闰秒60*/
        if((g_tRecBeginTm.ucIrigbLSFlag & IRIGB_PLS_60SEC) == IRIGB_PLS_60SEC)
        {
            if(tFirstPieceFirstPointTime.ucSec == 0)
            {
                g_tRecBeginTm.ucIrigbLSFlag = IRIGB_PLS_60SEC;
            }
        }
        /*正闰秒0以后*/
        else if((g_tRecBeginTm.ucIrigbLSFlag & IRIGB_AFTER_PLS_0SEC) == IRIGB_AFTER_PLS_0SEC)
        {
            tFirstPieceFirstPointTime.ucIrigbLSFlag = IRIGB_AFTER_PLS_0SEC;
            iSecToLs = SYN_GetSubSec(&tFirstPieceFirstPointTime, &g_tLSDataTime) + 1;
            if(iSecToLs < 0)
            {
                tFirstPieceFirstPointTime.ucIrigbLSFlag = 0;
                if( tFirstPieceFirstPointTime.ucSec == 59)
                    tFirstPieceFirstPointTime.ucIrigbLSFlag = IRIGB_PLS_60SEC;
                ulSec = TM_Time_To_Long(&tFirstPieceFirstPointTime);
                ulSec += 1;
                TM_Long_To_Time(&tFirstPieceFirstPointTime, ulSec);
            }
        }
        /*负闰秒0以后*/
        else if((g_tRecBeginTm.ucIrigbLSFlag & IRIGB_AFTER_NLS_0SEC) == IRIGB_AFTER_NLS_0SEC)
        {
            tFirstPieceFirstPointTime.ucIrigbLSFlag = IRIGB_AFTER_NLS_0SEC;
            iSecToLs = SYN_GetSubSec(&tFirstPieceFirstPointTime, &g_tLSDataTime) + 1;
            if(iSecToLs < 0)
            {
                tFirstPieceFirstPointTime.ucIrigbLSFlag = 0;
                ulSec = TM_Time_To_Long(&tFirstPieceFirstPointTime);
                ulSec -= 1;
                TM_Long_To_Time(&tFirstPieceFirstPointTime, ulSec);
            }
        }
        aucBuf[10] = tFirstPieceFirstPointTime.ucIrigbLSFlag;
        aucBuf[11] = tFirstPieceFirstPointTime.ucSec;
        if((tFirstPieceFirstPointTime.ucIrigbLSFlag & IRIGB_PLS_60SEC) == IRIGB_PLS_60SEC)
        {
            aucBuf[11] = 60;
        }

        SYN_LOG("$$$$$$$ g_tRecBeginTm: 时分秒毫秒微秒:%d-%d-%d-%d-%d   flag:0x%x\n",
                g_tRecBeginTm.ucHour,g_tRecBeginTm.ucMinute,g_tRecBeginTm.ucSec,
                g_tRecBeginTm.unMSEL, g_tRecBeginTm.unMicroSec, g_tRecBeginTm.ucIrigbLSFlag);
        SYN_LOG("$$$$$$$ tFirstPieceFirstPointTime: 时分秒毫秒微秒:%d-%d-%d-%d-%d   flag:0x%x\n",
                tFirstPieceFirstPointTime.ucHour, tFirstPieceFirstPointTime.ucMinute,tFirstPieceFirstPointTime.ucSec,
                tFirstPieceFirstPointTime.unMSEL, tFirstPieceFirstPointTime.unMicroSec, tFirstPieceFirstPointTime.ucIrigbLSFlag);
        SYN_LOG("$$$$$$$ uiAiRate_g: %d,  iPtsToBegin:%d, iTmUsInter:%d \n",
                uiAiRate_g,iPtsToBegin,iTmUsInter,0,0,0);
    }
    else if(iTmUsInter > 0)
    {
    }
#endif

    i=writeInDataDisk(iFd, aucBuf, 20);
    /* assert(i==20); */
    /* 张云添加 */
    if(i != 20)
    {
        /*  */
        *pRtWrLen=i;
        return EP_ERROR;
    }
    *pRtWrLen=i;
    return EP_SUCCESS;
}

/***********************************************************************
* RC_Wr_Rec_Dat - 记录一般的录波段中间的录波块数据 ，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Wr_Rec_Dat(
    int iFd,
    RC_DAT_BLK *pblk,
    uint32_t  *pRtWrLen,
    uint8_t *puErrSts
)
{
    int iPts;
    uint64_t *pullPtsBgn;
    uint8_t *pucWork;
    BOOL *pbWork;
    uint8_t *pucDst;
    uint8_t ucBit;
    int i;
    int j;

    *puErrSts=0;
    if(iFd<0)
    {
        /* 文件描述符无效 */
        *pRtWrLen=0;
        *puErrSts |= 0x01;
        return EP_ERROR;
    }

    assert(pblk);

    iPts=(pblk->head.ulNextCnt-pblk->head.ulFirstCnt)/pblk->head.iDatIntvl;			/* 获得本块记录的录波点数 */
    /* assert(iPts<=iPtsPerBlk_g); */
    /* 张云添加 */
    if(!(iPts <= iPtsPerBlk_g)) 			/* 最大的点数 */
    {
        /* 录波点数过多 */
        *pRtWrLen=0;
        *puErrSts |= 0x02;
        return EP_ERROR;
    }

    pucDst=pucFileBuf_g; 				/* 写故障报告的临时缓冲 */
    pullPtsBgn=pblk->aullDat; 						/* 数据存储地址 */
    for (i=0; i<iPts; i++)
    {
        /* 对每个点进行记录 */
        pucWork=(uint8_t*)pullPtsBgn;
        for (j=0; j<iRunRecAi_g*2; j++)
        {
            /*先写录波AI  */
            *pucDst++=pucWork[3];
            *pucDst++=pucWork[2];
            *pucDst++=pucWork[1];
            *pucDst++=pucWork[0];

            pucWork+=4;
        }

        pbWork=(BOOL*)pucWork;
        ucBit=0;
        for (j=0; j<iRunRecDi_g; j++)
        {
            /* 再记录录波DI */
            if (*pbWork++)
                ucBit |= BV8(j%8);

            if (j%8==7)
            {
                *pucDst++=ucBit;
                ucBit=0;
            }
        }
        if (j%8)
            *pucDst++=ucBit;

        /* 填写DOBUF缓冲中的内容 */
        pucWork=(uint8_t*)pbWork;
        /* 填写实际记录时刻 */
        *pucDst++=pucWork[3];
        *pucDst++=pucWork[2];
        *pucDst++=pucWork[1];
        *pucDst++=pucWork[0];

        pullPtsBgn+=iDatPerPts_g;
    }

    /* assert(pucDst-pucFileBuf_g==iPtsBytes_g*iPts); */
    /* 张云添加 */
    if(pucDst-pucFileBuf_g != iPtsBytes_g*iPts)
    {
        /* 记录点数无效*/
        *pRtWrLen=0;
        *puErrSts |= 0x04;
        return EP_ERROR;
    }

    i=writeInDataDisk(iFd, (char*)pucFileBuf_g, iPtsBytes_g*iPts);
    /* assert(i==iPtsBytes_g*iPts); */
    /* 张云添加 */
    if(i != iPtsBytes_g*iPts)
    {
        /*  写文件无效 */
        *pRtWrLen=0;
        *puErrSts |= 0x08;
        return EP_ERROR;
    }
    *pRtWrLen=iPts;

    return EP_SUCCESS;
}

/***********************************************************************
* RC_End_Piece - 结束本录波段的记录  lFilePos，本录波段的起始位置，为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_End_Piece(
    int iFd,
    uint32_t lFilePos,
    uint32_t ulPts
)
{
    int i;
    uint8_t aucBuf[4];
    int32_t lLastPos;

    if(iFd<0 || lFilePos<=0)
    {
        return	EP_ERROR;
    }

    lLastPos=lseek(iFd, 0, SEEK_CUR);
    /* assert(lLastPos>lFilePos); */
    /* 张云添加 */
    if(!(lLastPos>lFilePos))
    {
        /*  */
        return EP_ERROR;
    }

    i=lseek(iFd, lFilePos, SEEK_SET);
    /* assert(i==lFilePos); */
    /*  张云添加 */
    if(i != lFilePos)
    {
        /*  */
        return EP_ERROR;
    }

    U32_TO_BYTES(aucBuf, ulPts);			/* 设置本段的录波点数 */

#if 0
    /*if(!g_bRecEndTmAfterLs && g_bRecBeginTmBeforeLs)*/
    {
        ulPtsToBegin = ulPts * g_ulRecInter + g_ulPtsToBeginInPiece;
        ullTmUsToBegin = (1000000L/uiAiRate_g) * ulPtsToBegin;
        ullTmUsInter = SYN_GetSubMicroSec(&g_tRecBeginTm, &g_tLSDataTime);
        if((g_bLeapSecondFlagCpu & IRIGB_PLS) == IRIGB_PLS)
        {
            /*此处的+相当于-, g_tLSDataTime 时间是1s,
                所以不需要在进行+-1s的操作了*/
            if((ullTmUsInter < 0) && ((ullTmUsToBegin + ullTmUsInter) > 0))
            {
                /*g_bRecEndTmAfterLs = TRUE;*/
            }
        }
        else if((g_bLeapSecondFlagCpu & IRIGB_NLS) == IRIGB_NLS)
        {
            /*此处的+相当于-, g_tLSDataTime 时间是59s, 所以不需要在进行+-1s的操作了*/
            if((ullTmUsInter < 0) && ((ullTmUsToBegin + ullTmUsInter) > 0))
            {
                /*g_bRecEndTmAfterLs = TRUE;*/
            }
        }
    }
#endif


    i=writeInDataDisk(iFd, aucBuf, 4);
    /* assert(i==4); */
    /* 张云添加 */
    if(i != 4)
    {
        /*  */
        return EP_ERROR;
    }

    i=lseek(iFd, lLastPos, SEEK_SET);
    /* assert(i==lLastPos); */
    /* 张云添加 */
    if(i!=lLastPos)
    {
        /*  */
        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Append_Flag - 往故障报告文件中添加标志集记录  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Flag(
    int iFd,
    RC_DAT_BLK *pblk
)
{
    int i;
    int32_t lBgnPos;
    int32_t lAreaPos;
    int32_t lRecNum;
    RC_TSK_FLAG *ptskfg;
    RC_TSK_FLAG **pptskfg;
    uint32_t ulTemp;
    uint32_t  ulWrRecNum;
    EP_STATUS    stsWr;

    if(iFd<0)
    {
        return EP_ERROR;
    }
    assert(pblk);

    lBgnPos=lseek(iFd, 0, SEEK_CUR);
    /* assert(lBgnPos>0); */
    /* 张云添加 */
    if(!(lBgnPos>0))
    {
        /*  */
        return EP_ERROR;
    }

    i=writeInDataDisk(iFd, aucFlagHead_g, sizeof(aucFlagHead_g));/*填写标志头  */
    /* assert(i==sizeof(aucFlagHead_g)); */
    /* 张云添加 */
    if(i != sizeof(aucFlagHead_g))
    {
        /*  */
        return EP_ERROR;
    }

    /* 4 reserved bytes. */
    pucFileBuf_g[8]=0;
    pucFileBuf_g[9]=0;
    pucFileBuf_g[10]=0;
    pucFileBuf_g[11]=0;

    pptskfg=aptskfg_g;
    while ((ptskfg=*pptskfg++)!=NULL)
    {
        /* 依次处理每个标志区的标志记录 */
        lAreaPos=lseek(iFd, 0, SEEK_END);
        /* assert(lAreaPos>lBgnPos); */
        /* 张云添加 */
        if(!(lAreaPos>lBgnPos))
        {
            /*  */
            return EP_ERROR;
        }

        assert(ptskfg->pucHead && ptskfg->iHeadSz);
        i=writeInDataDisk(iFd, ptskfg->pucHead, ptskfg->iHeadSz);		/* 填写当前标志区的标志区头 */
        /* assert(i==ptskfg->iHeadSz); */
        /* 张云添加 */
        if(i!=ptskfg->iHeadSz)
        {
            /*  */
            return EP_ERROR;
        }

        stsWr=RC_Wr_Flag_Dat(iFd, ptskfg, pblk,&ulWrRecNum);				/* 填写该标志区的标志,张云修改过 */
        if(stsWr==EP_SUCCESS)
        {
            lRecNum=ulWrRecNum;
        }
        else
        {
            /* 若写不成功,则返回失败 */
            return EP_ERROR;
        }

        i=lseek(iFd, lAreaPos+ptskfg->iHeadSz-4, SEEK_SET);
        /* assert(i==lAreaPos+ptskfg->iHeadSz-4); */			/* 这里原来有bug */
        /* 张云添加 */
        if(i!=lAreaPos+ptskfg->iHeadSz-4)
        {
            /*  */
            return EP_ERROR;
        }


        /* Write record number to file. */
        U32_TO_BYTES(pucFileBuf_g, lRecNum);			/* 填写该标志区的记录的数目 */
        i=writeInDataDisk(iFd, pucFileBuf_g, 4);
        /* assert(i==4); */
        /* 张云添加 */
        if(i != 4)
        {
            /*  */
            return EP_ERROR;
        }
    }                                   /* FILE NOT SEEK TO THE END WHEN BREAK! */

    /* i is seek to file end. */
    i=lseek(iFd, 0, SEEK_END);
    /* assert(i>lBgnPos); */
    /* 张云添加 */
    if(!(i>lBgnPos))
    {
        /*  */
        return EP_ERROR;
    }

    ulTemp=i-lBgnPos-4;                 	/* Flag set message length. */
    U32_TO_BYTES(pucFileBuf_g, ulTemp);				/* 获得标志集信息长度 */

    i=lseek(iFd, lBgnPos, SEEK_SET);
    /*assert(i==lBgnPos);*/
    /* 张云添加 */
    if(i!=lBgnPos)
    {
        /*  */
        return EP_ERROR;
    }

    i=writeInDataDisk(iFd, pucFileBuf_g, 4);
    /* assert(i==4); */
    /* 张云添加 */
    if(i != 4)
    {
        /*  */
        return EP_ERROR;
    }

    i=lseek(iFd, 0, SEEK_END);
    /* assert(i>lBgnPos); */
    /* 张云添加 */
    if(!(i>lBgnPos))
    {
        /*  */
        return EP_ERROR;
    }

    return EP_SUCCESS;

}

/***********************************************************************
* RC_Wr_Flag_Dat - 往故障报告中添加某标志区的标志集的内容，返回标志块数  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Wr_Flag_Dat(
    int iFd,
    RC_TSK_FLAG *ptskfg,
    RC_DAT_BLK *pblk,
    uint32_t *pRtWrRecNum
)
{
    int32_t lBgnPos;
    uint32_t lRecNum;
    uint32_t ulOldDatTime;
    uint16_t unDatRptSN;
    RC_FLAG_BLK *pfgblk;
    int i;
    uint8_t *puc;
    uint8_t *pucDst;
    uint32_t ulTemp;
    STATUS vxsts;
    BOOL bValidRec;

    if(iFd<0)
    {
        return EP_ERROR;
    }
    assert(pblk);

    lBgnPos=lseek(iFd, 0, SEEK_CUR);
    /* assert(lBgnPos>0); */
    /* 张云添加 */
    if(!(lBgnPos>0))
    {
        /*  */
        *pRtWrRecNum=0;
        return EP_ERROR;
    }

rewrite:
    lRecNum=0;
    while (ptskfg->iUsedBlks)
    {
        /* 若该标志区的标志记录还有未处理完，循环处理 */
        vxsts=taskLock();
        assert(vxsts==OK);

        i=ptskfg->iBlkWrPos-ptskfg->iUsedBlks;		/* 此次记录的标志块在缓冲的位置，
            																			 为此次最新的记录块位置减去还未处理的记录块数 */
        if (i<0)
            i += iFgDatBlk_g;

        assert(i>=0 && i<iFgDatBlk_g);
        pfgblk=ptskfg->ppfgblk[i];		/* 此次记录的标志块 */

        ulOldDatTime=pfgblk->ulDatTime;					/* 此标志块数据窗时刻 */
        unDatRptSN=pfgblk->unRptSN;				/* 此标志块的报告号 */

        vxsts=taskUnlock();
        assert(vxsts==OK);

        i=unDatRptSN-pblk->head.unRptSN;
        if (i>0)		/* 若标志块对应的报告号比故障报告号还超前，则退出循环，不记录 */
            break;
        else if (i<0)
        {
            vxsts=taskLock();
            assert(vxsts==OK);

            if (ulOldDatTime==pfgblk->ulDatTime)
            {
                /* 否则，则表示来不及处理，则将该标志记录抛弃 */
                ptskfg->iUsedBlks--;
                assert(ptskfg->iUsedBlks>=0);
            }

            vxsts=taskUnlock();
            assert(vxsts==OK);
        }
        else		/* 若报告号正好匹配，则表示是该报告的标志记录，则记录之 */
        {
            ulTemp=pfgblk->ulRecTime-pblk->head.ulBgnCnt;
            U32_TO_BYTES(pucFileBuf_g, ulTemp);			/* 记录该标志的记录时刻 */

            ulTemp=pfgblk->ulDatTime-pblk->head.ulBgnCnt;/*记录该标志的数据窗时刻  */
            U32_TO_BYTES(pucFileBuf_g+4, ulTemp);

            /* Reverse byte order before writting to file. */
            puc=(uint8_t*)pfgblk->aulFlagSet;
            pucDst=pucFileBuf_g+12;
            i=(ptskfg->uiFlagSz+3)/4;
            while (i--)			/* 每4个字节为单位，写到缓冲 */
            {
                *pucDst++=puc[3];
                *pucDst++=puc[2];
                *pucDst++=puc[1];
                *pucDst++=puc[0];
                puc+=4;
            }

            i=writeInDataDisk(iFd, pucFileBuf_g, 12+ptskfg->uiFlagSz);			/* 写本标志记录到文件 */
            /*assert(i==12+ptskfg->uiFlagSz);*/
            /* 张云添加 */
            if(i != 12+ptskfg->uiFlagSz)
            {
                /*  */
                *pRtWrRecNum=0;
                return   EP_ERROR;
            }


            vxsts=taskLock();
            assert(vxsts==OK);

            if (ulOldDatTime==pfgblk->ulDatTime)
            {
                ptskfg->iUsedBlks--;		/* 待处理的标志块-- */
                assert(ptskfg->iUsedBlks>=0);

                lRecNum++;			/* 本报告的该任务标志记录数++ */
                bValidRec=TRUE;
            }
            else
                bValidRec=FALSE;/*不可能出现 */

            vxsts=taskUnlock();
            assert(vxsts==OK);

            if (!bValidRec)
            {

                 struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出
                unsigned long long nFreeSpace = 0; 
                STATUS vxsts;

                if (fstatvfs(iFd, &disk_stats) == 0) { // 成功返回 0
                    // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
                nFreeSpace = (unsigned long long)disk_stats.f_bavail * disk_stats.f_frsize;
            
                vxsts = OK;
                } else {
                    *pRtWrRecNum=0;
                    return   EP_ERROR;
                }


                goto rewrite;
            }
        }
    }
    *pRtWrRecNum=lRecNum;

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Append_Set - 往故障报告中添加定值信息  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Set(
    int iFd
)
{
    /*支持字符串定值，张云改过，2008-7-21日 ，以前初始化的BUF实际没有用 */
    const SC_SET_PAGE *psetpg;
    const SC_SET_ITEM *pset;
    int iPage;
    int i;
    uint8_t *puc;
    uint8_t  aucBuf[100+MAX_ID_LEN];
    int   iSetInfLen;


    if(iFd<0)
    {
        return EP_ERROR;
    }

    iSetInfLen=8+6*(iSetPgNum_g-1);
    for (iPage=0; iPage<iSetPgNum_g-1; iPage++)
    {
        psetpg=SC_Get_Set_Pg_Attr(iPage+1);

        iSetInfLen+=9*psetpg->iSetNum;

        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
        {
            if(pset->ucUnit==0x68)
            {
                iSetInfLen+=pset->valNow.ulVal;
            }
        }
    }

    U32_TO_BYTES(aucBuf, iSetInfLen-4);

    aucBuf[4]=SC_Work_Set_Area();
    aucBuf[5]=0;
    aucBuf[6]=0;

    assert(iSetPgNum_g>0 && iSetPgNum_g<=256);
    aucBuf[7]=iSetPgNum_g-1;

    i=writeInDataDisk(iFd, aucBuf, 8);
    if(i != 8)
    {
        /*  */
        return   EP_ERROR;
    }

    for (iPage=0; iPage<iSetPgNum_g-1; iPage++)
    {
        psetpg=SC_Get_Set_Pg_Attr(iPage+1);

        puc=aucBuf;

        *puc++=iPage;
        *puc++=(psetpg->bIsPub || psetpg->psublgc->bRun)?1:0;
        *puc++=LO8(psetpg->iSetNum);
        *puc++=HI8(psetpg->iSetNum);
        *puc++=0;
        *puc++=0;

        i=writeInDataDisk(iFd, aucBuf, 6);
        if(i != 6)
        {
            /*  */
            return   EP_ERROR;
        }


        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
        {

            puc=aucBuf;
            *puc++=0;
            *puc++=0;

            i=pset-psetpg->pset;
            *puc++=LO8(i);
            *puc++=HI8(i);

            *puc++=pset->ucUnit;

            U32_TO_BYTES(puc, pset->valNow.ulVal);
            puc+=4;
            if(pset->ucUnit==0x68)
            {
                strncpy(puc,pset->aucNowStr,pset->valNow.ulVal);
                puc=puc+pset->valNow.ulVal;
            }

            i=writeInDataDisk(iFd, aucBuf,puc-aucBuf );
            if(i != puc-aucBuf)
            {
                /*  */
                return   EP_ERROR;
            }
        }
    }

    return   EP_SUCCESS;

}

/***********************************************************************
* RC_Append_Link - 往故障报告中添加压板信息  为防止录波文件任务异常关闭，张云2008-1-23日修改过漏掉的软硬压板状态
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Link(
    int iFd
)
{
    EP_STATUS sts;
    int iIdx;
    BOOL bLinkSts;
    BOOL bSWLinkSts;
    int i;
    uint8_t *puc;
    uint16_t ulTotalLinkMode=0;
    uint8_t ucTemp=0;
    BOOL bRmtSwitchYB;
    BOOL bRmtSwitchSetting;

    if(iFd<0)
    {
        return  EP_ERROR;
    }
    SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
    switch(ulTotalLinkMode)
    {
        case LINK_MODE_HW:
            ucTemp=0;
            break;
        case LINK_MODE_SW:
            ucTemp=1;
            break;
        case LINK_MODE_AND:
            ucTemp=2;
            break;
        case LINK_MODE_OR:
            ucTemp=3;
            break;
        case LINK_MODE_CUS:
            ucTemp=4;
            break;
        default:
            return   EP_ERROR;

    }

    pucLinkBuf_g[4]=ucTemp;

    ucTemp = 0x00;
    if (FT_Get_GWYB_Sts(&bRmtSwitchYB, &bRmtSwitchSetting) == EP_SUCCESS)
    {
        ucTemp = 0x01;
        if (bRmtSwitchYB)
        {
            ucTemp |= 0x02;
        }

        if (bRmtSwitchSetting)
        {
            ucTemp |= 0x04;
        }
    }
    pucLinkBuf_g[5] = ucTemp;

    puc=pucLinkBuf_g+9;
    for (iIdx=0; iIdx<iLinkNum_g; iIdx++)
    {
        sts=SC_Get_Link_Now_Sts(iIdx, &bLinkSts);
        assert(sts==EP_SUCCESS);
        sts=SC_Get_Link_SW_Sts(iIdx, &bSWLinkSts);
        assert(sts==EP_SUCCESS);

        puc[3]=bSWLinkSts?1:0;
        puc[4]=bLinkSts?1:0;

        puc+=5;
    }

    i=writeInDataDisk(iFd, pucLinkBuf_g, ulLinkLen_g);
    /*assert(i==ulLinkLen_g);*/
    /* 张云添加 */
    if(i!=ulLinkLen_g)
    {
        /*  */

        return   EP_ERROR;
    }
    return   EP_SUCCESS;
}

/***********************************************************************
* RC_Append_Func - 往故障报告中添加保护功能投退信息  为防止录波文件任务异常关闭，张云修改过
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Append_Func(
    int iFd
)
{
    int i;

    if(iFd<0)
    {
        return EP_ERROR;
    }


    i=writeInDataDisk(iFd, pucFuncBuf_g, ulFuncLen_g);
    /* assert(i==ulFuncLen_g); */
    /* 张云添加 */
    if(i != ulFuncLen_g)
    {
        /*  */
        return EP_ERROR;
    }
    return EP_SUCCESS;
}

/***********************************************************************
* RC_Init_Rec_Head - 填写录波部分的文件头
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS RC_Init_Rec_Head(void)
{
    uint8_t *puc;
    int i;
    int iRecDoBufNum;

    /* 被录波记录的头的大小 */
    iRecHeadSz_g=14+5*iRunRecAi_g+4*iRunRecDi_g;			/* 前面4字节为录波信息大小，待最后添入 */

    if ((pucRecHead_g=calloc(iRecHeadSz_g, sizeof(*pucRecHead_g)))==NULL)
        return EP_BUF_ERR;

    /* 录波点记录的字节数 */
    iPtsBytes_g=iRunRecAi_g*8+(iRunRecDi_g+7)/8+4;

    /* 若临时文件缓冲太小，每个缓冲最大为一个录波块的大小，则重新申请 */
    if (ulFileBufSz_g<(uint32_t)iPtsPerBlk_g*iPtsBytes_g)
    {
        if (pucFileBuf_g)
            free(pucFileBuf_g);

        if ((pucFileBuf_g=calloc(iPtsPerBlk_g, iPtsBytes_g))==NULL)
            return EP_BUF_ERR;

        ulFileBufSz_g=(uint32_t)iPtsPerBlk_g*iPtsBytes_g;
    }

    /* 8 bytes of rec message length/piece number/reserved are 0 now. */
    iRecDoBufNum=0;

    pucRecHead_g[5]=LO8(iRecDoBufNum);		/* DO录波缓冲记录的最大DO数目 */
    pucRecHead_g[6]=HI8(iRecDoBufNum);
    pucRecHead_g[8]=LO8(iRunRecAi_g);
    pucRecHead_g[9]=HI8(iRunRecAi_g);
    pucRecHead_g[10]=LO8(iRunRecDi_g);
    pucRecHead_g[11]=HI8(iRunRecDi_g);

    pucRecHead_g[12]=LO8(iPtsBytes_g);
    pucRecHead_g[13]=HI8(iPtsBytes_g);

    puc=pucRecHead_g+14;
    for (i=0; i<iRecAiNum_g; i++)
    {
        /* 对所有录波模拟量进行循环 */
        if (praidb_g[i].pvHisHnd)
        {
            /* 若该AI录波量被真正录波，则添加到文件缓冲中去 */
            puc[0]=LO8(i);
            puc[1]=HI8(i);

            puc[2]=praicfg_g[i].ucUnit;

            puc+=5;
        }
    }

    for (i=0; i<iRecDiNum_g; i++)
    {
        if (prdidb_g[i].pvHisHnd)
        {
            /* 若该DI录波量被真正录波，则添加到文件缓冲中去 */
            puc[0]=LO8(i);
            puc[1]=HI8(i);

            puc+=4;
        }
    }

    assert(puc-pucRecHead_g==iRecHeadSz_g);

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Init_Flag_Head - 初始化故障报告的标志集头到缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Flag_Head(void)
{
    RC_TSK_FLAG *ptskfg;
    RC_TSK_FLAG **pptskfg;
    RC_FLAG_DB *prfgdb;
    RC_FLAG_DB **pprfgdbIdx;
    uint8_t *puc;
    int i;

    aucFlagHead_g[6]=LO8(uiAiRate_g);		/* 头从标志集大小4字节开始,公共头为9字节 */
    aucFlagHead_g[7]=HI8(uiAiRate_g);
    aucFlagHead_g[8]=iFgTskNum_g;
    assert(iFgTskNum_g<=255);

    pptskfg=aptskfg_g;
    while ((ptskfg=*pptskfg++)!=NULL)
    {
        /* 对每个标志区的相关头信息申请内存，填充到标志区中 */
        ptskfg->iHeadSz=20+6*ptskfg->iAiNum+5*ptskfg->iDiNum;		/* 20字节包括该区公共配置头6字节，
       																												 公共AI配置头2字节，公共DI配置头2字节，记录区的头10字节，供20字节 */

        if ((ptskfg->pucHead=calloc(ptskfg->iHeadSz,
                                    sizeof(*ptskfg->pucHead)))==NULL)
            return EP_BUF_ERR;

        puc=ptskfg->pucHead;
        /* 填充该记录区相关头信息 */
        i=ptskfg->iAiNum+ptskfg->iDiNum;							/* 该记录区的标志个数 */
        *puc++=LO8(i);				/* 记录区头信息开始 */
        *puc++=HI8(i);

        puc+=4;

        *puc++=LO8(ptskfg->iAiNum);			/* AI标志个数 */
        *puc++=HI8(ptskfg->iAiNum);
        /* 原始 REAL模拟量标志头信息 */
        pprfgdbIdx=pprfgRealAi_g;
        while ((prfgdb=*pprfgdbIdx++)!=NULL)
        {
            assert(prfgdb->sigtp==REAL_AI_CH);

            *puc++=prfgdb->iPage;
            assert(prfgdb->iPage<=255);

            *puc++=LO8(prfgdb->iItem);
            *puc++=HI8(prfgdb->iItem);

            *puc++=1;		/* 原始通道 */
            puc++;

            *puc++=prfgcfg_g[prfgdb-prfgdb_g].ucUnit;
            assert(IS_REAL_AI(*(puc-1)));
        }

        /* 原始COMPLEX模拟量标志信息 */
        pprfgdbIdx=pprfgCplxAi_g;
        while ((prfgdb=*pprfgdbIdx++)!=NULL)
        {
            assert(prfgdb->sigtp==CPLX_AI_CH);

            *puc++=prfgdb->iPage;
            assert(prfgdb->iPage<=255);

            *puc++=LO8(prfgdb->iItem);
            *puc++=HI8(prfgdb->iItem);

            *puc++=1;				/* 原始通道 */
            puc++;

            *puc++=prfgcfg_g[prfgdb-prfgdb_g].ucUnit;
            assert(IS_CPLX_AI(*(puc-1)));
        }

        /* 中间结果AI通道标志 */
        pprfgdbIdx=ptskfg->pprfgAi;
        while ((prfgdb=*pprfgdbIdx++)!=NULL)
        {
            assert(prfgdb->sigtp==ELEM_IO_AI);

            *puc++=prfgdb->iPage;
            assert(prfgdb->iPage<=255);

            *puc++=LO8(prfgdb->iItem);
            *puc++=HI8(prfgdb->iItem);

            *puc++=0;			/* 中间结果通道 */
            puc++;

            *puc++=prfgcfg_g[prfgdb-prfgdb_g].ucUnit;
        }

        *puc++=LO8(ptskfg->iDiNum);/* DI标志个数 */
        *puc++=HI8(ptskfg->iDiNum);

        /*原始DI通道标志  */
        pprfgdbIdx=pprfgOrgDi_g;
        while ((prfgdb=*pprfgdbIdx++)!=NULL)
        {
            assert(prfgdb->sigtp==ORG_DI_CH);

            *puc++=prfgdb->iPage;
            assert(prfgdb->iPage<=255);

            *puc++=LO8(prfgdb->iItem);
            *puc++=HI8(prfgdb->iItem);

            *puc++=1;			/* 原始通道 */
            puc++;
        }

        /* 中间结果DI通道标志 */
        pprfgdbIdx=ptskfg->pprfgDi;
        while ((prfgdb=*pprfgdbIdx++)!=NULL)
        {
            assert(prfgdb->sigtp==ELEM_IO_DI);

            *puc++=prfgdb->iPage;
            assert(prfgdb->iPage<=255);

            *puc++=LO8(prfgdb->iItem);
            *puc++=HI8(prfgdb->iItem);

            *puc++=0;		/* 中间结果通道 */
            puc++;
        }

        *puc++=LO8(ptskfg->uiFlagSz);			/* 该记录区中每次记录的长度BZL */
        *puc++=HI8(ptskfg->uiFlagSz);

        /* 12=RecTime+DatTime+Reserved.  +3 bytes to deal with DI using uint32_t. */
        i=12+ptskfg->uiFlagSz+3;
        if (ulFileBufSz_g<i)
        {
            if (pucFileBuf_g)
                free(pucFileBuf_g);

            if ((pucFileBuf_g=calloc(i, 1))==NULL)
                return EP_BUF_ERR;

            ulFileBufSz_g=i;
        }

        puc+=4;                         /* Reserved 4 bytes. 4字节保留*/

        puc+=4;                         /* Number of flag record.  Init=0 first. 该记录区的记录数目，待添，记录区头结束*/

        assert(puc-ptskfg->pucHead==ptskfg->iHeadSz);
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Init_Set_Buf - 填写定植头缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Set_Buf(void)
{
    const SC_SET_PAGE *psetpg;
    const SC_SET_ITEM *pset;
    int iPage;
    int i;
    uint8_t *puc;

    ulSetLen_g=8+6*(iSetPgNum_g-1);
    for (iPage=0; iPage<iSetPgNum_g-1; iPage++)
    {
        psetpg=SC_Get_Set_Pg_Attr(iPage+1);

        ulSetLen_g+=9*psetpg->iSetNum;
    }

    if ((pucSetBuf_g=calloc(ulSetLen_g, sizeof(*pucSetBuf_g)))==NULL)
        return EP_BUF_ERR;

    U32_TO_BYTES(pucSetBuf_g, ulSetLen_g-4);

    pucSetBuf_g[4]=SC_Work_Set_Area();
    pucSetBuf_g[5]=0;
    pucSetBuf_g[6]=0;

    assert(iSetPgNum_g>0 && iSetPgNum_g<=256);
    pucSetBuf_g[7]=iSetPgNum_g-1;

    puc=pucSetBuf_g+8;
    for (iPage=0; iPage<iSetPgNum_g-1; iPage++)
    {
        psetpg=SC_Get_Set_Pg_Attr(iPage+1);

        *puc++=iPage;
        *puc++=(psetpg->bIsPub || psetpg->psublgc->bRun)?1:0;
        *puc++=LO8(psetpg->iSetNum);
        *puc++=HI8(psetpg->iSetNum);
        *puc++=0;
        *puc++=0;

        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
        {
            *puc++=0;
            *puc++=0;

            i=pset-psetpg->pset;
            *puc++=LO8(i);
            *puc++=HI8(i);

            *puc++=pset->ucUnit;

            U32_TO_BYTES(puc, pset->valNow.ulVal);
            puc+=4;
        }
    }
    assert(puc-pucSetBuf_g==ulSetLen_g);

    return EP_SUCCESS;
}

/***********************************************************************
* RC_Init_Link_Buf - 填写压板头缓冲  张云2008-1-23日修改过漏掉的软硬压板状态
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Link_Buf(void)
{


    int iIdx;
    uint8_t *puc;
    uint16_t ulTotalLinkMode=0;
    uint8_t ucTemp=0;
    BOOL bRmtSwitchYB;
    BOOL bRmtSwitchSetting;

    ulLinkLen_g=9+5*iLinkNum_g;

    if ((pucLinkBuf_g=calloc(ulLinkLen_g, sizeof(*pucLinkBuf_g)))==NULL)
        return EP_BUF_ERR;

    U32_TO_BYTES(pucLinkBuf_g, ulLinkLen_g-4);

    SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
    switch(ulTotalLinkMode)
    {
        case LINK_MODE_HW:
            ucTemp=0;
            break;
        case LINK_MODE_SW:
            ucTemp=1;
            break;
        case LINK_MODE_AND:
            ucTemp=2;
            break;
        case LINK_MODE_OR:
            ucTemp=3;
            break;
        case LINK_MODE_CUS:
            ucTemp=4;
            break;
        default:
            return   EP_ERROR;

    }

    pucLinkBuf_g[4]=ucTemp;

    ucTemp = 0x00;
    if (FT_Get_GWYB_Sts(&bRmtSwitchYB, &bRmtSwitchSetting) == EP_SUCCESS)
    {
        ucTemp = 0x01;
        if (bRmtSwitchYB)
        {
            ucTemp |= 0x02;
        }

        if (bRmtSwitchSetting)
        {
            ucTemp |= 0x04;
        }
    }

    pucLinkBuf_g[5] = ucTemp;
    pucLinkBuf_g[6]=0;

    pucLinkBuf_g[7]=LO8(iLinkNum_g);
    pucLinkBuf_g[8]=HI8(iLinkNum_g);

    puc=pucLinkBuf_g+9;
    for (iIdx=0; iIdx<iLinkNum_g; iIdx++)
    {
        *puc++=LO8(iIdx);
        *puc++=HI8(iIdx);
        *puc++=0;
        *puc++=0;
        *puc++=0;
    }
    assert(puc-pucLinkBuf_g==ulLinkLen_g);

    return EP_SUCCESS;

}

/***********************************************************************
* RC_Init_Func_Buf - 填写保护功能投退头缓冲
*
* RETURNS: 无
*
*/
static EP_STATUS RC_Init_Func_Buf(void)
{
    uint8_t *puc;
    const SC_SUB_LGC_ITEM *psublgc;
    int iIdx;

    ulFuncLen_g=8+iSubLgcNum_g*4;

    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        psublgc=SC_Get_Sub_Lgc_Attr(iIdx);
        ulFuncLen_g+=strlen(psublgc->aucName);
    }

    if ((pucFuncBuf_g=calloc(ulFuncLen_g, sizeof(*pucFuncBuf_g)))==NULL)
        return EP_BUF_ERR;

    U32_TO_BYTES(pucFuncBuf_g, ulFuncLen_g-4);

    pucFuncBuf_g[4]=0;
    pucFuncBuf_g[5]=0;

    pucFuncBuf_g[6]=LO8(iSubLgcNum_g);
    pucFuncBuf_g[7]=HI8(iSubLgcNum_g);

    puc=pucFuncBuf_g+8;
    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        psublgc=SC_Get_Sub_Lgc_Attr(iIdx);

        *puc=strlen(psublgc->aucName);

        memcpy(puc+1, psublgc->aucName, *puc);
        puc+=*puc+1;

        *puc++=0;
        *puc++=0;

        *puc++=psublgc->bRun?1:0;
    }
    assert(puc-pucFuncBuf_g==ulFuncLen_g);

    return EP_SUCCESS;
}

/***********************************************************************
* RC_File_Added - 若录波文件数超过最大值，则删除最久的录波文件
*
* RETURNS: 无
*
*/
static void RC_File_Added(void)
{
    STATUS vxsts=FALSE;
    int nDataDiskLeftSize;

    iRecFileNum_g = lstCount(pmRecFileList_g);                    /* Normal increase total file number. */

    vxsts=GetDataDiskLeftSize(&nDataDiskLeftSize);

    if(vxsts == ERROR)
    {
        if (ENG_MODE == 1)
            LOG_Write(LOG_KERNEL, "ERROR, Get DATA Disk free space failure.\n", NULL);
        else if (ENG_MODE == 0)
            LOG_Write(LOG_KERNEL, "得到空闲空间失败.\n", NULL);
    }

    while ((iRecFileNum_g>rptsts_g.iMaxRecNum) || ((nDataDiskLeftSize<s_MinDataDiskSpace) && (vxsts == OK)))
    {
        if (!RC_DelOldRptFile())
        {
            return;
        }

        vxsts=GetDataDiskLeftSize(&nDataDiskLeftSize);

        if(vxsts == ERROR)
        {
            if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "ERROR, Get DATA Disk free space failure!\n", NULL);
            else if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "得到空闲空间失败!\n", NULL);
        }
    }
}

/***********************************************************************
* RC_MakeFileTaskExecHandle - 为了处理录波文件异常时的处理函数，张云添加
*
* RETURNS: 无
*
*/
static void RC_MakeFileTaskExecHandle(
    int iFd,			/* 文件描述符 */
    int iExecReason		/* 异常原因 */
)
{
    STATUS vxsts;
    int nDiskHdl;
    int nFreeSpace;
    static uint32_t ulCnt=0;

    ulCnt++;
    if(ulCnt%10 == 1)
    {
        static uint8_t aucLogInfo[256];
        sprintf(aucLogInfo, "WARNING, Make Record File failure because of resean No. %d!\n", iExecReason);		/* 记录出错原因 */
        /* LOG_Write(LOG_KERNEL, aucLogInfo, NULL); */
        if(iExecReason == 0)
        {
            if(ENG_MODE==0)
                LOG_Write(LOG_KERNEL, "前次录波文件写入还未结束,本次录波失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "the last record wave not finish, the current record failed.\n", NULL);
        }
        else if(iExecReason == 1)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "创建新的录波文件失败,本次录波失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "creat new record wave file failed.\n", NULL);
        }
        else if(iExecReason == 2)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "写新录波段段头失败,本次录波失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "writing new record wave header failed.\n", NULL);
        }
        else if(iExecReason == 3)
        {
            if(uWrRecDataErrSts&0x01)
            {
                if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "文件描述符无效,填写本录波块数据失败,本次录波失败.\n", NULL);
                else if(ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "Invalid file descriptor, the current record failed.\n", NULL);
            }
            else if(uWrRecDataErrSts&0x02)
            {
                if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "录波点数过多,填写本录波块数据失败,本次录波失败.\n", NULL);
                else if(ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "record too much point, the current record failed.\n", NULL);
            }
            else if(uWrRecDataErrSts&0x04)
            {
                if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "数据写入丢失,填写本录波块数据失败,本次录波失败.\n", NULL);
                else if (ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "date lost, the current record failed.\n", NULL);
            }
            else if(uWrRecDataErrSts&0x08)
            {
                if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "数据段写入文件失败,填写本录波块数据失败,本次录波失败.\n", NULL);
                else if(ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "write date failed, the current record failed.\n", NULL);
            }
        }
        else if(iExecReason == 4)
        {
            if(ENG_MODE == 0)
                LOG_Write(LOG_KERNEL, "结束本录波段失败,本次录波失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "end current record failed.\n", NULL);
        }
        else if(iExecReason == 5)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed.\n", NULL);
        }
        else if(iExecReason == 6)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,添加标志集失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, add marking failed.\n", NULL);
        }
        else if(iExecReason == 7)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,添加定值失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, add setting failed.\n", NULL);
        }
        else if(iExecReason == 8)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,添加压板失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, add switch failed.\n", NULL);
        }
        else if(iExecReason == 9)
        {
            if(ENG_MODE==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,添加保护功能投退失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, add protect function failed.\n", NULL);
        }
        else if(iExecReason == 10)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,更改临时文件名失败.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, change temporary file failed.\n", NULL);
        }
        else if(iExecReason == 11)
        {
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,录波信息长度不对.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, rec inf len is failure.\n", NULL);
        }
        else if(iExecReason == 12)
        {
            /*2013-8-20 ZY */
            if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "录波文件形成失败,本次录波失败,录波段数过多.\n", NULL);
            else if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "record file generate failed, rec piece num is too much.\n", NULL);
        }
    }

    if(iFd >= 0)
    {
        static uint32_t ulCnt=0;
        ulCnt++;
        close(iFd);
        if (FT_Is_File(EP_WAVE_REC_DIR "/edptmp.frw.bak"))
        {
            remove(EP_WAVE_REC_DIR "/edptmp.frw.bak");
        }

        if(ulCnt%10==1)
        {
            /* 每10报错1次,第1次必须报 */
            if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "WARNING, Make Record File failure for busy or no space.\n", NULL);
            else if(ENG_MODE==0)
                LOG_Write(LOG_KERNEL, "告警,生成录波文件失败.\n", NULL);
        }
    }

    nDiskHdl= open( EP_DATA, O_RDONLY,0 );
    if(nDiskHdl==ERROR)
    {
        logMsg("ERROR,Open DATA  Disk  failure  for  error!\n",
               0, 0, 0, 0, 0, 0);
        if(ENG_MODE == 1)
            LOG_Write(LOG_KERNEL, "ERROR, Open DATA Disk failure for error.\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "打开date目录失败.\n", NULL);
        return;
    }

    // vxsts = ioctl(nDiskHdl, FIONFREE, (int)(&nFreeSpace) );
    vxsts = ERROR;
    struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出


    // 使用 fstatvfs 替换 ioctl(..., FIONFREE, ...)
    if (fstatvfs(nDiskHdl, &disk_stats) == 0) { // 成功返回 0
        // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
        nFreeSpace = (int)disk_stats.f_bavail * disk_stats.f_frsize;
        
        printf("Linux fstatvfs: Available space is %llu bytes.\n", nFreeSpace);
        vxsts = OK;
    } 


    if(vxsts == ERROR)
    {
        logMsg("ERROR,Get DATA Disk  free space  failure  for  error!\n",
               0, 0, 0, 0, 0, 0);
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR, Get DATA Disk free space failure for error.\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "打开data目录失败.\n", NULL);
        close(nDiskHdl);
        return;
    }

    while(nFreeSpace<s_MinDiskSpace)
    {
        /*若磁盘空间不够,则删除原来的老的文件  */
        if(iRecFileNum_g>0)
        {
            static uint32_t ulDelCnt=0;
            if (!RC_DelOldRptFile())
            {
                close(nDiskHdl);
                return;
            }

            ulDelCnt++;
            if(ulDelCnt%10 == 1)
            {
                /* 每10报错1次,第1次必须报 */
                logMsg("warning,DATA Disk free space isn't enough,so del one rec file!\n",
                       0, 0, 0, 0, 0, 0);
            }
            // vxsts = ioctl(nDiskHdl, FIONFREE, (int)(&nFreeSpace) );
            vxsts = ERROR;
             struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出


    // 使用 fstatvfs 替换 ioctl(..., FIONFREE, ...)
            if (fstatvfs(nDiskHdl, &disk_stats) == 0) { // 成功返回 0
                // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
                nFreeSpace = (int)disk_stats.f_bavail * disk_stats.f_frsize;
                
                printf("Linux fstatvfs: Available space is %llu bytes.\n", nFreeSpace);
                vxsts = OK;
            } else {
                perror("fstatvfs failed");
                vxsts = ERROR;
            }
            if(vxsts == ERROR)
            {
                logMsg("ERROR,Get DATA Disk  free space  failure  for  error!\n",
                       0, 0, 0, 0, 0, 0);
                if(ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "ERROR, Get DATA Disk free space failure for error.\n", NULL);
                else if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "获取data目录空闲空间失败.\n", NULL);
                close(nDiskHdl);
                return;
            }
        }
        else
        {
            /*若无文件删除,则退出循环  */

            static uint32_t ulCalcCnt=0;

            ulCalcCnt++;
            if(ulCalcCnt%10 == 1)
            {
                /* 每10报错1次,第1次必须报 */

                logMsg("warning,DATA Disk free space isn't enough,but no rec file to  del!\n",
                       0, 0, 0, 0, 0, 0);
            }

            close(nDiskHdl);
            return;
        }
    }

    close(nDiskHdl);

    rptsts_g.bRecWrFileOn=FALSE;
    rptsts_g.bEvtWrFileOn=FALSE;		/* 当前没有文件形成 */
}

/***********************************************************************
* RC_Start_Lubo_Sample - 启动即时录波采样信息
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Start_Lubo_Sample(
    char *strRtLuboFileName,		/* 供返回的录波报告文件名字符串，参数字符串空间由调用方分配，本函数填充返回值。 */
    uint8_t *pucRtLuboSN		/* 供返回的录波段号，参数由调用方分配，本函数填充返回值。 */
)
{
    SCI_LUBO_START_INFO_TYPE LuboStartInfo;
    uint16_t unRptSN;
    LuboStartInfo.unForwardLuboTime=0;			/* 前向录波时间为0 */
    LuboStartInfo.ucBackwardLuboTimeType=0;			/* 录波定长时间 */
    LuboStartInfo.ulBackwardLuboTime=MAX_LUBO_SAMPLE_MS;				/* 后向录波持续时间 */
    LuboStartInfo.unLuboFreq=uiAiRate_g;					/* 录波频率等于AI频率 */

    unRptSN=rptsts_g.unRptSN;  		/* 当前报告和录波号 */

    sprintf(strRtLuboFileName, EP_WAVE_REC_DIR "/edp%04x.frw", unRptSN);
    *pucRtLuboSN=rptsts_g.ucRecSN;
    bOnLuboSample_g=TRUE;

    SCI_Set_Lubo_Start_Flag(&LuboStartInfo,RD_AI_Cnt());			/* 启动录波 */

    return EP_SUCCESS;
}

/***********************************************************************
* GetRecBufTaskStatus - 获取录波任务状态
*
* RETURNS: 无
*
*/
BOOL GetRecBufTaskStatus()
{
    /* 获得Recbuf_Task的状态,若正常,则返回真,否则,返回假 */
    static char strTaskStatus[128];

    if(taskIdVerify(nRecBufTaskID_g)==ERROR)
    {
        /* 首先判定该任务是否有效 */
        return FALSE;
    }
    taskStatusString(nRecBufTaskID_g,strTaskStatus);

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* GetRecFileTaskStatus - 获取录波文件写入任务状态
*
* RETURNS: 无
*
*/
BOOL GetRecFileTaskStatus()
{
    /* 获得RecFile_Task的状态,若正常,则返回真,否则,返回假 */
    static char strTaskStatus[128];

    if(taskIdVerify(nRecFileTaskID_g)==ERROR)
    {
        /* 首先判定该任务是否有效 */
        return FALSE;
    }
    taskStatusString(nRecFileTaskID_g,strTaskStatus);

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/* 删除部分录波文件
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
static BOOL RC_DelOldRptFile(void)
{
    int nRecFileCnt;
    int nSlowDelRecFileCnt = 0;
    int32_t iAllowCnt;	/* 每次删除允许保留的个数 */
    FILENODE *pFileNode;	/* 文件节点指针 */
    FILENODE *pDelFileNode = NULL;	/* 待删除文件节点指针 */

    semTake(semRecFileListWR_g, WAIT_FOREVER);
    nRecFileCnt = lstCount(pmRecFileList_g);

    /* 实际文件数更新全局变量 */
    iRecFileNum_g = nRecFileCnt;

    /*
     * 删除文件数为录波文件夹当前文件数的1/4
     */

    iAllowCnt = iRecFileNum_g*3/4;

    /* 获取慢速删除个数 */
    for (pFileNode = (FILENODE *)lstFirst(pmRecFileList_g); pFileNode != NULL;
            pFileNode = (FILENODE *)lstNext((NODE *)pFileNode))
    {
        if (pFileNode->ucDelAttr == 0x00)
        {
            nSlowDelRecFileCnt++;
        }
    }

    /* 实际文件数大于允许值时,进行多次删除
     * 慢速删除文件大于给定值
     * 删除最后一个报告即可(可能是慢速,或快速报告)
     */
    for (pFileNode = (FILENODE *)lstLast(pmRecFileList_g);
            (pFileNode != NULL) && (iRecFileNum_g>iAllowCnt) && (nSlowDelRecFileCnt>MIN_SLOW_DELETE_NUM);)
    {
        pDelFileNode = pFileNode;
        pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);
        if (pDelFileNode->ucDelAttr == 0x00)
        {
            nSlowDelRecFileCnt--;
        }
        FS_RemoveFile(pDelFileNode->ucFullFileName, REC_DIR);
        iRecFileNum_g--;
    }

    /* 删除最后快速文件 */
    for (pFileNode = (FILENODE *)lstLast(pmRecFileList_g);
            (pFileNode != NULL) && (iRecFileNum_g>iAllowCnt);)
    {
        pDelFileNode = pFileNode;
        pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);
        if (pDelFileNode->ucDelAttr == 0x01)
        {
            FS_RemoveFile(pDelFileNode->ucFullFileName, REC_DIR);
            iRecFileNum_g--;
        }
    }

    /* 实际文件数大于允许值时,进行多次删除
     * 经前两步删除后仍不能满足要求
     * 则逐一删除最后一个报告(可能是慢速,或快速报告)
     */
    for (pFileNode = (FILENODE *)lstLast(pmRecFileList_g);
            (pFileNode != NULL) && (iRecFileNum_g>iAllowCnt);)
    {
        pDelFileNode = pFileNode;
        pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);
        FS_RemoveFile(pDelFileNode->ucFullFileName, REC_DIR);
        iRecFileNum_g--;
    }

    semGive(semRecFileListWR_g);

    if (iRecFileNum_g == nRecFileCnt)
    {
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
* creatInDataDisk - 在DATA盘中创建文件,是对标准CREAT函数的重载,首先进行磁盘空间检测
*
* RETURNS: OK, or ERROR
*
*/
int creatInDataDisk
(
    const char *name, 		/* name of the file to create */
    int flag  		/* O_RDONLY, O_WRONLY, or O_RDWR */
)
{
    STATUS vxsts;
    int nDiskHdl;
    int nFreeSpace;

    nDiskHdl= open( EP_DATA, O_RDONLY,0 );
    if(nDiskHdl==ERROR)
    {
        logMsg("ERROR,Open DATA  Disk  failure  for  error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR,Open DATA Disk failure  for  error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "打开date目录失败!\n", NULL);
        return ERROR;
    }

    // vxsts = ioctl(nDiskHdl, FIONFREE, (int)(&nFreeSpace) );
    vxsts=ERROR;
     struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出
    // unsigned long long nFreeSpace = 0; 
    // STATUS vxsts=ERROR;

    // 使用 fstatvfs 替换 ioctl(..., FIONFREE, ...)
    if (fstatvfs(nDiskHdl, &disk_stats) == 0) { // 成功返回 0
        // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
        nFreeSpace = (int)disk_stats.f_bavail * disk_stats.f_frsize;
        
        printf("Linux fstatvfs: Available space is %llu bytes.\n", nFreeSpace);
        vxsts = OK;
    } else {
        perror("fstatvfs failed");
        vxsts = ERROR;
    }

    if(vxsts==ERROR)
    {
        logMsg("ERROR,Get DATA Disk  free space  failure  for  error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR,Get DATA Disk  free space  failure  for  error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "获取data目录空闲空间失败!\n", NULL);
        close(nDiskHdl);
        return ERROR;
    }

    close(nDiskHdl);

    if(nFreeSpace>=s_MinWrDiskSpace)
    {
        return creat(name, flag);
    }
    else
    {
        return ERROR;
    }
}

/***********************************************************************
* writeInDataDisk - 在DATA盘中写文件,是对标准write函数的重载,首先进行磁盘空间检测
*
* RETURNS: OK, or ERROR
*
*/
int writeInDataDisk
(
    int    fd,     /* file descriptor on which to write */
    char * buffer, /* buffer containing bytes to be written */
    size_t nbytes  /* number of bytes to write */
)
{
#define ONE_WRITE_SIZE 512
    STATUS vxsts;
    int nDiskHdl;
    int nFreeSpace;

    nDiskHdl= open( EP_DATA, O_RDONLY,0 );
    if(nDiskHdl==ERROR)
    {
        logMsg("ERROR,Open DATA  Disk  failure  for  error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE==1)
            LOG_Write(LOG_KERNEL, "ERROR,Open DATA Disk failure  for  error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "打开data目录失败!\n", NULL);
        return ERROR;
    }
    // vxsts = ioctl(nDiskHdl, FIONFREE, (int)(&nFreeSpace) );

    vxsts=ERROR;
     struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出
    // unsigned long long nFreeSpace = 0; 
    // STATUS vxsts=ERROR;

    // 使用 fstatvfs 替换 ioctl(..., FIONFREE, ...)
    if (fstatvfs(nDiskHdl, &disk_stats) == 0) { // 成功返回 0
        // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
        nFreeSpace = (int)disk_stats.f_bavail * disk_stats.f_frsize;
        
        printf("Linux fstatvfs: Available space is %llu bytes.\n", nFreeSpace);
        vxsts = OK;
    } else {
        perror("fstatvfs failed");
        vxsts = ERROR;
    }

    if(vxsts==ERROR)
    {
        logMsg("ERROR,Get DATA Disk  free space  failure  for  error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE==1)
            LOG_Write(LOG_KERNEL, "ERROR,Get DATA Disk  free space  failure  for  error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "获取data目录空闲空间失败!\n", NULL);
        close(nDiskHdl);
        return  ERROR;
    }

    close(nDiskHdl);

    if(nFreeSpace>=nbytes+s_MinWrDiskSpace)
    {
        /* 若块太大,使得能多次读写,从而提供CPU调度得可能 */
        int iNotWritedSize;
        iNotWritedSize=nbytes;
        while(iNotWritedSize>=ONE_WRITE_SIZE)
        {
            if(write(fd,buffer,ONE_WRITE_SIZE)!=ONE_WRITE_SIZE)
            {
                if(ENG_MODE ==1)
                    LOG_Write(LOG_KERNEL, "ERROR, the writing to DATA Disk fail!\n", NULL);
                else if(ENG_MODE ==0)
                    LOG_Write(LOG_KERNEL, "写入data目录失败!\n", NULL);
                return ERROR;
            }
            buffer=buffer+ONE_WRITE_SIZE;
            iNotWritedSize=iNotWritedSize-ONE_WRITE_SIZE;
        }

        if(write(fd,buffer,iNotWritedSize)!=iNotWritedSize)
        {
            if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "ERROR, the writing to DATA Disk fail!\n", NULL);
            else if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "写入data目录失败!\n", NULL);
            return ERROR;
        }
        else
        {
            return nbytes;/*写成功  */
        }
    }
    else
    {
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR, the free space of DATA Disk is too small!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "data目录剩余空间太小!\n", NULL);
        return ERROR;
    }
}

/***********************************************************************
* GetDataDiskLeftSize - 获取Data盘剩余空间大小
*
* RETURNS: OK, or ERROR
*
*/
STATUS GetDataDiskLeftSize(
    int *piSize		/* 剩余空间大小 */
)
{
    STATUS vxsts;
    int nDiskHdl;

    nDiskHdl=open( EP_DATA, O_RDONLY, 0);
    if(nDiskHdl == ERROR)
    {
        logMsg("ERROR, Open DATA Disk failure for error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR, Open DATA Disk failure for error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "打开data目录失败!\n", NULL);

        return ERROR;
    }

    // vxsts=ioctl(nDiskHdl, FIONFREE, (int)piSize);
    vxsts=ERROR;
    struct statvfs disk_stats;
    // 使用 64 位整型，因为现代文件系统空间很大，32位会溢出
    // unsigned long long nFreeSpace = 0; 
    // STATUS vxsts=ERROR;

    // 使用 fstatvfs 替换 ioctl(..., FIONFREE, ...)
    if (fstatvfs(nDiskHdl, &disk_stats) == 0) { // 成功返回 0
        // 使用 f_bavail 和 f_frsize 计算对普通用户可用的空间
        *piSize = (int )disk_stats.f_bavail * disk_stats.f_frsize;
        
        // printf("Linux fstatvfs: Available space is %llu bytes.\n", nFreeSpace);
        vxsts = OK;
    } else {
        perror("fstatvfs failed");
        vxsts = ERROR;
    }
    if(vxsts == ERROR)
    {
        logMsg("ERROR, Get DATA Disk free space failure for error!\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR, Get DATA Disk free space failure for error!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "获取data空闲空间失败!\n", NULL);
        close(nDiskHdl);

        return ERROR;
    }

    close(nDiskHdl);

    return OK;
}

/***********************************************************************
* GetRecWrSts - 获取录波状态
*
* RETURNS:
*                TRUE，正在录波
*                FALSE，录波结束
*
*/
BOOL GetRecWrSts(void)
{
    return rptsts_g.bRecWrFileOn;
}

/***********************************************************************
* GetEvtWrSts - 获取事件形成状态
*
* RETURNS:
*                TRUE，正在形成
*                FALSE，结束
*
*/
BOOL GetEvtWrSts(void)
{
    return rptsts_g.bEvtWrFileOn;
}

/* 录波测试函数 */
void RC_Test(uint32_t ulForWardTime)
{
    SCI_LUBO_START_INFO_TYPE LuboStartInfo;

    LuboStartInfo.unForwardLuboTime = 5*MAX_LUBO_SAMPLE_MS; /* 前向录波时间 */
    LuboStartInfo.ucBackwardLuboTimeType = 0;	/* 录波定长时间 */
    LuboStartInfo.ulBackwardLuboTime = (uint16_t)ulForWardTime;	/* 后向录波持续时间 */
    LuboStartInfo.unLuboFreq = uiAiRate_g;	/* 录波频率等于AI频率 */

    SCI_Set_Lubo_Start_Flag(&LuboStartInfo, RD_AI_Cnt()); /* 启动录波 */

    rptsts_g.unRptSN++;

    return;
}
