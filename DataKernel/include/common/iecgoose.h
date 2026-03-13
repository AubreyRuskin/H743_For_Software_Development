

#ifndef _GO_H
#define _GO_H


#include "vxworks_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#undef MBR_DATA_VALUE_CHECK_SUPPORT /* 是否判断接收项数据类型 */
#undef RCV_PACKET_TIME_SUPPORT /* 支持报文接收时标读取 */
#undef GO_DI_TO_DO_DRC /*  直接开出 */

/****************************************
 * 版本号
 */
#define VERSION  "2.04.26_b14.h"

/****************************************
*测试宏定义
*/
/*#define SHOW_DATA_TEST*/
/*#define EFFICIENCY_TEST*/
/*#define TX_TIME_TEST*/

/****************************************
*文件名定义
*/
#define CONFIG_FILE		"/tffs/lgh/gse.xml"

#define RISCTIMERNUMFORPUBGOOSE 15		/* GOOSE PUB任务使用的Risc Timer */

/****************************************
*数据类型
*/
#define    INT8   		signed char
#define    INT16       	signed short
#define    INT32  		signed long
#define    UINT8  		unsigned char
#define    UINT16 		unsigned short
#define    UINT32 		unsigned long

/****************************************
*网络变量定义
*/

#define ETHE_MAC_LEN				6
#define ETHE_LEN_HEAD				(2*ETHE_MAC_LEN + 2)
#define ETYPE_VLAN_TYPE_ID      	0x8100
#define ETYPE_TYPE_GOOSE        	0x88B8
#define ETYPE_TCI_GOOSE	        	0x8000

#define SUB_GO_NET_A                (0)
#define SUB_GO_NET_B                (1)

#define ETHE_LEN_QTAG_PREFIX		4	/* 802.1Q (VLAN) header length	*/
#define ETHE_GSE_HDR_LEN        	24

#define  MAX_STRING_LEN       		128
#define  MAX_RX_MSG_LEN		  	    1518
#define  MAX_PDU_BUF_SIZE     		1500
#define  MAX_NET_PORT_NUM			12

extern BOOL DOUBLE_GOOSE_COM;
extern BOOL bGoNetMode_g;
#define GOOSE_MODE_SINGLE_A  		0x01
#define GOOSE_MODE_SINGLE_B  		0x02
#define GOOSE_MODE_DOUBLE  			0x04
#define GOOSE_MODE_SINGLE_REDU      0x08

/****************************************/
#define  MAX_TIME_SOURCE_DI_NUM     32
#define  MAX_PUB_MAPPING_TIMES      16

/****************************************
*
*/
#define MEA_TYPE_VALUE          	10
#define DI_TYPE_VALUE           	11
#define TIMESTAMP_TYPE_VALUE    	12
#define Q_TYPE_VALUE            	13

#define FLOAT_TYPE_MEA          	16
#define INT32_TYPE_MEA          	17
#define SINGLE_TYPE_DI          	18
#define DUAL_TYPE_DI            	19
#define UTC_TYPE_TIME           	20

#define GOOSE_CFG_ERROR			    -1
#define GOOSE_CFG_EMPTY			     0
#define GOOSE_CFG_HAVE_RX			 1
#define GOOSE_CFG_HAVE_TX			 2
#define GOOSE_CFG_HAVE_TRANS		 4
#define GOOSE_CFG_HAVE_ALL		     7

#define GOOSE_SUB_CFG_ERROR		0x01
#define GOOSE_SUB_CFG_ALARM		0x02
#define GOOSE_PUB_CFG_ERROR		0x04
#define GOOSE_PUB_CFG_ALARM		0x08
#define GOOSE_TRANS_CFG_ERROR	0x10
#define GOOSE_TRANS_CFG_ALARM	0x20

/***********************************************
发送接收状态值
***********************************************/

#define GOOSE_TX_INIT_END			0x10	/*发送数据已经初始化*/
#define GOOSE_TX_NORMAL				0x20	/*发送稳定周期状态T0*/
#define GOOSE_TX_VALUE_INVALID      0x30    /*数据无效*/
#define GOOSE_TX_CHANGE				0x80	/*发生变位T1*/
#define GOOSE_TX_FIRST_PRRIOD		0x81	/*变位后第一阶段*/
#define GOOSE_TX_SECOND_PRRIOD		0x82	/*变位后第二阶段*/
#define GOOSE_TX_THIRD_PRRIOD		0x83	/*变位后第三阶段*/
#define GOOSE_TX_FOURTH_PRRIOD		0x84	/*变位后第四阶段*/

#define GOOSE_DATA_INVALID      	0
#define GOOSE_DATA_READY        	1
#define GOOSE_DATA_READING      	2
#define GOOSE_DATA_WRITING      	3
#define GOOSE_DATA_EMPTY        	9   //goose.ini not found

/****************************************
*IED状态
*/
#define SUB_IED_READY               1
#define SUB_IED_COM_ERR             4
#define SUB_IED_OVERTIME            5
#define SUB_IED_FAULT               6
#define SUB_IED_IN_REPAIR           7
#define SUB_IED_TEST_MODE           SUB_IED_IN_REPAIR
#define SUB_IED_OK                  8
#define SUB_IED_CONFREV_ERR         9   /*2007-7-7日张云添加,SUB关联的CONFREV版本不对  */
#define SUB_IED_YABAN_EXIT          10   /*2007-7-10日张云添加,SUB关联的压板退出    */

#define IEC61850_MOD_TEST			(0x03)

/*****************************************
平台功能码
*/
#define PLATFORM_FUNC_CODE_DEFAULT			0
#define PLATFORM_FUNC_CODE_E02_TRANSMIT		1

#define CPU_RCV_QUEUE_LEN 32  /* 缓冲队列长度 */
#define CPU_RCV_QUEUE_MODE 0x1F  /* 长度模 */

#define CC_MODE  /* CC模式 */

/****************************************
*数据结构定义
*/

#ifndef EP_DATE_TIME_STRUCT
#define EP_DATE_TIME_STRUCT

typedef int (*eth_cb_func)(uint8_t port, uint8_t *buf, uint32_t len, int para1, int para2);

typedef struct
{
    uint16_t 	unMicroSec;   	/* 0-999 */
    uint16_t 	unMSEL;       	/* 0-999 */
    uint8_t 	ucSec;        		 /* 0-59 */
    uint8_t 	ucMinute;            /* 0-59 */
    uint8_t 	ucHour;                     /* 0-23 */
    uint8_t 	ucDate;                     /* 1-31 */
    uint8_t 	ucWeekDay;                  /* 1～7，other value means don't care */
    uint8_t 	ucMonth;                    /* 1-12 */
    uint16_t 	unYear;                    /* 2000～…… */
    uint8_t ucQflag;
    uint8_t ucIrigbLSFlag;          /*闰秒的特殊处理标志*/
} EP_DATE_TIME;
#endif

typedef struct mms_utc_time_tag
{
    UINT32 secs;      	/* Number of seconds since January 1, 1970	*/
    UINT32 fraction;  	/* Fraction of a second				*/
    UINT8  qflags;		/* Quality flags, 8 least-significant bits only	*/
} MMS_UTC_TIME;

typedef struct tag_timestamp
{
    int flag;
    int yr;
    int mon;
    int dy;
    int hr;
    int min;
    int sec;
    int msec;
    int invalid;
} TIMESTAMP_VALUE;

/*开关量*/
typedef struct tag_di_value
{
    int flag;   /*SINGLE_TYPE_DI|DUAL_TYPE_DI*/
    int cls;    /*flag==DUAL_TYPE_DI  1:OPEN  2:CLOSE   0,3:中间态   0x80:错误*/
    /*flag==SINGLE_TYPE_DI  0:OPEN  1:CLOSE*/
} DIGI_VALUE;

/*模拟量*/
typedef struct tag_mea_value
{
    int flag;   /*FLOAT_TYPE_MEA|INT32_TYPE_MEA*/
    int i;
    float f;
} MEA_VALUE;

/*品质量*/
enum qflag_valid
{
    data_good=0,
    data_invalid,
    data_reserved,
    data_questionable
};

enum qflag_source
{
    src_process=0,
    src_substitued
};

typedef struct tag_quality
{
    enum qflag_valid eValidity;
    BOOL bOverflow;
    BOOL bOutOfRange;
    BOOL bBadReference;
    BOOL bOscillatory;
    BOOL bFailure;
    BOOL bOldData;
    BOOL bInconsistent;
    BOOL bInaccurate;
    enum qflag_source eSource;
    BOOL bTest;
    BOOL bOpBlocked;
} QUALLITY_VALUE;

union DaValue
{
    MEA_VALUE        mea_val;
    DIGI_VALUE       di_val;
    TIMESTAMP_VALUE  tm_val;
    QUALLITY_VALUE   q_val;
};

typedef struct tag_Goose_Da_Value
{
    int nDaType;    /*MEA_TYPE_VALUE | DI_TYPE_VALUE | TIMESTAMP_TYPE_VALUE | Q_TYPE_VALUE*/
    union DaValue da_val;
} GOOSE_DA_VALUE;


/****************************************
*
*/
typedef struct gse_mac_hdr
{
    UINT8   dst_mac[6];
    UINT8   src_mac[6];
    UINT16  tpid;
    UINT16  tcl;
    UINT16  ethtype;
    UINT16  appid;
    UINT16  lenth;
    UINT16  Reserved1;
    UINT16  Reserved2;
    UINT8   pduBuf[MAX_PDU_BUF_SIZE];
} GOOSE_FRAME_HDR;


typedef struct
{
    UINT8   	    dst_mac[6];
    UINT8   		src_mac[6];
    UINT16  		appID;
    char           	gcRef[MAX_STRING_LEN];
    UINT32         	timeToLive;
    char           	dataSetRef[MAX_STRING_LEN];
    char            GoID[MAX_STRING_LEN];
    MMS_UTC_TIME   	utcTime;
    UINT32         	stNum;
    UINT32         	sqNum;
    BOOL           	test;
    INT32          	confRev;
    BOOL           	ndscom;
    INT32          	numDataEntries;
    char			*pMbr;
} GSE_IEC_INFO;


typedef struct go_rx_msg_st
{
    int 	msgLen;
    int 	netPort;
    UINT8 	msgBuf[MAX_RX_MSG_LEN];
} NET_MSG_INFO;


/****************************************
*
*/
typedef enum valuetype
{
    UNKNOW_TYPE = 0,
    STRUCTURE_TYPE,
    BOOL_TYPE,
    DPC_TYPE,
    INT_TYPE,
    UINT_TYPE,
    FLOAT_TYPE,
    QUALITY_TYPE,
    UTCTIME_TYPE
} VALUETYPE;

typedef enum nettype
{
    UNKNOWN =0,
    ALONE,
    REDUNDANT		/*冗余*/
} NETTYPE;

typedef union da_value
{
    BOOL			bvalue;
    UINT8			svalue[64];
    int				ivalue;
    UINT32			uvalue;
    float			fvalue;
    MMS_UTC_TIME	utctime;
    QUALLITY_VALUE  quality;
} DA_VALUE;

typedef struct da_info
{
    int 			index;
    int  			ordinal;
    VALUETYPE		type;
    int				flag;
    BOOL			bfixedval;
    BOOL			bvaluemapped;
    DA_VALUE		*value;
    DA_VALUE		*packettime;
    char			*desc;
    int32_t lModAddr; /* 对应模件地址 */
    int32_t lDoChnNum;  /* 对应通道号 */
    int16_t pointid; /* 序号 */
    int iSubDaIdx; /* 多合一序号 */
    struct 	da_info *next;
    struct da_info *child;
    char                *ybLogicID; /* 压板的逻辑标识*/
    int iYabanIndex;
    BOOL bLinked;   /* 是否关联DATAMAP */
    uint8_t PoolIndex;  /* 用于索引当前DI上次变位的packettime */
} DA_INFO;

typedef struct dst_address
{
    int			portnum;
    BOOL        chg_src_mac;
    char		src_mac[6];
} DST_ADDR;

typedef struct addr_info
{
    int 		portnum[MAX_NET_PORT_NUM];
    int         portcount;
    UINT16		vid;
    char    	dst_mac[6];
    int			destnum;
    DST_ADDR	dst_address[MAX_NET_PORT_NUM];
} PORT_INFO;

typedef struct tag_gse_net_info
{
    int 			num;
    NETTYPE			type;
    PORT_INFO		addr[MAX_NET_PORT_NUM];
} NET_INFO;


typedef struct tag_gse_rx_rtt_info
{
    UINT32      	timeToLive;
    MMS_UTC_TIME   	utcTime;
    UINT32         	stNum;
    UINT32         	sqNum;
    BOOL           	test;
    BOOL           	ndscom;
    int				count;
    UINT32			retTime;
    UINT32			retTimeOrigin;
    UINT8			stat;
    UINT8			statOld;
    UINT8			statOrigin;
    BOOL			bRebootFlag;
} RTT_INFO;

typedef struct tag_gse_user_info
{
    UINT16  			appID;
    char                *pIEDName;  /* IED名称 */
    char           		*gcRef;
    char           		*dataSetRef;
    char            	*goID;
    INT32          		confRev;
    INT32      			DataNum;
    char                *ybLogicID; /* 压板的逻辑标识*/
    INT16               ybIndex;
    char                *admYbLogicID; /* 管理压板的逻辑标识*/
    BOOL                GmrpFlag;
} SUB_USER_INFO;

typedef struct tag_gse_user_info2
{
    UINT16  	appID;
    char        *pIEDName;  /* IED名称 */
    char        *gcRef;
    char        *dataSetRef;
    char        *goID;
    INT32       confRev;
    INT32      	DataNum;
    INT32		T0;
    INT32		T1;
    INT8		Priority;
} PUB_USER_INFO;

typedef struct tag_gse_user_info3
{
    BOOL        GmrpFlag;
} TRANSMIT_USER_INFO;

typedef struct	sub_info 	GSE_SUB_INFO;

typedef struct	pub_info	GSE_PUB_INFO;

typedef struct  transmit_info   GSE_TRANSMIT_INFO;

typedef struct tag_gse_sub_map_info
{
    int 							index;
    int								DaIndex;
    int								cpuid;
    int  							ordinal;
    DA_INFO *pa; /* 所对应的数据集数据 */
    int								dType;/*1-samepole 2-hdlbox*/
    VALUETYPE						type;
    DA_VALUE						*value;
    DA_VALUE						*packettime;
    GSE_SUB_INFO					*pInfoNode;
    char							*desc;
    UINT8							state;
    struct 	tag_gse_sub_map_info 	*next;
    struct tag_gse_sub_map_info 	*child;
    int *piYabanIndex;
    uint16_t                        AIOIndex; /* 多合一序号 */
    uint8_t				*pPoolIndex;
} SUB_MAP_INFO;


typedef struct tag_gse_pub_map_info
{
    int 							index;
    int								cpuid;
    int  							ordinal;
    int								dType;/*1-samepole 2-hdlbox*/
    VALUETYPE						type;
    int                             timesourcediindex[MAX_TIME_SOURCE_DI_NUM];
    float							threshold;
    DA_VALUE						*value[MAX_PUB_MAPPING_TIMES];
    GSE_PUB_INFO					*pInfoNode[MAX_PUB_MAPPING_TIMES];
    UINT8							state;
    char							*desc;
    char *linkLogicID; /* 压板的逻辑标识, 每个开出对应一个(目前仅能对应一个), 可不关联 */
    BOOL bIsCfgLink; /* 是否关联压板 */
    struct 	tag_gse_pub_map_info 	*next;
    struct tag_gse_pub_map_info 	*child;
} PUB_MAP_INFO;

/****************************************
*
*/
struct	sub_info
{
    int					GcbIndex;   /*从1开始，与转发时的GOOSE分不同口转发的GCB索引一致*/
    NET_INFO 			NetInfo;
    SUB_USER_INFO		UserInfo;
    RTT_INFO			RttInfo[MAX_NET_PORT_NUM];
    RTT_INFO			RttInfoAll;
    DA_INFO				*pDataMbr;
    uint8_t				PoolIndex;
    uint32_t ulRcvRrmCnt; /* 入库帧数 */
    uint32_t ulRcvCnt; /* 接收帧数 */
    uint32_t ulRcvCfgErrCnt; /* 接收异常帧计数 */
    void * pSubGooseGcb;        /*对应CCD文件中的发送控制块信息 SUB_GCB_INFO *pSubGooseGcb*/
    /*该结构用于CCD导出CC板GS文件中所有GOOSE块，包括sub pub,
    该变量表示是否是pub的GOOSE块,用于在GS文件中增加<SRC_MAC>字段,
    用于CC板转发时替换源MAC地址*/
    BOOL bIsPubGcb;
    struct	sub_info	*next;
};


typedef struct	sub_pool
{
    char			port[MAX_NET_PORT_NUM];
    GSE_SUB_INFO	*pSubInfoRootNode;
    SUB_MAP_INFO	*pSubMapRootNode;
} GSE_SUB_POOL;



struct	pub_info
{
    int					GcbIndex;
    NET_INFO 			NetInfo;
    PUB_USER_INFO		UserInfo;
    RTT_INFO			RttInfo;
    DA_INFO				*pDataMbr;
    void * pPubGooseGcb;        /*对应CCD文件中的发送控制块信息 PUB_GCB_INFO *pPubGooseGcb*/
    uint32_t ulSndCnt; /* 发送帧数 */
    struct	pub_info	*next;
};


typedef struct	pub_pool
{
    GSE_PUB_INFO	*pPubInfoRootNode;
    PUB_MAP_INFO	*pPubMapRootNode;
} GSE_PUB_POOL;

struct	transmit_info
{
    int					GcbIndex;
    NET_INFO 			NetInfo;
    TRANSMIT_USER_INFO	UserInfo;
    /*RTT_INFO			RttInfo[MAX_NET_PORT_NUM];
    DA_INFO				*pDataMbr;*/
    struct	transmit_info	*next;
};

typedef struct	transmit_pool
{
    char			port[MAX_NET_PORT_NUM];
    GSE_TRANSMIT_INFO	*pTransmitInfoRootNode;
} GSE_TRANSMIT_POOL;

/* 光功率报文队列元素 */
typedef struct
{
    uint32_t ulData[MAX_RX_MSG_LEN/4+1];
    int32_t ulLen;
    uint8_t ucAddr;
} T_WATT_DATA_ELE, *pT_WATT_DATA_ELE;

/* 报文队列 */
typedef struct
{
    T_WATT_DATA_ELE *base;
    int front;
    int rear;
} T_WATT_QUEUE;

/* globals */

extern T_WATT_QUEUE g_OptWattRcv;
extern int g_FccDealCount;  /* FCC口每次查询处理个数 */
extern BOOL g_FccNetOverFlow;  /* FCC口是否网络风暴 */

/****************************************
*全局函数声明
*/
/****************************************
*获取订阅GOOSE存储空间根指针
*/
GSE_SUB_POOL 	*GetSubPool();
/****************************************
*获取订阅GOOSE存储信息跟指针
*/
GSE_SUB_INFO 	*GetSubInfoRootNode();
/****************************************
*
*/
SUB_MAP_INFO 	*GetSubMapRootNode();
/****************************************
*全局函数声明
*/
GSE_PUB_POOL 	*GetPubPool();
/****************************************
*
*/
GSE_PUB_INFO 	*GetPubInfoRootNode();
/****************************************
*
*/
PUB_MAP_INFO 	*GetPubMapRootNode();
/****************************************
*
*/
GSE_TRANSMIT_POOL 	*GetTransmitPool();
/****************************************
*
*/
GSE_TRANSMIT_INFO 	*GetTransmitInfoRootNode();
/****************************************
*
*/
GSE_SUB_INFO 	*NewSubInfoNode();
/****************************************
*
*/
SUB_MAP_INFO 	*NewSubMapInfoNode();
/****************************************
*
*/
GSE_PUB_INFO 	*NewPubInfoNode();
/****************************************
*
*/
PUB_MAP_INFO 	*NewPubMapInfoNode();
/****************************************
*
*/
GSE_TRANSMIT_INFO 	*NewTransmitInfoNode();
/****************************************
*
*/
DA_INFO 	 	*NewDaInfo();
/****************************************
*
*/
DA_VALUE 	 	*NewDaValue();
/****************************************
*
*/
DA_VALUE 		*NewDaValueDouble();
/****************************************
*
*/
DA_INFO 		*UpdataSubInfo(GSE_IEC_INFO *pdu,int port,int len, GSE_SUB_INFO **pps, int *netno);
/****************************************
*
*/
int 		 	GetSubNetPort(int *port);
/****************************************
*
*/
void 			CheckGoRxTask();
/****************************************
*
*/
void 		 	DisplaySubPool();
/****************************************
*
*/
void 		 	DisplayPubPool();

/****************************************
*
*/
BOOL			CheckMbrDaValue(char *ptr,int len,DA_INFO *pa);
/****************************************
*
*/
BOOL 			UpdataMbrDaValue(char *ptr,int len,DA_INFO *pa,BOOL btimeflag,MMS_UTC_TIME *utctime,GSE_SUB_INFO *ps);
/****************************************
*
*/
BOOL 		 	LinkSubToMapData();
/****************************************
*
*/
BOOL 		 	LinkPubToMapData();
/****************************************
*
*/
void            CheckGoRx(UINT32 TimeElapsed);
/****************************************
*
*/
int             LoadGoCfgFile(char *file);
/****************************************
*
*/
BOOL 			goose_rx_start();
BOOL            goose_tx_start();
BOOL            goose_trans_start();

void            GoPubHandle(UINT32 timeElapsed, MMS_UTC_TIME *pUTCTime);


/****************************************
*供平台调用接口函数
*/
/****************************************
*全局函数声明
*/
int 			notify_61850_prot_data_update(unsigned long appId,
        MMS_UTC_TIME*pUTCTime,
        BOOL bDataIsUpdate,
        unsigned long ulUsCnt);
/****************************************
*
*/
BOOL 			ReadSubStat(int iSubIndex, int  *piRtValidNetCnt,
                            unsigned char *pucRtSubStatArrBase,
                            unsigned char *pucRtSubStatArrOrigin);

/****************************************
*
*/
BOOL 			GetSubTestMode(int iSubIndex);

/* 获取检修位(根据MAP).
 * Para:
 *     pm, MAP数据.
 * Return:
 *     TRUE: 检修状态; FALSE: 非检修状态.
 */
BOOL GetSubTestModeByMap(SUB_MAP_INFO *pm);

/****************************************
*
*/
char 			*QuerySubGcRefByIdx(int idx);
/****************************************
* 通过SUB序号查询相应压板的逻辑标识
*/
char *QuerySubYbIdByIdx(int idx);
/****************************************
*
*/
void            goose_record_do(int DoIdx,BOOL curValue);
/****************************************
*
*/
BOOL			InitAllGoYabanInfo();
/****************************************
*
*/
void  			RefreshAllGoYabanSts();
/****************************************
*
*/
BOOL			InitPubGoYabanInfo();
/****************************************
*
*/
extern	BOOL 	goose_task_start();
/****************************************/

int Drive_Breakout_Goose_Pub(unsigned long appId,
                             MMS_UTC_TIME *pUTCTime);

int Goose_Publisher_Start(int PubTaskPRI);

void AddSubInfoYabanInfo();

UINT32 Get_Goose_Cfg_Status();

int Get_Goose_Sub_Num();

int Get_Goose_Pub_Num();

void Set_Goose_Platform_Support_Func_Code(uint8_t FuncCode);

/* GOOSE配置解析
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
BOOL goose_cfg_start(void);

/* 获取IED名称
 * Para:
 *     NONE.
 * Return:
 *     名称地址.
 */
char *QuerySubIEDNameByIdx(int idx);

/* 初始化光功率报文队列.
 * Para:
 *     Q, 队列指针.
 * Return:
 *     OK, or ERROR.
 */
extern STATUS GsInitQue(T_WATT_QUEUE *Q);

/* 获取光功率报文队列写入点.
 * Para:
 *     Q, 队列指针.
 *     p, 写入点指针.
 * Return:
 *     OK, or ERROR.
 */
STATUS GsEnQue(T_WATT_QUEUE *Q, T_WATT_DATA_ELE **p);

/* 读取光功率报文队列.
 * Para:
 *     Q, 队列指针.
 *     p, 读取点指针.
 * Return:
 *     OK, or ERROR.
 */
STATUS GsDeQue(T_WATT_QUEUE *Q, T_WATT_DATA_ELE **p);

/* 设置GOOSE DI直接控制输出.
 * Para:
 *     lIndex, datamap通道索引, 从1开始.
 *     lModAddr, 模件地址, 从0开始.
 *     lDoChnNum, 通道序号, 从0开始.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SetGsDiToDo(int32_t lIndex, int32_t lModAddr, int32_t lDoChnNum);

/* 根据GOOSE DO序号获取压板序号.
 * Para:
 *     cpuid, CPU点号.
 *     pointid, GOOSE DO序号, 从1开始.
 *     typeid, 类型.
 *     pVtValType, 类型.
 * Return:
 *     压板序号(从0开始), or -1(无压板控制).
 */
int16_t GetPubGoLinkIndexByDoNum(int cpuid, int pointid, int typeId, VALUETYPE *pVtValType);

/* 获取通信状态更新标志
 * Para:
 *     void
 * Return:
 *     BOOL TRUE or FALSE
 */
BOOL GetCommStsChangeFlag();

/* 清除通信状态更新标志
 * Para:
 *     void
 * Return:
 *     void
 */
void ClearCommStsChangeFlag();

/* 通过SUB序号查询SUB信息 */
GSE_SUB_INFO *QuerySubByIdx(int idx);

#ifdef __cplusplus
}
#endif


#endif

