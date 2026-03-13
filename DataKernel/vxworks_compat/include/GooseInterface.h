/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      GooseInterface.h                                    1.0                 */
/*                                                                              */
/*                                                                              */
/*     文件描述                                                                 */
/*                                                                              */
/*       该文件定义了智能操作箱数据模块的头文件                                 */
/*                                                                              */
/*      作者                                                                    */
/*                                                                              */
/*      DQ                                                                      */
/*                                                                              */
/*                                                                              */
/*      开发记录                                                                */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/********************************************************************************/


#ifndef GOOSEINTERFACE_H
#define GOOSEINTERFACE_H

#include "iecgoose.h"


#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_GENERAL_PORT_NUM 2
#define MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM 9	/*包含一个SPT总线接口*/
#define MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT 8	/*不包含SPT总线接口*/
#define MAX_INTERNAL_FPGA_TRANSMITTER_GOOSE_PORT_NUM 2
#define MAX_EXTERN_PORT_NUM	(MAX_GENERAL_PORT_NUM+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM)
#define MAX_EXTERN_PORT_NUM_WITHOUT_SPT	(MAX_GENERAL_PORT_NUM+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)
#define MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM (MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM+MAX_INTERNAL_FPGA_TRANSMITTER_GOOSE_PORT_NUM)
#define MAX_FPGA_TO_CPU_PORT_NUM 1
#define MAX_CPU_GOOSE_PORT_NUM  (MAX_GENERAL_PORT_NUM+MAX_FPGA_TO_CPU_PORT_NUM)

#define GENERAL_NET_A   0
#define GENERAL_NET_B   1
#define NET_FPGA_TO_CPU_A     2
#define FPGA_GOOSE_NET_A    2
#define FPGA_GOOSE_NET_B    3
#define FPGA_GOOSE_NET_C    4
#define FPGA_GOOSE_NET_D    5
#define FPGA_GOOSE_NET_E    6
#define FPGA_GOOSE_NET_F    7
#define FPGA_GOOSE_NET_G    8
#define FPGA_GOOSE_NET_H    9

#define FPGA_GOOSE_NET_SPT    10
#define FPGA_GOOSE_NET_CPU    11

#define   GO_TASK_BUF_SIZE		50000   /*goose任务所需的BUF SIZE大小，用于BRGB的空间，  */
#define   MAX_ALLOW_SUB_GO_NUM	64   	/*允许最大的sub goose个数，2007-7-3  */
#define   MAX_GSE_NET_CNT		MAX_EXTERN_PORT_NUM       /*2007-7-17日 张云, 允许保护goose的最大网络个数 */

#define STRING_MAX_LEN         128
#define GOOSE_LINE_MAX_LEN     512
#define GOOSE_MEMBER_MAX_NUM   256       //member max num per grp
#define GOOSE_DA_MAX_NUM       512*6      //da max num of all sub grp
#define PUB_GOOSE_MAX_NUM      16       //pub goose grp max num
#define SUB_GOOSE_MAX_NUM      32       //sub goose grp max num
#define DATA_ZONE_MAX_LEN      GOOSE_DA_MAX_NUM*100

/*主动发送的goose数据源类型 按位依次分配，便于或操作  以后可扩展，2007-3-27 张云*/
#define  NOT_PROT_DATA_TYPE		0X00000000  /* 非保护数据源					*/
#define  SAME_POLE_GO_SRC_TYPE	0X00000001  /* 同杆并架数据源 	第1个BIT	*/
#define  HDL_BOX_GO_SRC_TYPE	0X00000002  /* 智能操作箱数据源 第2个BIT	*/

#define INFO_NOTIFY_OK    		(0x00)
#define INFO_NOTIFY_WAITTING	(0x01)
#define INFO_NOTIFY_ERROR		(0x02)
#define INFO_NOTIFY_MSGQ_FULL	(0x03)

#define SUB_IED_COM_ERR         4
#define SUB_IED_OVERTIME        5
#define SUB_IED_FAULT           6
#define SUB_IED_IN_REPAIR       7
#define SUB_IED_TEST_MODE       SUB_IED_IN_REPAIR
#define SUB_IED_OK              8
#define SUB_IED_CONFREV_ERR     9   /*2007-7-7日张云添加,SUB关联的CONFREV版本不对  */
#define SUB_IED_YABAN_EXIT      10   /*2007-7-10日张云添加,SUB关联的压板退出    */

/********************************************************************************/
/*						    与平台接口相关信息	          						*/
/********************************************************************************/
/* 61850调用平台接口的初始化 */
typedef void (*CfgErrLog)(uint8_t *strFmt, int iArg1,int iArg2);
typedef uint32_t (*GetDicTime)(int nIdx);
typedef void (*SetSubGoYBSts)(int subgo_num, BOOL sts);
typedef	BOOL (*SynTimeOK)();
typedef int (*Get_Yaban_Num)(uint8_t *strID, int16_t *pnRtNum);
typedef int (*Get_Yaban_Value)(int16_t nNum, BOOL *pbRtYabanValue, uint32_t ulScnTime);

typedef struct plat_interface
{
    uint8_t		goose_net_mode;

    CfgErrLog		cfgerr_fun;	/* 配置错误记录日志接口 */
    GetDicTime		getdic_fun; /* 获取DI变位时间接口 */
    SetSubGoYBSts 	set_subgo_yb_fun;	/* 设置sub go 压板状态 */
    SynTimeOK		syn_time_ok;/* 对时是否成功 */
    Get_Yaban_Num	get_yaban_num;/* 根据压板的逻辑标识，获得压板的相关信息 */
    Get_Yaban_Value	get_yaban_val;/* 根据压板的压板号，获取压板当前值 */
} Plat_Interface;

//void init_goose_interface(Plat_Interface *plat_if);

/********************************************************************************/
/*						    goose_rsv相关接口	          						*/
/********************************************************************************/
#define MEA_TYPE_VALUE          10
#define DI_TYPE_VALUE           11
#define TIMESTAMP_TYPE_VALUE    12
#define Q_TYPE_VALUE            13

#define FLOAT_TYPE_MEA          16
#define INT32_TYPE_MEA          17
#define SINGLE_TYPE_DI          18
#define DUAL_TYPE_DI            19
#define UTC_TYPE_TIME           20

#define GOOSE_NET_NUM           4

/*
Defined some goose data type for returnning to application
*/
#if 0
typedef struct tag_mea_value
{
    int flag;   //FLOAT_TYPE_MEA|INT32_TYPE_MEA
    int i;
    float f;
} MEA_VALUE;

typedef struct tag_di_value
{
    int flag;   //SINGLE_TYPE_DI|DUAL_TYPE_DI
    int cls;    //flag==DUAL_TYPE_DI  1:OPEN  2:CLOSE   0,3:中间态   0x80:错误
    //flag==SINGLE_TYPE_DI  0:OPEN  1:CLOSE
} DIGI_VALUE;

typedef struct tag_timestamp
{
    int flag;    //UTC_TYPE_TIME
    int yr;
    int mon;
    int dy;
    int hr;
    int min;
    int sec;
    int msec;
    int invalid;
} TIMESTAMP_VALUE;

enum qflag_valid {data_good=1,data_invalid,data_reserved,data_questionable};
enum qflag_source {src_process=1,src_substitued};
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
    MEA_VALUE mea_val;
    DIGI_VALUE di_val;
    TIMESTAMP_VALUE tm_val;
    QUALLITY_VALUE q_val;
};
typedef struct tag_Goose_Da_Value
{
    int nDaType;    //MEA_TYPE_VALUE | DI_TYPE_VALUE | TIMESTAMP_TYPE_VALUE | Q_TYPE_VALUE
    union DaValue da_val;
} GOOSE_DA_VALUE;
#endif

/********************************************************************************/
/*							get_sub_mea_info_from_iec61850						*/
/********************************************************************************/
/* The max lenth of the ldName and leafName is 256.								*/
/*------------------------------------------------------------------------------*/
/* cpuId:		the cpu num of the data.										*/
/* pointId:		the sequence num of the data in the cpu.						*/
/* appId:		the type of the data. (type1 | type2 | SAME_POLE_GO_SRC_TYPE)	*/
/* ldName:		the name of the LD(logica domain).								*/
/* leafName:	the data attribute name of the LN(logical node).				*/
/*------------------------------------------------------------------------------*/
/* If get the info success, return GET_INFO_OK.									*/
/* If one parameter ptr is NULL, return PARA_ERROR.								*/
/* If the info is not ready, return HAVE_NO_INIT.								*/
/* If have no the successful result, return HAVE_NO_INFO.						*/
/********************************************************************************/
int get_sub_mea_info_from_iec61850( unsigned char cpuId, unsigned short pointId,
                                    unsigned long appId, char *ldName, char *leafName);

/********************************************************************************/
/*							get_sub_rsv_info_from_iec61850						*/
/********************************************************************************/
/* The max lenth of the ldName and leafName is 256.								*/
/*------------------------------------------------------------------------------*/
/* cpuId:		the cpu num of the data.										*/
/* pointId:		the sequence num of the data in the cpu.						*/
/* appId:		the type of the data. (type1 | type2 | SAME_POLE_GO_SRC_TYPE)	*/
/* ldName:		the name of the LD(logica domain).								*/
/* leafName:	the data attribute name of the LN(logical node).				*/
/*------------------------------------------------------------------------------*/
/* If get the info success, return GET_INFO_OK.									*/
/* If one parameter ptr is NULL, return PARA_ERROR.								*/
/* If the info is not ready, return HAVE_NO_INIT.								*/
/* If have no the successful result, return HAVE_NO_INFO.						*/
/********************************************************************************/
int get_sub_rsv_info_from_iec61850(unsigned char cpuId, unsigned short pointId,
                                   unsigned long appId, char **ldName,
                                   char **leafName, int *nNum);


/*根据某SubGoose的Idx，获得其GcRef 2007-11-11日 DQ
  参数， iIdx，SubGoose的索引号，(从0开始)
  返回，字符串指针，NULL－查找失败
*/
char *QuerySubGcRefByIdx(int iIdx);


/*功能：初始化所有的goose 压板信息，必须在goose.ini初始化和软件配置初始化之后，保护驱动之前调用
  参数，无
  返回，操作成功与否  张云 2007-7-7 */
BOOL   InitAllGoYabanInfo();

BOOL GetSubTestMode(int nIdx);

/* 获取检修位(根据MAP).
 * Para:
 *     pm, MAP数据.
 * Return:
 *     TRUE: 检修状态; FALSE: 非检修状态.
 */
extern BOOL GetSubTestModeByMap(SUB_MAP_INFO *pm);

/*根据SUB INDEX，获得所有有效网络端口的状态 2007-7-10日 张云
   参数，　iSubIndex,SUB的INDEX
           piRtValidNetCnt,返回有效网络的个数
           pucRtSubStatArrBase,返回的该SUB在有效网络的状态数组的基址,由本函数填充数组内容,数组空间由调用函数分配
               SUB_IED_COM_ERR|SUB_IED_OVERTIME|SUB_IED_FAULT|SUB_IED_IN_REPAIR|SUB_IED_OK|SUB_IED_CONFREV_ERR|SUB_IED_YABAN_EXIT
   返回：  TRUE，操作成功
           FALSE，操作失败*/
BOOL ReadSubStat(int iSubIndex, int  *piRtValidNetCnt, unsigned char *pucRtSubStatArrBase, unsigned char *pucRtSubStatArrOrigin);

/* 记录DO变化 */
//VOID goose_record_do(unsigned int do_num, uint8_t cursts);

/*
 * 驱动GOOSE重发和心跳，需由应用定时调用
 */
int drive_goose(unsigned long appid, unsigned long ulUsCnt);

/*根据某DA的DaIdx，获得其所在的Sub IDX  2007-6-23日 张云
  参数， iDaIdx，根据某DA的DaIdx，
         piRtSubIdx，返回的SUB,IDX
  返回，TRUE，查找成功
        FALSE，查找失败 */
BOOL   QuerySubGoIdxByDaIdx(int  iDaIdx,int  *piRtSubIdx);

/*
Description: read sub goose da value indexed by pSubDa,
             the result is storaged in Da_Val;
*/
BOOL readSubGooseDa(int nSubDaIdx, GOOSE_DA_VALUE *Da_Val, int *nStat);

BOOL readSubGooseDa_T_New(int nSubDaIdx, TIMESTAMP_VALUE *t_Val);

/********************************************************************************/
/*							  注册以太网回调函数						        */
/********************************************************************************/
#define ETH_TYPE_GOOSE	0x88B8
#define ETH_TYPE_GSE	0x88B9
#define ETH_TYPE_SV		0x88BA

/* register ethernet callback function */
//typedef int (*eth_cb_func)(uint8_t port, uint8_t *buf, int32_t len);

//BOOL eth_register_dissector(uint8_t port, uint16_t eth_type, eth_cb_func func);

/**
 * 获取GOOSE网络运行方式
 *
 */
#define GOOSE_MODE_SINGLE_A  	0x01
#define GOOSE_MODE_SINGLE_B  	0x02
#define GOOSE_MODE_DOUBLE		0x04
#define GOOSE_MODE_SINGLE_REDU	0x08

//uint8_t GetNetMode();

/**
 * C口是否使用
 *
 */
//BOOL IsPortCUsed();

STATUS reg_Int_Goose_Recv_Fun(uint8_t port, eth_cb_func func);

#ifdef	__cplusplus
}
#endif
#endif
