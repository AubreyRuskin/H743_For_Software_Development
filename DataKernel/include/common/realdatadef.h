
/* realdatadef.h - This file contains programs to manager common realtime data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 11nov06, dy change the style of program and separate to three file: realdata.h, hwcfg.h, dspai.h.
01c, 26jul03, hdx Updated to version 1.0.
01b, 27feb03, hdx Updated to version 0.2.
01a, 26jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains programs to manager common realtime data.
*/

#ifndef REALDATADEF_H
#define REALDATADEF_H

/* includes */

//#include  "vxWorks.h"
#include  "edpbase.h"


#ifdef  __cplusplus
extern "C" {
#endif

/* defines */

#define MAX_PART_NUM 32
#define CPU_04_ADDR 200			/* CPU地址 */

#define REDUN_CPU_04_ADDR 201		/* 目前励磁使用*/

#define CPU_AO_ADDR 100   /*AO地址 */

#ifdef EDP02_GTP_BUILD /* 发变组保护 */
#define MAX_MOD_NUM 3              /* 每机箱中最多IO子模件个数 ，增加了主板上的开入子模件，16改为11，加快启动时间 */
#else
#define MAX_MOD_NUM 2              /* 每机箱中最多IO子模件个数 ，增加了主板上的开入子模件，16改为11，加快启动时间 */
#endif

#if defined(EDP02_PSR_BUILD)
#define MAX_DO_PER_MOD 320     /* 每子模件中最多DO通道数 */
#define MAX_DI_PER_MOD  320              /* 每子模件中最多DI通道数 */
#else
#define MAX_DO_PER_MOD 16     /* 每子模件中最多DO通道数 */
#define MAX_DI_PER_MOD 16              /* 每子模件中最多DI通道数 */

#endif

#define   MAX_OPT_AI_NUM   40   /* 光纵AI的最大数目 */
#define   MAX_OPT_AO_NUM   40   /* 光纵AO的最大数目 */

#define MAX_VTBOX_AI_NUM 64   /* 虚拟机箱AI的最大数目 */
#define MAX_VTBOX_AO_NUM 64   /* 虚拟机箱AO的最大数目 */

#ifdef EXCITE_BUILD
#define MAXSAMPPOINT 36					/* 由FPGA决定 */
#elif defined(EDP03_BUILD)
#define MAXSAMPPOINT 96				/* 周波最大采样点数 */
#else
#define MAXSAMPPOINT 128				/* 周波最大采样点数 */
#endif

#define HDL_DI_MAX_RECV_NUM 32

/* typedefs */

typedef enum	/* Define of submodule type */
{
    IDLE_MODULE=0x00,   /* Unused module */
    DO_MODULE=0x11,   /* Digit output module */
    DI_MODULE=0x12,           /* Digit input module */
    DIO_MODULE=0x13,     /* Digit input and output module */
    AI_MODULE=0x14,   /* Analog input module */
    AO_MODULE=0x15,         /* Analog Output module */
    CKDIO_MODULE=0x16,   /* Cekong used Digit input and output module */
    COM_MODULE = 0x17   /* slow changed analog and multi-IO module */
} SUB_MOD_TYPE;

typedef struct
{
    /*张云为光纵添加,光纵通道状态定义  2006-7-28日张云修改*/
    int32_t lTsse;					/* 采样时刻差,微秒 */
    BOOL bValid;				/* 通道数据采样同步有效与否， */
    BOOL bComStable;								/* 通讯稳定与否 */
    BOOL bComValid;					/* 通道通信有效与否，即数据接收正常与否 */
    BOOL  bDataIsCredible;  /* 接收数据是否可信  2009-3-9 ZY*/
    int  iRcvSndDiffChgTime;  /* 前后两次通信稳定状态变化期间，通道的收发时间差变化值，单位US
                                 Value=本次稳定时真实收发时间差-上次稳定时真实收发时间差
                                 若为0，表示没有发生变化，或无法判定
                                 若非0，表示此次判定出来的收发时间差的变化值  2009-2-13 ZY*/
} OPT_CH_STS;

typedef struct		/* status of the virtual box. */
{
    int32_t lTsse;					/* sample time, us */
    BOOL bValid;				/* if valid of channel data sampling synchronization */
    BOOL bComStable;								/* If the communication is stable.  */
    BOOL bComValid;					/* If the communication is valid. */
} VIRT_BOX_CH_STS;

typedef struct		/* 模件信息结构 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t ucType;
    uint8_t ucPosition;
    uint8_t aucHwAddr[4];
    uint8_t ucAiNum;
    uint8_t ucAoNum;  /* 为光纵和励磁添加AO的个数 */
    uint8_t ucDiNum;
    uint8_t ucDoNum;
    uint16_t unAiSmplRate;
    uint16_t unAiPts;				/* 周波采样点数 */
    uint32_t ulDIModCurVaule;			/* DI模件的当前值，只对DI摸件有效 */
    uint8_t iVtBoxSeqNo;				/* 序号 */
    uint8_t ucVtBoxPos;		/* 虚拟机箱位置编号，从0开始 */
    uint8_t iVtBoxAddrInSameKind;			/* 地址，供驱动程序使用 */
    uint8_t aucVtBoxFd[MAX_ID_LEN+1];			/* 所位于虚拟机箱的设备描述符 */
    uint8_t ucAiModType;		/* 交流模件类型 */
    uint32_t servertype;	/* 服务类型 */
} RD_PART_INFO;

typedef struct		 /* 读取整个模拟量缓冲区的结构 */
{
    int iLgcNum; 				/* 逻辑通道 */
    int iCalcNum; 		/* 预处理通道 */
    int iMsucNum; 				/* 测量通道数 */
    int iTxCalcNum; 		/* 扩展机箱有关 */
    int iDcAiNum;                /* DC number. */
    uint32_t ulNextCnt;              /* Next(not writen) AI Count. */
    uint32_t ulHeadClk;        /* This, dealed sample clock. */
    float *pfDbBgn;
    COMPLEX *pxDbBgn; 			/* 开始 */
    float *pfWork;
    COMPLEX *pxWork; 			/* 当前 */
    COMPLEX *pxmsuDbBgn; 				/* 测量结果存储地址 */
    float *pfDcDbBgn;		/* 直流模拟量 */
    float *pfDcWork;
    BOOL *pbAiValidDbBgn;   			/* AI数据有效性缓冲指针,张云添加 */
    BOOL *pbAiValidDbWork; 				/* 当前 */
    BOOL *pbAiValidDbEnd;  /* 结束位置 */
    /*光纵通道状态DB  */
    OPT_CH_STS   *pOptChStsDbBgn;
    OPT_CH_STS   *pOptChStsDbWork; /*这是和正常的通道同步的，而不是实际要写的地址(需要进行往前推算),只对光纵通道1和光纵通道2分配  */
    OPT_CH_STS *pOptChStsDbEnd;   /* 结束位置 */
    uint32_t     ulOptRefreshedCnt;  /*光纵数据刷新完成后的AICNT  */
    uint32_t     ulOptFastTaskMatchCnt;/*光纵数据刷新完成后的对方快速任务中间结果对应的AICNT  2006-11-12日张云*/
    /* 虚拟机箱通道状态DB */
    VIRT_BOX_CH_STS *pVirtChStsDbBgn;
    VIRT_BOX_CH_STS *pVirtChStsDbWork; /* 这是和正常的虚拟机箱通道同步的, 而不是实际要写的地址(需要进行往前推算), 只对虚拟机箱同步 */
    uint32_t ulVirtRefreshedCnt;  		/* 虚拟机箱数据刷新完成后的AICNT */
    uint32_t ulVirtFastTaskMatchCnt;			/* 虚拟机箱数据刷新完成后的对方快速任务中间结果对应的AICNT */
    uint8_t aucVtBoxFd[MAX_ID_LEN+1];			/* 所位于虚拟机箱的设备描述符 */
    uint8_t iVtBoxSeqNo;		/* 虚拟机箱位置编号，从0开始 */
    uint8_t iVtBoxAddrInSameKind;				/* address in a kind of virtual box.*/
    void *pBoxDrv;		/* virtual box driver. */
    uint32_t ulClkDiff;
    uint32_t ulSndCnt;		/* 发送计数*/
    uint32_t ulRestCout;
    int8_t ucRltDif;
} RD_AI_MOD;

typedef struct			/* 硬件通道配置 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    float fMaxVal;
    float fMinVal;
    RD_AI_MOD *paimod;
    RD_PART_INFO *p_part;   /* module information. */
    uint8_t ucModCh;
    float fCoff;		/* CT变比系数 */
    uint32_t uFstRatedVal;			/* 一次额定值,电压单位V,电流单位A */
    uint16_t uSecRatedVal;			/* 二次额定值 */
    float fOriginCoff;			/* 保存原始配置值 */
    float fSetCoff;			/* 设定系数，源自硬件配置或软件配置 */
    float fAmendCoff;
    float fGain;			/* 增益系数 */
    float fRatio;
    uint8_t ucUnit;
    uint8_t OptAOEnable;		/* 设置光纵量 */
    int iSmplCh; 			/* 瞬时值存储通道 */
    uint8_t FactorSetModWord;				/* 配置参数设置方式字 */
    uint8_t MaxValueDingzhiTagBase[MAX_ID_LEN+1];				/* 逻辑标志字符串 */
    uint8_t MinValueDingzhiTagBase[MAX_ID_LEN+1];
    uint8_t ScalefactorDingzhiTagBase[MAX_ID_LEN+1];
    uint8_t OptAONameString[MAX_ID_LEN+1];
    float fExcCoff;					/* 零点偏移值 */
    BOOL bSetOptAo;    /* 设置为光纵AO */
    uint8_t aucAoId[MAX_ID_LEN+1];		/* 光纵引用逻辑标志 */
    BOOL bSetAo;    /* 设置为AO */
    uint8_t aucAoLogicId[MAX_ID_LEN+1];		/* AO引用逻辑标志 */
    uint8_t ucMmiShow;				/* 是否在人机界面上显示该通道值*/
    uint8_t ucVtBoxPos;		/* 虚拟机箱位置编号，从0开始 */
    uint32_t servertype;	/* 服务类型 */
    void *pSmv;  /* 所属smv配置通道,初始为NULL */
    uint8_t ucMstPortNum;  /* 所属本级CC板端口号,从0开始,0xFF表示无效 */
    uint8_t ucSlvPortNum;  /* 所属前级CC板端口号,从0开始,0xFF表示无效 */
    uint8_t ucChSrc;  /* 所属通信端口 */
    uint8_t arrSVID[MAX_ID_LEN+1];  /* 所对应的SVID */
    void  *   pAiHdl;		/*xiang add for STI used to get Q (HDL_AI_HND)*/

    BOOL bCalcMeaFlag; /* 通道模拟量计算标识 */
    float fSecToFstCoff;  /* 二次转一次系数 */
    float PropConf;  /* 转换系数 */
    float fTradSecToFstCoff; /* 二次转一次系数 */
    int32_t iIndexSn; /* 索引定值页序, 初始为-1 */
    float fThreshold; /* 监视输入基准和显示门槛 */
} RD_HW_AI_CH;


typedef struct			/* 逻辑通道配置 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint16_t unLgcSN;
    RD_HW_AI_CH *phwai;
    uint8_t ucFiltTp;
    uint8_t ucUnit;
    BOOL bRec;
    uint8_t aucRecId[MAX_ID_LEN+1];
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    BOOL bMea;
    uint8_t aucMeaId[MAX_ID_LEN+1];
    BOOL bSetAo;    /* 设置为AO. */
    uint8_t aucAoId[MAX_ID_LEN+1];		/* AO引用逻辑标志 */

    union
    {
        /* 数据有可能是实数，也有可能是复数，联合 */
        float *pfLgcAI;
        COMPLEX *pxCalcAI;
    } pdat;
} RD_LGC_AI_CH;

#ifdef  __cplusplus
}
#endif
#endif                                  /* HWCFG_H */
