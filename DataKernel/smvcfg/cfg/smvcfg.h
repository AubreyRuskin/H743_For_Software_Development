/*******************************************************************************

* Copyright (c) 2009,南自电网数字化事业部

* All rights reserved.

文件名         : smvcfg.h

相关文件       :

功能           :

作者           : 任广宇

版本           : 0.1

--------------------------------------------------------------------------------

修改记录 :

日  期          版本            修改人          修改内容

2009/09/18      0.1             任广宇          创建

********************************************************************************/


#ifndef	_SMVCFG_H
#define _SMVCFG_H


#include "dsp.h"
#include "mxml.h"
#define SMV_9_1_CHANNUM		1
#define SMV_AD_CHANNUM		1
#define SMV_FT3_CHANNUM		6
#define SMV_SLF_CHANNUM		50
#define SMV_TX_CHANNUM		2

#define SMV_CFG_FILE		"/tffs/smv.xml"
#define SMV_EXCFG_FILE		"/tffs/excfg.xml"


#include "vxworks_type.h"

/* 配置文件有可能同时出现配置多项的情况，如同时SMV_AD和SMV_FT3，或者同时要求SMV_PUB，平台应用需要知道配置了哪些内容，然后去针对性的读取 */
/* E02、E03原有保留，未变 */
typedef struct
{
    int    smvAdsuNo;      /* 报文ASDU号，十进制，结果 -1 */
    int    smvAdsuChn;     /* 报文该ASDU号中的通道号，十进制，结果 -1 */
    int    smvDataChn;     /* 该通道对应于硬件配置文件中的内序号，十进制，结果 -1*/
    int32_t smvValIn;	   /* 用于9_2码值转换，对应于额定值 */
    void *phwaich; /* 指向物理通道 */
    int16_t smvValOut;	   /* 用于9_2码值转换，对应于额定值 */
    uint8_t   MuYabanIDStr[33];       /*关联的MU压板ID，2013-6-7 ZY */
    int        MuLinkNo;				/*MU投入压板号*/
    BOOL   MuLinkUSE;
    uint16_t	smvChnType;			/*CHN_MAP通道类型*/
    uint16_t	MuTypeNo;
    char   *smvDes;        /* 描述，字符串 */

    int smvAsduNoInPack;  /* 在包中的相对序号,从0开始 */
    int PackNo;  /* 包号 */
    int smvNum;  /* smv.xml中DATASET结构的序号,从0开始 */
} IEC_SMV_CHAP;

typedef struct
{
    uint8_t  smvID;        /*配置的9-1序号，结果 -1*/
    uint8_t  smvPortChn;   /* 9_2中NET端口序号，十进制，结果-1 */
    uint8_t  smvSrc[6];    /* MUL_SRC:后的MAC地址*/
    uint16_t appID;        /*APP_ID:后的APPID值，十进制（最好搞成十六进制并按十六进制解析）*/
    uint8_t dataNum;       /*本9-1中对应有几组CHN_MAP:*/
    uint8_t receiveType;   /* TYPE:后的值*/
    uint8_t asduNum;       /* ASDU_NUM:后的值*/
    uint8_t oppLineType[2];       /* ASDU_NUM:后的值*/
    uint16_t smprate9_2;//数据源采样率
    BOOL    forceSyn;//强制同步
    BOOL    synPulse;//发送脉冲
    uint8_t    DDA;//DICRECT DATA ACCESS 数据直接对应模式，优化效率
    IEC_SMV_CHAP smvData[MAXHCHNNUM];
} IEC_SMV_9_1_CFG;

typedef struct
{
    uint16_t smvNum;      /*共有几组<SMV_9_1Cfg>~~~<SMV_9_1Cfg/>*/
    IEC_SMV_9_1_CFG Smv_9_1Cfg[SMV_9_1_CHANNUM];
} IEC_SMV_CFG;

extern IEC_SMV_CFG    gSmvCfg;   /*最终存放的结构体名*/


/* AD解析文件存放结构体 */
typedef struct
{
    int    smvChn;          /* FPGA中AD序号，十进制，结果-1 */
    float  smvChnFactor;    /* FPGA中该AD比差因子，正常配“1.00” */
    int    smvChnAng;       /* FPGA中该AD调整角度，单位“分”，支持负数 */
    int    smvChnZeroCur;   /* 是否参与零序电流计算 0：不参与，1：参与*/
    int    smvDataChn;      /* FPGA中该AD通道对应的硬件配置文件中的内序，结果-1 */
    char   *smvDes;         /* 描述 */
} IEC_SMV_AD_CHAP;

typedef struct
{
    uint8_t smvID;          /* AD配置文件序号，结果 -1*/
    uint8_t smvWaitPoints;  /* AD采样等待点数 */
    uint8_t smvZeroCurType; /* 零序电流类型，1：自产，2：通道 */
    uint8_t dataNum;        /* 本AD中对应有几组CHN_MAP配对关系 */
    IEC_SMV_AD_CHAP smvData[MAXHCHNNUM];
} IEC_SMV_AD_CFG;

typedef struct
{
    uint16_t smvNum;         /* 共有几组 */
    IEC_SMV_AD_CFG Smv_AD_Cfg[SMV_AD_CHANNUM];
} IEC_SMV_AD_POOL;


/* FT3解析文件存放结构体 */
typedef struct
{
    int    smvChn;          /* FT3中SMV序号，十进制，结果-1 */
    int    smvDataChn;      /* FT3中该SMV通道对应的硬件配置文件中的内序，结果-1 */
    int        MuLinkNo;				/*MU投入压板号*/
    BOOL   MuLinkUSE;
    char   *smvDes;         /* 描述 */
} IEC_SMV_FT3_CHAP;

typedef struct
{
    uint8_t smvID;          /* FT3配置文件序号，结果 -1*/
    uint8_t smvPortChn;      /* FT3中SR端口序号，十进制，结果-1 */
    uint8_t smvWaitPoints;  /* FT3采样等待点数 */
    uint8_t smvType;        /* FT3类型，1：内部规约，2：标准 */
    uint8_t dataNum;        /* 本FT3中对应有几组CHN_MAP配对关系 */
    IEC_SMV_FT3_CHAP smvData[MAXHCHNNUM];
} IEC_SMV_FT3_CFG;

typedef struct
{
    uint16_t smvNum;         /*  */
    IEC_SMV_FT3_CFG Smv_FT3_Cfg[SMV_FT3_CHANNUM];
} IEC_SMV_FT3_POOL;

extern IEC_SMV_FT3_POOL gSmvFT3Cfg;   /*最终存放的结构体名*/

/* SLF解析文件存放结构体 */
typedef struct
{
    int    smvDataChnR1;          /* 零序来源通道1，十进制，结果-1 */
    int    smvDataChnR2;      /* 零序来源通道2，结果-1 */
    int    smvDataChnR3;		/* 零序来源通道3，结果-1 */
    int    smvDataChn;		/* 零序目标通道，结果-1 */
    int32_t smvValIn;	   /* 用于9_2码值转换，对应于额定值 */
    void *phwaich; /* 指向物理通道 */
    int16_t smvValOut;	   /* 用于9_2码值转换，对应于额定值 */
    char   *smvDes;         /* 描述 */
} IEC_SMV_SLF_CHAP;

typedef struct
{
    uint16_t smvNum;         /*  */
    IEC_SMV_SLF_CHAP Smv_SLF_Cfg[SMV_SLF_CHANNUM];
} IEC_SMV_SLF_POOL;

extern IEC_SMV_SLF_POOL gSmvSLFCfg;   /*最终存放的结构体名*/


/* SMV发送解析文件存放结构体 */
typedef struct
{
    int    smvDataChn;      /* SMV通道对应的硬件配置文件中的内序，结果-1 */
    int    smvTxDataChn;    /* 内序对应SMV发送的通道号，结果-1 */
    char   *smvDes;         /* 描述 */
} IEC_SMV_TX_CHAP;

typedef struct
{
    uint8_t smvID;          /* SMV TX配置文件序号，结果 -1*/
    uint8_t smvRate;        /* 采样率：每周波点数 */
    uint8_t smvTxType;      /* TX类型，1：无发送，2：9_1, 3：9_2LE, 4:FT3 */
    uint8_t smvAsduNum;     /* 发送报文ASDU数 */
    uint16_t RatedCurrent;   /* 额定相电流，9_1中使用 */
    uint16_t RatedZeroCurren;/* 额定中线电流，9_1中使用 */
    uint16_t RatedVoltage;   /* 额定电压，9_1中使用 */
    uint16_t RatedTime;      /* 延时，9_1中使用 */
    uint8_t dataNum;        /* 本TX中对应有几组CHN_MAP配对关系，即发送通道数 */
    IEC_SMV_TX_CHAP smvData[MAXHCHNNUM];
} IEC_SMV_TX_CFG;

typedef struct
{
    uint16_t smvNum;         /*  */
    IEC_SMV_TX_CFG Smv_TX_Cfg[SMV_TX_CHANNUM];
} IEC_SMV_TX_POOL;

extern IEC_SMV_TX_POOL gSmvTXCfg;   /*最终存放的结构体名*/


BOOL LoadSmvCfg(char *filename);

IEC_SMV_9_1_CFG *GetSmvChn(uint8_t* addr, uint16_t appID);

/********************************************************************/
/*使用e02_sgcfg.xml配置文件*/

/* uint32类型字节数 */
#define UINT32_BYTE_NUM 4

/* 数据源类型 */
#define DATA_SRC_STDFT3 0  /* 标准FT3 */
#define DATA_SRC_GRDFT3 1    /* 国网FT3 */
#define DATA_SRC_ADC 2  /* A/D采样 */
#define DATA_SRC_ADC_REPEAT 3  /* A/D复采 */
#define DATA_SRC_SMV 4  /* 标准9-2 */
#define DATA_SRC_SUM 5  /* 合流配置 */

/* PUB类型 */
#define SMV_SEND_STDFT3 0  /* 标准FT3 */
#define SMV_SEND_GRDFT3 1    /* 国网FT3 */
#define SMV_SEND_SMV92 2   /* 标准9-2接收 */

/* SUB通道类型 */
#define DATA_TYPE_PRO 1  /* 保护电流 */
#define DATA_TYPE_MEA 2    /* 测量电流 */
#define DATA_TYPE_VOL 3  /* 电压 */

#define SMV_DIGIT_SCP 463   /* 保护电流额定数字量 */
#define SMV_DIGIT_SCM 11585  /* 测量电流额定数字量 */
#define SMV_DIGIT_SV 11585  /* 电压额定数字量 */
#define SMV_MAX_DIGIT 32768   /* 16位有符号最大数字量 */
#define AD_MAX_INPUT 5000.0 /* A/D输入最大值,mV */

/* 应用类型 */
#define E03_MODE_640U 0  /* 低压保护 */
#define E03_MODE_INTEL_TERM 1    /* 智能终端 */
#define MODE_600U 0  /* 高压线路保护 */

/* 电平处理 */
#define SYN_REV_NORMAL 0  /* 正常 */
#define SYN_REV_REVERSE	1    /* 取反 */

/* 秒脉冲边沿处理 */
#define SYN_EDGE_RISE 1  /* 上升沿 */
#define SYN_EDGE_DROP 2     /* 下降沿 */

/* 守时 */
#define SYN_DEAL_NOR 1  /* 不守时 */
#define SYN_DEAL_FOL 2    /* 守时 */

/* 数据处理方式 */
#define DATA_DSP_RAW 1 /* 原始数据 */
#define DATA_DSP_ZERO 2  /* 滤零漂 */
#define DATA_DSP_INT 3  /* 积分 */

/* 寄存器地址 */

/* FPGA基址 */

#ifdef SMV_DEBUG
#define FPGA_MEM_ADRS ((volatile uint32_t *)sFpgaSimul.pSimulAddr)
#else
#define FPGA_MEM_ADRS ((volatile uint32_t *)(0xA2000000))
#endif

#define FPGA_HIGH_ADDR (UINT32_BYTE_NUM*0x5000)  /* FPGA最高地址(字节),用于测试时内存分配 */

/* FPGA功能数据块基址 */
#define GENERAL_REG_BASE 0x00000   /* 通用寄存器基址 */
#define SPI_REG_BASE 0x00100  /* SPI基址 */
#define AD_REG_BASE 0x00200   /* A/D寄存器基址 */
#define AD_DUP_REG_BASE 0x00300   /* A/D复采寄存器基址 */
#define FT3_0_REG_BASE 0x00400   /* FT3_0寄存器基址 */
#define FT3_1_REG_BASE 0x00500   /* FT3_1寄存器基址 */
#define FT3_T_0_REG_BASE 0x00600   /* FT3_T_0寄存器基址(发送) */
#define SV_R_0_REG_BASE 0x00700   /* 9-2 SV接收0寄存器基址 */
#define SV_R_1_REG_BASE 0x00800   /* 9-2 SV接收1寄存器基址 */
#define SV_REG_BASE 0x00900   /* 9-2 SV发送寄存器基址 */

/* 通用寄存器相对地址 */
#define GENERAL_CTRL_REG_0 0x00  /* 控制寄存器0 */
#define GENERAL_STATUS_REG_0 0x01  /* 状态寄存器0 */
#define GENERAL_STATUS_REG_1 0x02  /* 状态寄存器1 */
#define GENERAL_SEC_SYN_STATUS_REG_1 0x03  /* 秒脉冲同步状态寄存器 */
#define GENERAL_B_REG_0 0x04  /* B码寄存器0 */
#define GENERAL_B_REG_1 0x05  /* B码寄存器1 */

#define GENERAL_TIME_NS_REG 0x08

/* A/D及A/D复采寄存器 */
#define AD_CTRL_REG_0 0x00  /* 控制寄存器0 */
#define AD_STATUS_REG_0 0x01  /* 状态寄存器0 */
#define AD_ATTR_REG 0x02  /* 通道属性寄存器基址 */
#define AD_BD_ADDR 0x20   /* BD地址 */
#define AD_DATA_BUF 0x40   /* 数据缓冲区 */

/* FT3寄存器 */
#define FT3_CTRL_REG 0x00  /* 控制寄存器0 */
#define FT3_STATUS_REG 0x01  /* 状态寄存器0 */
#define FT3_STATUS_1_REG 0x02  /* 状态寄存器1 */
#define FT3_STATUS_2_REG 0x03  /* 状态寄存器2 */
#define FT3_STATUS_3_REG 0x04  /* 状态寄存器3 */
#define FT3_STATUS_4_REG 0x05  /* 状态寄存器4 */
#define FT3_BD_ADDR 0x20   /* BD地址 */
#define FT3_DATA_BUF 0x40   /* 数据缓冲区 */

/* SMV接收寄存器 */
#define SMV_CTRL_REG 0x00  /* 控制寄存器 */
#define SMV_MAC0_REG 0x01  /* MAC0 */
#define SMV_MAC1_REG 0x02  /* MAC1 */
#define SMV_STATUS0_REG 0x03  /* 状态寄存器0 */
#define SMV_STATUS1_REG 0x04  /* 状态寄存器1 */
#define SMV_STATUS2_REG 0x05  /* 状态寄存器2 */
#define SMV_BD_ADDR 0x20   /* BD地址 */
#define SMV_DATA_BUF 0x40   /* 数据缓冲区 */

/* FT3/SV发送寄存器 */
#define SV_CFG_ADDR 0x00   /* 配置寄存器 */
#define SV_LEN_ADDR 0x42   /* 长度寄存器 */
#define SV_ENABLE_ADDR 0x43   /* 端口使能寄存器 */
#define SV_DATA_BUF 0x60   /* 数据帧地址 */
#define FT3_ENABLE_ADDR 0x42  /* FT3端口发送使能寄存器 */

/* 数据缓冲区长度 */
#define AD_BUF_LEN 0x20  /* A/D */
#define FT3_BUF_LEN 0x20   /* FT3 */
#define SMV_BUF_LEN 0x20  /* 9-2 */

/* 通道数限定 */
#define FPGA_AD_SAM_CHN_NUM 24   /* A/D采样通道数 */
#define FPGA_AD_REPEAT_SAM_CHN_NUM 12   /* A/D复采通道数 */
#define FPGA_FT3_SAM_CHN_NUM 22   /* FT3采样通道数 */
#define FPGA_FT3_SND_CHN_NUM 22   /* FT3发送通道数 */
#define FPGA_SV_RCV_CHN_NUM 22   /* SV接收通道数 */
#define FPGA_SV_SND_CHN_NUM 22   /* SV发送通道数 */

/* 合流通道数 */
#define CHN_MERG_NUM 3

/* MAC地址长度 */
#define MAC_ADDR_LEN 6

/* 端口定义 */

#define AD_PORT_NO 0   /* A/D固定端口号 */
#define AD_REPEAT_PORT_NO 1   /* A/D复采固定端口号 */
#define FT3_PORT_NO_1 2    /* FT3端口1端口号 */
#define FT3_PORT_NO_2 3       /* FT3端口2端口号 */

#define SMV_PORT_NO_1 4   /* 9-2端口1端口号 */
#define SMV_PORT_NO_2 5     /* 9-2端口2端口号 */

#define SMV_RCV_PORT_NUM 2    /* 9-2接收端口个数 */
#define FT3_PORT_NUM 2    /* FT3接收端口个数 */

/* SUB/PUB数量 */
#define MAX_SUB_NUM 7    /* 最多允许7路接收: 2路9-2, 2路FT3, 1路A/D采样，1路A/D复采，1路合流通道 */
#define MAX_PUB_NUM 2   /* 最多两种发送配置,分别以9-2以及FT3格式发送 */

/* SUB/PUB最大通道数 */
#define MAX_CHN_NUM 40

/* svID等名称最大长度 */
#define MAX_CHAR_NUM 35

/* 最大缓冲数据块 */
#define MAX_BD_NUM 6

/* 最多ASDU数 */
#define MAX_ASDU_NUM 1

#define SMP_RATE_PER_CYCLE 80  /* 周波点数 */

/* 9-2帧相关长度 */
#define ETH_HEAD_LEN 0x12  /* 以太网帧头部 */
#define APDU_HEAD_LEN 0x11   /* APDU头部长度,从APPID开始,包含APPID,到ASDU SEQ TAG结束 */
#define APDU_2_HEAD_LEN 0x06   /* APDU头部长度_2,APDU TAG以下,不包括该TAG,到ASDU SEQ TAG结束 */
#define ASDU_HEAD_LEN 0x19   /* ASDU头部长度,从ASDU TAG到ASDU DATA TAG,不包括可变长度 */
#define ASDU_2_HEAD_LEN 0x16   /* ASDU头部长度_2,ASDU TAG以下,不包括该TAG,不包括可变长度 */

/* 标准FT3最大通道数 */
#define FT3_STD_MAX_CHN_NUM 12

/* 国网FT3最大通道数 */
#define FT3_GRD_MAX_CHN_NUM 22


/* 允许最多通道数,无内存问题 */

#define MAXCHNELS 96

/* 最多采样通道数
 * 最多1路A/D采样, 最多24通道: 1~24(0)
 * 最多复采通道数，最多12通道: 25~36(1)
 * 最多2路FT3数据, 每路22通道: 37~58(2)、59~80(3)
 * 最多2路9-2数据, 每路22通道: 81~102(4)、103~124(4)
 */

/* 以太网帧最大长度 */
#define ETHE_MAX_LEN 1518

/* FT3帧最大长度(标准: 48字节；国网: 64字节) */
#define FT3_MAX_LEN 64

/* 插值同步方式 */
#define INTERNAL_FPGA_CRYS 0  /* 本地FPGA晶振计数产生 */
#define INTERNAL_CPU_CRYS 1  /* 内部CPU秒脉冲 */
#define EXTERNAL_SEC_PULSE 2  /* 外部光秒脉冲 */
#define EXTERNAL_OPT_B_PULSE 3  /* 外部光B码 */
#define EXTERNAL_1588_0_PULSE 4  /* 外部1588对时脉冲0 */
#define EXTERNAL_1588_1_PULSE 5  /* 外部1588对时脉冲1 */
#define EXTERNAL_1588_2_PULSE 6  /* 外部1588对时脉冲2 */
#define EXTERNAL_1588_3_PULSE 7  /* 外部1588对时脉冲3 */
#define EXTERNAL_SEC_FROM_OPP_PULSE 8  /* 对侧CPU板秒脉冲*/
#define EXTERNAL_B_FROM_OPP_PULSE 9  /* 对侧CPU板B码 */
#define EXTERNAL_SEC_FROM_TDC_PULSE 10  /* TDC板秒脉冲 */
#define EXTERNAL_B_FROM_TDC_PULSE 11  /* TDC板B码 */

#define PULSE_MODE_MAX_NUM 12

/* 对时方式 */
#define INTERNAL_DEFAULT 0  /* 缺省 */
#define EXTERNAL_HMI_ADJ 1  /* 从HMI获取时间对时,同时通过秒脉冲修正 */
#define EXTERNAL_SEC_ADJ 2  /* 外部光秒脉冲 */
#define EXTERNAL_OPT_B_ADJ 3  /* 外部光B码 */
#define EXTERNAL_1588_0_ADJ 4  /* 外部1588对时脉冲0 */
#define EXTERNAL_1588_1_ADJ 5  /* 外部1588对时脉冲1 */
#define EXTERNAL_1588_2_ADJ 6  /* 外部1588对时脉冲2  FPGA实现*/
#define EXTERNAL_1588_3_ADJ 7  /* 外部1588对时脉冲3  FPGA实现*/
#define EXTERNAL_SEC_FROM_OPP_ADJ 8  /* 对侧CPU板秒脉冲 */
#define EXTERNAL_B_FROM_OPP_ADJ 9  /* 对侧CPU板B码 */
#define EXTERNAL_SEC_FROM_TDC_ADJ 10  /* TDC板秒脉冲 */
#define EXTERNAL_B_FROM_TDC_ADJ 11  /* TDC板B码 */

#define TIME_ADJ_MODE_NUM 12

/* 9-2标志位偏移 */
#define SMV_SYN_OFF 0 /* 同步偏移 */
#define SMV_TEST_OFF 1 /* 检修偏移 */
#define SMV_VALID_OFF 2 /* 有无效偏移 */

/* 9-1标志偏移 */
#define FT3_CHN_OFF 7   /* 状态字中通道分隔 */
#define FT3_VALID_OFF 5  /* 状态字中有无效标志位偏移 */
#define FT3_SYN_OFF 4  /* 同步位偏移 */
#define FT3_REPAIR_OFF 1  /* 检测位偏移 */

#define POINT_REJECT 0x01    /* 丢弃 */
#define POINT_SAVE 0x02   /* 保存 */
#define POINT_DRAW 0x04      /* 抽取 */

/* typedefs */

/* SUB通道 */
typedef struct
{
    UINT8 inputno;
    UINT8 inputno1;/*矢量和使用*/
    UINT8 inputno2;/*矢量和使用*/
    UINT8 inputno3;/*矢量和使用*/
    UINT8 hwcfgno;
    UINT8 chanel;
    UINT8 type;
    UINT16 rated;
    float coeff;
    UINT8 deal;
    char *desc;
    int32_t smvValIn; /* 用于9_2码值转换,对应于一次额定值 */
    int16_t smvValOut;	   /* 用于9_2码值转换，对应于额定值数字量或互感器输出小信号(mV) */
    BOOL MuLinkUSE;  /* 是否配置压板 */
    int16_t	iYabanNum;   /* 压板号 */
    INT16 phase;  /* 单通道相角调整 */
} CHN_SUB_INFO;

/* SUB信息 */
typedef struct
{
    UINT8 type;
    UINT8 port;
    INT16 phase;
    INT16 rcDly;  /* RC滤波延时 */
    BOOL bRdDlyCfg;  /* RC滤波延时配置标志 */
    INT16 dTChn;  /* 延迟时间通道 */
    INT16 dTValid;  /* 延迟时间通道是否有效 */
    UINT16 smpRate;
    UINT8 macAdr[MAC_ADDR_LEN];
    UINT16 appID;
    UINT8 chnNum;
    CHN_SUB_INFO chnSub[MAX_CHN_NUM];
} SMV_SUB_INFO;

/* PUB地址信息 */
typedef struct
{
    UINT32 port;
    UINT8 dstMac[MAC_ADDR_LEN];
    UINT8 srcMac[MAC_ADDR_LEN];
    UINT8 priority;
    UINT16 vlanID;
    UINT16 appID;
    UINT8 svID[MAX_CHAR_NUM];
    UINT8 security[MAX_CHAR_NUM];
} PUB_ADDRESS_INFO;

/* PUB通道信息 */
typedef struct
{
    UINT8 inputno;
    UINT8 outputno;
    UINT8 delayFlag;  /* 延时参数标志 */
} CHN_PUB_INFO;

/* PUB数据集 */
typedef struct
{
    UINT8 name[MAX_CHAR_NUM];
    UINT8 chnNum;
    CHN_PUB_INFO chnPub[MAX_CHN_NUM];
} PUB_DATASET_INFO;

/* PUB信息 */
typedef struct
{
    UINT8 type;
    UINT16 smpRate;
    UINT8 noASDU;
    UINT8 LNName;
    UINT8 DSName;
    UINT16 LDName;
    UINT32 confRev;
    UINT16 rtdPhsCur;
    UINT16 rtdNeucur;
    UINT16 rtdPhsVol;
    UINT16 rtdDlyTime;
    PUB_ADDRESS_INFO pubAddress;
    PUB_DATASET_INFO pubDataset;
} SMV_PUB_INFO;

/* XML文件SMV信息 */
typedef struct
{
    UINT8 muMode;
    UINT8 fpgaMode;
    UINT8 synMode;
    UINT8 synRev;
    UINT8 synEdge;
    UINT8 synDeal;
    UINT8 subNum;
    SMV_SUB_INFO smvSub[MAX_SUB_NUM];
    UINT8 pubNum;
    BOOL b9_2SndFlag;  /* 9-2发送与否 */
    uint8_t uc9_2Sn;  /* 9-2 PUB配置序号 */
    BOOL bFT3SndFlag;    /* FT3发送与否 */
    uint8_t ucFt3Sn;  /* FT3 PUB配置序号 */
    SMV_PUB_INFO smvPub[MAX_PUB_NUM];
    UINT8 intelPulse;  /* 插值脉冲类型 */
    UINT8 intelRev;  /* 插值脉冲取反与否 */
    UINT8 syn;  /* 强制同步 */
    UINT8 test;    /* 强制测试 */
} SMV_CFG_INFO;

/* XML解析信息 */
typedef struct
{
    SMV_CFG_INFO smvCfg;
} XML_CFG_INFO;

/* FPGA寄存器配置信息 */
typedef struct
{
    BOOL bFpgaInitFinishFlag;  /* FPGA设置完成标志 */
    BOOL bFt3CfgFlag;   /* 是否配置FT3标志 */
    BOOL bExternalSynFlag;   /* 需要外部同步源标志 */

    /* 控制寄存器0 */
    union CTRL_REG0_UN
    {
        struct CTRL_REG0_ST
        {
            uint32_t reserved:14;  /* reserved */
            uint32_t FpgaSoftReset:1; /* 1:软复位,0:正常运行 */
            uint32_t SndSynFlag:1;  /* 9-2/FT3强制同步位 */
            uint32_t SndTestFlag:1;  /* 9-2/FT3强制测试位 */
            uint32_t SmvSndFlag:1;  /* 9-2是否发送 */
            uint32_t FT3SndFlag:1;   /* FT3是否发送 */
            uint32_t FT3SndType:1;  /* 发送FT3格式 */
            uint32_t FT3Type:1;  /* 接收FT3格式 */
            uint32_t AdjRev:1;   /* 对时脉冲是否取反 */
            uint32_t AdjPulse:4;      /* 对时模式 */
            uint32_t PulseRev:1;   /* 同步脉冲是否取反 */
            uint32_t IntelPulse:4; /* 插值脉冲同步方式 */
            uint32_t InitOverFlag:1;  /* 初始化结束 */
        } ctrlReg0_st;

        uint32_t ulCtrlReg0;
    } ctrlReg0_un;
    volatile uint32_t *pctrlReg0Addr;  /* 地址 */

    /* 状态寄存器0 */
    union STS_REG0_UN
    {
        struct STS_REG0_ST
        {
            uint32_t proVer:16;
            uint32_t swVer:16;
        } stsReg0_st;

        uint32_t ulStsReg0;
    } stsReg0_un;
    volatile uint32_t *pstsReg0Addr;  /* 地址 */

    /* 状态寄存器1 */
    union STS_REG1_UN
    {
        struct STS_REG1_ST
        {
            uint32_t reserved:8;
            uint32_t delaytime:16;
            uint32_t backPoint:8;
        } stsReg1_st;

        uint32_t ulStsReg1;
    } stsReg1_un;
    volatile uint32_t *pstsReg1Addr;  /* 地址 */

    /* 秒脉冲同步状态寄存器 */
    union SEC_PULSE_STS_REG_UN
    {
        struct SEC_PULSE_STS_REG_ST
        {
            uint32_t reserved:27;
            uint32_t Pnct:1;  /* 是否处于守时状态 */
            uint32_t HasPulse:1;  /* 有无同步信号 */
            uint32_t PosPulse:1;    /* 正脉冲是否正常 */
            uint32_t Period:1;   /* 周期是否正常 */
            uint32_t Syn:1;  /* 同步状态 */
        } stsSecPulseReg_st;

        uint32_t ulSecPulseStsReg;
    } stsSecPulseReg_un;
    volatile uint32_t *pstsSecPulseRegAddr;  /* 地址 */

    /* B码时间寄存器0 */
    union B_REG0_UN
    {
        struct B_REG0_ST
        {
            uint32_t reserved:2;
            uint32_t Day:10;  /* 天 */
            uint32_t Hour:6;  /* 时 */
            uint32_t Min:7;  /* 分 */
            uint32_t Sec:7;  /* 秒 */
        } bReg0_st;

        uint32_t ulBReg0;
    } bReg0_un;
    volatile uint32_t *pBReg0Addr;  /* 地址 */

    /* B码时间寄存器1 */
    union B_REG1_UN
    {
        struct B_REG1_ST
        {
            uint32_t reserved:5;
            uint32_t controlOther:18;
            uint32_t YearTens:4;
            uint32_t Indexbit:1;
            uint32_t YearUnits:4;
        } bReg1_st;

        uint32_t ulBReg1;
    } bReg1_un;
    volatile uint32_t *pBReg1Addr;  /* 地址 */

    /* ns级时钟寄存器 */
    union TIME_NS_REG_UN
    {
        struct TIME_NS_REG_ST
        {
            uint32_t nscount;
        } nsTimeReg_st;

        uint32_t ulnsTimeReg;
    } nsTimeReg_un;
    volatile uint32_t *pnsTimeRegAddr;  /* 地址 */

    uint32_t ulLocalUs;   /* 本地us计数器,与B码对应 */

    /* SPI相关配置 */
    struct SPI_PARA
    {
        /* 待处理 */
    } spiPara;

    /* A/D相关配置 */
    struct AD_PARA
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t ucPortNo;  /* 端口号,固定为01 */
        uint8_t ucMaxChnNum;   /* 使用的最大内序,从1开始 */
        int16_t	iYabanNum[FPGA_AD_SAM_CHN_NUM];   /* 压板号,小于0则无效 */
        BOOL bRtYabanValue[FPGA_AD_SAM_CHN_NUM]; /* 压板状态 */
        BOOL bRdDlyCfg;  /* 是否配置RC滤波延时 */

        /* 控制寄存器 */
        union AD_CTRL_REG_UN
        {
            struct AD_CTRL_REG_ST
            {
                uint32_t rcDelay:12;  /* RC filter circuit delay(unit of us) */
                uint32_t maxDataBufNum:4;
                uint32_t reserved:16;
            } ctrlReg_st;

            uint32_t ulCtrlReg;
        } ctrlReg_un;
        volatile uint32_t *pctrlRegAddr;  /* 地址 */

        /* 状态寄存器 */
        union AD_STS_REG_UN
        {
            struct AD_STS_REG_ST
            {
                uint32_t reserved:27;
                uint32_t ad4Error:1;
                uint32_t ad3Error:1;
                uint32_t ad2Error:1;
                uint32_t ad1Error:1;
                uint32_t overflowFlag:1;
            } stsReg_st;

            uint32_t ulStsReg;
        } stsReg_un;
        volatile uint32_t *pstsRegAddr;  /* 地址 */

        /* 通道属性寄存器组,包含零漂和相移
         */
        union AD_ATTR_REG_UN
        {
            struct AD_ATTR_REG_ST
            {
                uint32_t phaseShift:16;
                uint32_t zeroDrift:16;
            } attrReg_st;

            uint32_t ulAttrReg;
        } attrReg_un[FPGA_AD_SAM_CHN_NUM];
        volatile uint32_t *pAttrRegAddr;

        /* BD寄存器,根据实际BD个数动态分配 */
        union AD_BD_REG_UN
        {
            struct AD_BD_REG_ST
            {
                uint32_t notEmpty:1;
                uint32_t address:7;
                uint32_t q:24;
            } bdReg_st;

            uint32_t ulBdReg;
        } bdReg_un[MAX_BD_NUM];
        volatile uint32_t *pbdRegAddr[MAX_BD_NUM];  /* 地址数组 */

        /* 数据寄存器 */
        volatile struct AD_DATA_REG
        {
            uint16_t reserved;    /* 保留字节 */
            uint16_t ulSamCnt;  /* 采样节拍 */
            int16_t ulSamData[FPGA_AD_SAM_CHN_NUM];   /* 采样值 */
        } *pulDataReg[MAX_BD_NUM];
    } adPara;

    /* A/D复采相关配置 */
    struct AD_PARA_REPEAT
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t ucPortNo;  /* 端口号,固定为01 */
        uint8_t ucMaxChnNum;   /* 使用的最大内序,从1开始 */
        int16_t	iYabanNum[FPGA_AD_REPEAT_SAM_CHN_NUM];   /* 压板号,小于0则无效 */
        BOOL bRtYabanValue[FPGA_AD_REPEAT_SAM_CHN_NUM]; /* 压板状态 */
        BOOL bRdDlyCfg;  /* 是否配置RC滤波延时 */
        uint8_t ucDataPosOff;  /* 存储地址偏移*/

        /* 控制寄存器 */
        union AD_REPEAT_CTRL_REG_UN
        {
            struct AD_REPEAT_CTRL_REG_ST
            {
                uint32_t rcDelay:12;  /* RC filter circuit delay(unit of us) */
                uint32_t maxDataBufNum:4;
                uint32_t reserved:16;
            } ctrlReg_st;

            uint32_t ulCtrlReg;
        } ctrlReg_un;
        volatile uint32_t *pctrlRegAddr;  /* 地址 */

        /* 状态寄存器 */
        union AD_REPEAT_STS_REG_UN
        {
            struct AD_REPEAT_STS_REG_ST
            {
                uint32_t reserved:26;
                uint32_t ad2Error:1;
                uint32_t ad1Error:1;
                uint32_t CRCError:1;
                uint32_t intervalError:1;
                uint32_t linkError:1;
                uint32_t overflowFlag:1;
            } stsReg_st;

            uint32_t ulStsReg;
        } stsReg_un;
        volatile uint32_t *pstsRegAddr;  /* 地址 */

        /* 通道属性寄存器组,包含零漂和相移
         */
        union AD_REPEAT_ATTR_REG_UN
        {
            struct AD_REPEAT_ATTR_REG_ST
            {
                uint32_t phaseShift:16;
                uint32_t zeroDrift:16;
            } attrReg_st;

            uint32_t ulAttrReg;
        } attrReg_un[FPGA_AD_REPEAT_SAM_CHN_NUM];
        volatile uint32_t *pAttrRegAddr;

        /* BD寄存器,根据实际BD个数动态分配 */
        union AD_REPEAT_BD_REG_UN
        {
            struct AD_REPEAT_BD_REG_ST
            {
                uint32_t notEmpty:1;
                uint32_t address:9;
                uint32_t reserved:10;
                uint32_t q:12;
            } bdReg_st;

            uint32_t ulBdReg;
        } bdReg_un[MAX_BD_NUM];
        volatile uint32_t *pbdRegAddr[MAX_BD_NUM];  /* 地址数组 */

        /* 数据寄存器 */
        volatile struct AD_REPEAT_DATA_REG
        {
            uint16_t reserved;    /* 保留字节 */
            uint16_t ulSamCnt;  /* 采样节拍 */
            int16_t ulSamData[FPGA_AD_REPEAT_SAM_CHN_NUM];   /* 采样值 */
        } *pulDataReg[MAX_BD_NUM];
    } adParaRepeat;

    /* FT3相关配置 */
    struct FT3_PARA
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t ucPortNo;  /* FT3端口号 */
        uint8_t ucDataPosOff;  /* 存储地址偏移*/
        uint8_t ucMaxChnNum;   /* 使用的最大内序,从1开始 */
        int16_t	iYabanNum[FPGA_FT3_SAM_CHN_NUM];   /* 压板号,小于0则无效 */
        BOOL bRtYabanValue[FPGA_FT3_SAM_CHN_NUM]; /* 压板状态 */

        /* 控制寄存器 */
        union FT3_CTRL_REG_UN
        {
            struct FT3_CTRL_REG_ST
            {
                uint32_t reserved:11;
                uint32_t dTimeValid:1;  /* 延迟时间是否有效 */
                uint32_t maxDataBufNum:4;
                uint32_t phaseShift:16;
            } ctrlReg_st;

            uint32_t ulCtrlReg;
        } ctrlReg_un;
        volatile uint32_t *pctrlRegAddr;  /* 地址 */

        /* 状态寄存器 */
        union FT3_STS_REG_UN
        {
            struct FT3_STS_REG_ST
            {
                uint32_t reserved:28;
                uint32_t crcErr:1;
                uint32_t rangeOut:1;
                uint32_t linkFail:1;
                uint32_t overflowFlag:1;
            } stsReg_st;

            uint32_t ulStsReg;
        } stsReg_un;
        volatile uint32_t *pstsRegAddr;  /* 地址 */

        /* 寄存器 */
        struct tag_REG
        {
            struct REG1
            {
                uint32_t LNName:8;
                uint32_t DataSetName:8;
                uint32_t LDName:16;
            } *pReg1;

            struct REG2
            {
                uint32_t PhsA_Artg:16;
                uint32_t Neut_Artg:16;
            } *pReg2;

            struct REG3
            {
                uint32_t PhsA_Vrtg:16;
                uint32_t Tdr:16;
            } *pReg3;

            struct REG4
            {
                uint32_t Status1:16;
                uint32_t Status2:16;
            } *pReg4;
        } reg;

        /* BD寄存器,根据BD个数动态分配 */
        union FT3_BD_REG_UN
        {
            struct FT3_BD_REG_ST
            {
                uint32_t notEmpty:1;
                uint32_t address:9;
                uint32_t q:22;
            } bdReg_st;

            uint32_t ulBdReg;
        } bdReg_un[MAX_BD_NUM];
        volatile uint32_t *pbdRegAddr[MAX_BD_NUM];  /* 地址数组 */

        /* 数据寄存器 */
        volatile struct FT3_DATA_REG
        {
            uint16_t reserved;    /* 保留字节 */
            uint16_t ulSamCnt;  /* 采样节拍 */
            int16_t ulSamData[FPGA_FT3_SAM_CHN_NUM];   /* 采样值 */
        } *pulDataReg[MAX_BD_NUM];
    } ft3Para[FT3_PORT_NUM];

    /* FT3发送相关配置 */
    struct FT3_SND_PARA
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t type;   /* 类型 */
        uint16_t len;  /* 长度 */
        uint8_t ucPortNo;  /* SV端口号 */
        uint8_t chnNum;  /* 通道数 */
        uint8_t LNName;
        uint8_t DSName;
        uint16_t LDName;
        uint32_t confRev;
        uint16_t rtdPhsCur;
        uint16_t rtdNeucur;
        uint16_t rtdPhsVol;
        uint16_t rtdDlyTime;

        /* 通道配置 */
        struct CHN_FT3_SND_CFG_ST
        {
            /* 通道合流配置寄存器 */
            union CHN_FT3_MERG_CFG_UN
            {
                struct CHN_FT3_CFG_ST
                {
                    uint32_t valid:1;  /* 是否合流 */
                    uint32_t srcSel:7;  /* AD:0, FT3-0:1, FT3-1:2, FT3-2:3 */
                    uint32_t srcChnNo:8;
                    uint32_t coff:16;
                } cfgReg_st;

                uint32_t ulCfgReg;
            } chnMergCfg[CHN_MERG_NUM];

            int SubNoComefrom[CHN_MERG_NUM];
            int ChnNoComefrom[CHN_MERG_NUM];
            int HwCfgIndexComefrom[CHN_MERG_NUM];

        } chnSndCfg[FPGA_FT3_SND_CHN_NUM];
        volatile uint32_t *pCfgRegAddr;  /* 地址 */

        /* 多路开关控制寄存器 */
        uint32_t ulMultiplexCtrlReg;
        volatile uint32_t *pMultiplexCtrlRegAddr;  /* 地址 */

        /* PUB在SUB中对应的通道号
         */
        uint8_t outputChnNo[FPGA_FT3_SND_CHN_NUM];

        volatile uint32_t *pFrameRegAddr;  /* 发送帧地址 */
        uint32_t ucFt3Frame[(FT3_MAX_LEN+3)/4];   /* 发送帧框架 */
    } ft3SndPara;

    /* SV接收相关配置 */
    struct SV_RCV_PARA
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t ucPortNo;  /* 端口号 */
        uint8_t ucDataPosOff;  /* 存储地址偏移*/
        uint8_t ucMaxChnNum;   /* 使用的最大内序,从1开始 */
        int16_t	iYabanNum[FPGA_SV_RCV_CHN_NUM];   /* 压板号,小于0则无效 */
        BOOL bRtYabanValue[FPGA_SV_RCV_CHN_NUM]; /* 压板状态 */

        /* 控制寄存器 */
        union SV_RCV_CTRL_REG_UN
        {
            struct SV_RCV_CTRL_REG_ST
            {
                uint32_t reserved:6;
                uint32_t dTimeValid:1;  /* 延迟时间是否有效 */
                uint32_t dTChn:5;   /* Delay time channel select(0~21) */
                uint32_t maxDataBufNum:4;
                uint32_t phaseShift:16;
            } ctrlReg_st;

            uint32_t ulCtrlReg;
        } ctrlReg_un;
        volatile uint32_t *pctrlRegAddr;  /* 地址 */

        /* MAC寄存器0 */
        union MAC_REG0_UN
        {
            struct MAC_REG0_ST
            {
                uint32_t byte2:8;
                uint32_t byte3:8;
                uint32_t byte4:8;
                uint32_t byte5:8;
            } macReg0_st;

            uint32_t ulMacReg0;
        } macReg0_un;
        volatile uint32_t *pmacReg0Addr;  /* 地址 */

        /* MAC寄存器1 */
        union MAC_REG1_UN
        {
            struct MAC_REG1_ST
            {
                uint32_t reserved:16;
                uint32_t byte0:8;
                uint32_t byte1:8;
            } macReg1_st;

            uint32_t ulMacReg1;
        } macReg1_un;
        volatile uint32_t *pmacReg1Addr;  /* 地址 */

        /* 状态寄存器0 */
        union SV_RCV_STS_REG0_UN
        {
            struct SV_RCV_STS_REG0_ST
            {
                uint32_t reserved:24;
                uint32_t rcvErr:1;
                uint32_t noErr:1;
                uint32_t lenErr:1;
                uint32_t crcErr:1;
                uint32_t decodeErr:1;
                uint32_t rangeOut:1;
                uint32_t linkFail:1;
                uint32_t overflowFlag:1;
            } stsReg0_st;

            uint32_t ulStsReg0;
        } stsReg0_un;
        volatile uint32_t *psvRecvSts0RegAddr;  /* 地址 */

        /* 状态寄存器1 */
        union SV_RCV_STS_REG1_UN
        {
            struct SV_RCV_STS_REG1_ST
            {
                uint32_t reserved:8;
                uint32_t chn21Valid:1;  /* 有效与否标志 */
                uint32_t chn20Valid:1;
                uint32_t chn19Valid:1;
                uint32_t chn18Valid:1;
                uint32_t chn17Valid:1;
                uint32_t chn16Valid:1;
                uint32_t chn15Valid:1;
                uint32_t chn14Valid:1;
                uint32_t chn13Valid:1;
                uint32_t chn12Valid:1;
                uint32_t chn11Valid:1;
                uint32_t chn10Valid:1;
                uint32_t chn9Valid:1;
                uint32_t chn8Valid:1;
                uint32_t chn7Valid:1;
                uint32_t chn6Valid:1;
                uint32_t chn5Valid:1;
                uint32_t chn4Valid:1;
                uint32_t chn3Valid:1;
                uint32_t chn2Valid:1;
                uint32_t chn1Valid:1;
                uint32_t chn0Valid:1;
                uint32_t test:1;  /* 测试与否 */
                uint32_t syn:1;  /* 同步与否 */
            } stsReg1_st;

            uint32_t ulStsReg1;
        } stsReg1_un;
        volatile uint32_t *psvRecvSts1RegAddr;  /* 地址 */

        /* 状态寄存器2 */
        union SV_RCV_STS_REG2_UN
        {
            struct SV_RCV_STS_REG2_ST
            {
                uint32_t reserved:16;
                uint32_t Tdr:16;
            } stsReg2_st;

            uint32_t ulStsReg2;
        } stsReg2_un;
        volatile uint32_t *psvRecvSts2RegAddr;  /* 地址 */

        /* BD寄存器,根据BD个数动态分配 */
        union SV_RCV_BD_REG_UN
        {
            struct SV_RCV_BD_REG_ST
            {
                uint32_t notEmpty:1;
                uint32_t address:9;
                uint32_t q:22;
            } bdReg_st;

            uint32_t ulBdReg;
        } bdReg_un[MAX_BD_NUM];
        volatile uint32_t *pbdRegAddr[MAX_BD_NUM];  /* 地址数组 */

        /* 数据寄存器 */
        volatile struct SV_RCV_DATA_REG
        {
            uint32_t ulSamCnt;  /* 高16位为保留字节,低16位为采样节拍,为了与采样值内存布局一致如此处理 */
            int32_t ulSamData[FPGA_SV_RCV_CHN_NUM];   /* 采样值 */
        } *pulDataReg[MAX_BD_NUM];

    } svRcvPara[SMV_RCV_PORT_NUM];

    /* SV发送相关配置 */
    struct SV_SND_PARA
    {
        BOOL bUsed;  /* 使用标志 */
        uint8_t ucPortNo;  /* SV端口号 */
        uint8_t chnNum;  /* 通道数 */
        uint8_t noASDU;    /* ASDU数 */

        /* 通道配置 */
        struct CHN_SV_SND_CFG_ST
        {
            /* 通道合流配置寄存器 */
            union CHN_SV_MERG_CFG_UN
            {
                struct CHN_SV_CFG_ST
                {
                    uint32_t delayFlag:1;  /* Filled in rated delay time(0:sample data 1:rated delay time) */
                    uint32_t valid:1;  /* 是否合流 */
                    uint32_t MeaFlag:1; /* Source data coming from measuring current */
                    uint32_t srcSel:5;  /* AD:0, AD_Repeat:1, FT3-0:2, FT3-1:3, SV-0:4, SV-1:5 */
                    uint32_t srcChnNo:8;
                    uint32_t coff:16;
                } cfgReg_st;

                uint32_t ulCfgReg;
            } chnMergCfg[CHN_MERG_NUM];

            int SubNoComefrom[CHN_MERG_NUM];
            int ChnNoComefrom[CHN_MERG_NUM];
            int HwCfgIndexComefrom[CHN_MERG_NUM];

        } chnSndCfg[FPGA_SV_SND_CHN_NUM];
        volatile uint32_t *pCfgRegAddr;  /* 地址 */

        /* 长度寄存器 */
        union LEN_UN
        {
            struct LEN_ST
            {
                uint32_t reserved:12;
                uint32_t cntOff:8;  /* smpCnt offset address */
                uint32_t reserved0:1;
                uint32_t len:11;
            } lenReg_st;

            uint32_t ulLenReg;
        } chnLen;
        volatile uint32_t *pLenRegAddr;  /* 地址 */

        /* 使能寄存器 */
        union PORT_ENB_UN
        {
            struct PORT_ENB_ST
            {
                uint32_t reserved:26;
                uint32_t port7:1;
                uint32_t port6:1;
                uint32_t port5:1;
                uint32_t port4:1;
                uint32_t port3:1;
                uint32_t port2:1;
            } portEnbReg_st;

            uint32_t ulPortEnbReg;
        } portEnb;
        volatile uint32_t *pportEnbRegAddr;  /* 地址 */

        /* PUB在SUB中对应的通道号
         */
        uint8_t outputChnNo[FPGA_SV_SND_CHN_NUM];

        uint8_t ucDestMacAddr[MAC_ADDR_LEN];   /* 目的地址 */
        uint8_t ucSrcMacAddr[MAC_ADDR_LEN];       /* 源地址 */
        uint16_t TCI;
        uint16_t APPID;
        uint16_t AllLength;  /* 长度 */
        uint8_t APDU_len;
        uint16_t seqASDU_len;
        uint8_t ASDU_len;
        uint8_t seqData_len;
        uint8_t strLen;    /* SVID */
        volatile uint32_t *pFrameRegAddr;  /* 地址 */
        uint32_t confRev;
        uint32_t ucEthFrame[(ETHE_MAX_LEN+3)/4];
    } svSndPara;

    uint8_t maxDataBufNum;  /* A/D,FT3缓冲区大小 */
} FPGA_CFG_INFO;

/* FPGA数据源仿真 */
typedef struct tag_FPGA_SIMUL
{
    uint32_t *pSimulAddr;  /* 模拟地址 */
    int16_t usSimulCnt;  /* 仿真缓冲区填写顺序计数 */
    uint32_t ulSimuSamCnt;   /* 仿真采样节拍 */
    int16_t iWaveData[2*SMP_RATE_PER_CYCLE];	/* 仿真数据 */
    int32_t iDrawOutWaveData[MAXCHNELS];	/* 抽取仿真数据 */

    uint32_t ulInvalidCnt1;  /* 无有效填写点计数1 */
    uint32_t ulInvalidCnt2;  /* 无有效填写点计数2 */
    uint32_t ulInvalidCnt3;  /* 无有效填写点计数3 */
    uint32_t ulValidCnt;  /* 有效填写点计数 */
    uint32_t ulDrawOutInvalidCnt;  /* 无有效读取点计数 */

    uint32_t ulValidDrawPoint;   /* 有效抽取点数 */

    uint32_t ulSimulRunCnt;  /* 填写次数计数 */
    uint32_t ulRdCnt;  /* 读取次数计数 */
    uint32_t ul_80_ValidCnt;   /* 周波80有效点计数 */
} FPGA_SIMUL;

typedef struct tag_SMV_DATA
{
    /* 插值抽取采样数据,包括1路A/D,最多2路FT3
     * A/D占用前0~17路通道
     * FT0占用18~39路通道
     * FT1占用40~61路通道
     */
    int32_t nProcSamData[MAXCHNELS];

    /* 本次采样数据点 */
    int32_t nCurSamData[MAXCHNELS];

    /* 上一传送采样数据点,用于插值抽取 */
    int32_t nLstSamData[MAXCHNELS];

    /* 上一传送采样数据点状态标,用于合成实际抽取点状态标 */
    uint32_t nLstSamDataSts[MAXCHNELS];

    /* 当前采样数据点状态标,用于合成实际抽取点状态标 */
    uint32_t nCurSamDataSts[MAXCHNELS];

    /* 采样点有效标志 */
    uint16_t ucCurSrcValidFlag;
    uint16_t ucAllSrcValidFlag;

    /* 仿真读取采样点有效标志 */
    uint8_t ucRdCurSrcValidFlag;

    uint16_t usSamCnt;  /* 采样秒计数 */
    uint16_t usSamCycleCnt;  /* 采样周波计数 */
    uint16_t usDrawCycleCnt;  /* 抽取周波计数 */
    uint16_t usSrcCntCyle; /* 接收源计数周期 */
    uint32_t usSecCnt;  /* 秒计数 */
    uint16_t usLstSecCnt;  /* 上次秒计数,用于判丢点 */
    uint16_t usLstCycleCnt;  /* 上次采样周波计数,用于判丢点 */
    uint32_t usLostCnt;  /* 丢点计数 */
    uint32_t ulLostMaxInt;   /* 最大丢点间隔 */

    uint8_t ucBufCnt;  /* 缓冲计数,从0开始查询,有效则累加 */

    int32_t nStartDif_Pol;  /* 起始点插值抽取位置 */
    int32_t nFw_Pol; /* 偏移位置 */
    int32_t nDif_Pol;  /* 插值抽取位置 */
    int32_t nArr_Dif_Pol[SMP_RATE_PER_CYCLE];  /* 插值抽取位置数组 */
    uint32_t nArr_Pol_Proc_Flag[SMP_RATE_PER_CYCLE];  /* 丢弃,保存,插值 */
    uint16_t usForeCnt[SMP_RATE_PER_CYCLE];  /* 插值前点 */
    BOOL bFstDrawFlag;    /* 首抽取点出现 */
    uint8_t ucBackPoint;    /* 回推点数 */
    uint16_t Ft3Tdr[FT3_PORT_NUM];  /* FT3通道延时 */
    uint16_t SmvTdr[SMV_RCV_PORT_NUM];  /* SMV通道延时 */
    BOOL bSynChgFlag;  /* 同步状态变化判断 */
    uint32_t ulInsertDelay;  /* 延迟抽取点数 */

    uint32_t ulStartPointNum;  /* 起始点前点 */
    uint32_t ulEndPointNum;  /* 起始点后点 */
} SMV_DATA;

#define MAX_TIME_SYC_MODE_NUM 2

typedef struct tag_TIME_SYC_MODE
{
    UINT8 SycModeNum;
    UINT8 CurModeNum;
    UINT8 SycModeArray[MAX_TIME_SYC_MODE_NUM];
} TIME_SYC_MODE_STRUCT;

#define MAX_INTER_PULSE_MODE_NUM 2

typedef struct tag_INTER_PULSE_MODE
{
    UINT8 PulseModeNum;
    UINT8 CurModeNum;
    UINT8 PulseModeArray[MAX_INTER_PULSE_MODE_NUM];
} INTER_PULSE_MODE_STRUCT;

/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的SMV输入虚端子供界面监视的单端子配置信息结构  */
typedef struct
{
    int  iValType;  /*虚端子单位类型，和内部规约一致，6是SMV浮点类型 */
    uint16_t  uiDataSetAppID;   /*关联源端数据集的APPID ，以'\0'结尾*/
    uint8_t   aucDescStr[65];   /*描述字符串，以'\0'结尾 */
    uint8_t   aucYabanIDStr[33];  /*该虚端子关联的MU压板ID ，以'\0'结尾*/
    uint8_t   ucUnit;           /*该虚端子单位类型,指电流，电压 */
    int      iYabanNo;          /*该虚端子关联的MU压板号，<0代表无压板关联*/
    void   *  pHwCh;        /*该虚端子关联的硬件配置通道句柄， */
    void   *  pLogAI;        /*该虚端子关联的未滤波算法的AI逻辑通道句柄 */
    BOOL bIsPend; /* 是否悬空 */
}   SMV_VT_SV_TERM_CFG;

/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的SMV输入虚端子供界面监视的总体配置信息结构  */
typedef struct
{
    int  iTermCnt;
    SMV_VT_SV_TERM_CFG   aTermCfgArr[MAXHCHNNUM];
}   SMV_TOTAL_VT_SV_TERM_CFG;


/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的SMV输入虚端子供界面监视的单端子状态信息结构  */
typedef struct
{
    float     fTermVal;    /*SV虚端子有效值，为浮点数 */
    uint8_t   ucTermQuality; /*SV虚端子品质 */
}   SMV_VT_SV_TERM_STS;

/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的SMV输入虚端子供界面监视的总体状态信息结构  */
typedef struct
{
    int  iTermCnt;
    SMV_VT_SV_TERM_STS   aTermStsArr[MAXHCHNNUM];
}   SMV_TOTAL_VT_SV_TERM_STS;


/* globals */

extern IEC_SMV_AD_POOL gSmvADCfg;   /*最终存放的结构体名*/
extern FPGA_CFG_INFO sFpgaCfg;
extern XML_CFG_INFO sXmlCfg;

extern TIME_SYC_MODE_STRUCT TimeSycModeCfg;
extern INTER_PULSE_MODE_STRUCT InterPulseModeCfg;
extern BOOL bNeedtoStartFPGA;
extern BOOL bPlatformCfgFPGA;  /* 平台初始化FPGA */

extern uint32_t FPGASmvCommStat[2+FT3_PORT_NUM+SMV_RCV_PORT_NUM];/*AD+AD复采+FT3+92*/
extern BOOL bSysSynFlag;

/*当前所有采样通道的实际接收状态标数组 ZY 2013-6-5 */
extern  uint32_t aulCurSamDataStsArr_g[MAXCHNELS];

/* functions */

/* 解析配置文件.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL cfgFileParse(void);

/* 解析SUB配置文件.
 * Para:
 *     xml句柄.
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL cfgParseSmvSubInfo(mxml_node_t *rootnode);

/* 解析PUB配置文件.
 * Para:
 *     xml句柄.
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL cfgParseSmvPubInfo(mxml_node_t *rootnode);

/* 通过通讯获取采样值初始化.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
	*/
extern BOOL GetFPGASampDataInit(void);

/* 启动FPGA运行.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void fpgaStart(void);

/* 获取默认对时模式.
 * Para:
 *     NONE.
 * Return:
 *     TimeSycMode.
 */
extern UINT8 Get_DefaultTimeSycMode();

/* 获取默认插值脉冲方式.
 * Para:
 *     NONE.
 * Return:
 *     InterPulseMode.
 */
extern UINT8 Get_DefaultInterPulseMode();

/* 更新FPGA采样转发系数
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void UpdateFPGASmvSendCoff();

/* 切换对时与插值脉冲方式.
 * Para:
 *     NONE.
 * Return:
 *     TRUE or FALSE.
 */
extern BOOL Change_TimeSycAndInterPulseMode();


/* FPGA数据有效性判断.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 * Alert:
 *     返回TRUE表明本次查询并插值抽取到有效数据,
 *     返回FALSE表明无有效插值抽取数据,
 *     支持多次查询
 */
extern BOOL fpgaValidDataJudge(void);

/* 同步信号状态变化判断.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void fpgaSynChgSts(void);

/* FPGA数据读取.
 * Para:
 *     NONE
 * Return:
 *     NONE.
 */
extern void fpgaDataDraw(void);

/* 获取物理通道到采样通道对应关系.
 * Para:
 *     pSamtoAna,物理通道到采样通道对应关系.
 *     pSamSource, 通道对应端口号
 * Return:
 *     NONE.
 */
extern void cfgGetSamtoAna(uint8_t *pSamtoAna, uint8_t *pSamSource);

/* 获取物理通道一次/数字量系数.
 * Para:
 *     ucHdCh,物理通道号,从1开始.
 * Return:
 *     系数.
 */
extern float cfgGetHwChnCoff(uint8_t ucHdCh);

/* 计算插值位置
 * Para:
 *     ulOrgPointNum, 原始数据周波点数.
 *     ulInsertPointNum, 插值周波点数.
 *     ulDelayNum, 数据延迟点数.
 * Return:
 *     NONE.
 */
extern void fpgaCalcInsertPos(uint32_t ulOrgPointNum, uint32_t ulInsertPointNum, uint32_t ulDelayNum);

/* 通过ASDU序号获取间隔描述
 * Para:
 *     nAsduNo, ASDU序号,从0开始计数
 *     pDesc, 间隔描述字符串输出的目的指针
 * Return:
 *     TRUE, 成功 FALSE, 失败
 */
extern BOOL GetBayDesByAsduNo(int nAsduNo, char *pDesc);

#endif

