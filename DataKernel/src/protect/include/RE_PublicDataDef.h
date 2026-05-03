/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_PublicDataDef.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块所需要的公共定义                               */
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
/*         张云       2002.11.27              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_PUBLICDATADEF_H
#define RE_PUBLICDATADEF_H


#include <vxWorks.h>
#include "edpbase.h"
#include   "logic.h"

#include  "errtest.h"
#include  "logmsg.h"
#include   "realdata.h"
#include    "swcfg.h"
#include    "view.h"
#include    "rec.h"
#include    "stdio_compat.h"
#include    "semLib.h"
#include   "RE_ListLib.h"
#include  "RE_RelayEngine.h"

/* 合并版所有平台包含 */
#include    "measure.h"



enum   Figure_TYPE/*各种图形类型，以后可以适当调整， */
{
    RE_SUANFA=0X0,
    RE_AND=0X4,
    RE_OR=0X5,
    RE_NOT=0X6,
    RE_TIMER=0xC,
    RE_YABAN=0xD,
    RE_CONTROLWORD=0XE,
    RE_ZERO=0X10,
    RE_ONE=0X11,
    RE_GREATERTHAN=0X14,
    RE_LESSTHAN=0X15,
    RE_EQUAL=0X16,
    RE_OUTERINPUT=0x1C,
    RE_OUTEROUTPUT=0X1D,
    RE_LUBO=0X20,
    RE_EVENT=0X21,
    RE_MULTIWAYSELECT=0X22,
    RE_IMPORT=0X26,
    RE_EXPORT=0X27,
    RE_SETTING=0X28,
    RE_RELAYSTART=0X2C,
    RE_REPORTSTART=0X2D,
    RE_DIMOD=0X30,
    RE_DISET=0X31,
    RE_DOSET=0X32,
    RE_AISET=0X33,
    RE_AMPANG=0X38,
    RE_REALIMAGE=0X39,
    RE_MAX=0X3A,
    RE_MIN=0X3B,
    RE_MODEWORD=0X3C,
    RE_OUTERORDER=0X40,
    RE_SYSSERVOUT=0X41
};




enum   TuyuanScan_TYPE/*各种图元扫描类型，以后可以适当调整， */
{
    RE_SUANFA_SCAN=0X0,

    RE_2_AND_SCAN=0X4,
    RE_3_AND_SCAN=0X5,
    RE_4_AND_SCAN=0X6,
    RE_5_AND_SCAN=0X7,
    RE_6_AND_SCAN=0X8,
    RE_7_AND_SCAN=0X9,
    RE_8_AND_SCAN=0XA,
    RE_MULTI_AND_SCAN=0XF,

    RE_2_OR_SCAN=0X10,
    RE_3_OR_SCAN=0X11,
    RE_4_OR_SCAN=0X12,
    RE_5_OR_SCAN=0X13,
    RE_6_OR_SCAN=0X14,
    RE_7_OR_SCAN=0X15,
    RE_8_OR_SCAN=0X16,
    RE_MULTI_OR_SCAN=0X1F,

    RE_NOT_SCAN=0X20,

    RE_TIMER_SCAN=0x24,

    RE_YABAN_SCAN=0x28,

    RE_CONTROLWORD_SCAN=0X2B,

    RE_ZERO_SCAN=0X30,

    RE_ONE_SCAN=0X34,

    RE_FLOAT_GREATERTHAN_SCAN=0X38,
    RE_UNSIGNED_32INT_GREATERTHAN_SCAN=0X39,
    RE_SIGNED_32INT_GREATERTHAN_SCAN=0X3A,

    RE_FLOAT_LESSTHAN_SCAN=0X40,
    RE_UNSIGNED_32INT_LESSTHAN_SCAN=0X41,
    RE_SIGNED_32INT_LESSTHAN_SCAN=0X42,

    RE_FLOAT_EQUAL_SCAN=0X48,
    RE_UNSIGNED_32INT_EQUAL_SCAN=0X49,
    RE_SIGNED_32INT_EQUAL_SCAN=0X4A,

    RE_FLOAT_AI_OUTERINPUT_SCAN=0x50,
    RE_COMPLEX_AI_OUTERINPUT_SCAN=0x51,
    RE_DI_OUTERINPUT_SCAN=0x52,
    RE_PULSE_OUTERINPUT_SCAN=0X53,
    RE_MEA_AI_OUTERINPUT_SCAN=0X54,

    RE_AI_PLUSCOF_OUTERINPUT_SCAN=0X55,
    RE_AI_OFFCOF_OUTERINPUT_SCAN=0X56,
    RE_CL_PLUSCOF_OUTERINPUT_SCAN=0X57,
    RE_CL_OFFCOF_OUTERINPUT_SCAN=0X58,
    RE_PO_OUTERINPUT_SCAN=0X59,

    RE_DINGZHI_OUTERINPUT_SCAN=0x5A,
    RE_CONSTVALUE_OUTERINPUT_SCAN=0x5B,

    RE_AI_PROCOF_OUTERINPUT_SCAN=0X5C,

    RE_DO_OUTEROUTPUT_SCAN=0X60,
    RE_LAMP_OUTEROUTPUT_SCAN=0X61,
    RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN=0X62,
    RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN=0X63,
    RE_OPTAO_OUTEROUTPUT_SCAN=0X64,/* 光纵AO输出 或励磁ao输出*/
    RE_PO_OUTEROUTPUT_SCAN=0X65,
    RE_AIPLUSADT_OUTEROUTPUT_SCAN=0X66,
    RE_AIOFFADT_OUTEROUTPUT_SCAN=0X67,
    RE_CLPLUSADT_OUTEROUTPUT_SCAN=0X68,
    RE_CLOFFADT_OUTEROUTPUT_SCAN=0X69,
    RE_DIFILTADT_OUTEROUTPUT_SCAN=0X6A,
    RE_YCOVERADT_OUTEROUTPUT_SCAN=0X6B,
    RE_CLOVERADT_OUTEROUTPUT_SCAN=0X6C,

    RE_EXTERN_IMPORT_OUTERINPUT_SCAN=0x70,

    RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN=0X74,

    RE_TRIP_ENABLE_OUTEROUTPUT_SCAN=0X78,

    RE_REPORT_ENABLE_OUTEROUTPUT_SCAN=0X7C,

    RE_ONLYSTART_LUBO_SCAN=0X80,
    RE_ONLYSTOP_LUBO_SCAN=0X81,
    RE_STARTSTOP_LUBO_SCAN=0X82,

    RE_EVENT_SCAN=0X84,

    RE_2WAY_MULTIWAYSELECT_SCAN=0X88,
    RE_3WAY_MULTIWAYSELECT_SCAN=0X89,
    RE_MULTIWAY_MULTIWAYSELECT_SCAN=0X8A,

    RE_DI_MOD_SCAN=0X8C,

    RE_DI_SET_SCAN=0X90,

    RE_FLOAT_AI_SET_SCAN=0X94,
    RE_CMPLX_AI_SET_SCAN=0X95,

    RE_DO_SET_SCAN=0X98,

    RE_AMPANG_SCAN=0X9C,

    RE_REALIMAGE_SCAN=0XA0,

    RE_FLOAT_MAX_SCAN=0XA4,
    RE_UNSIGNED_32INT_MAX_SCAN=0XA5,
    RE_SIGNED_32INT_MAX_SCAN=0XA6,

    RE_FLOAT_MIN_SCAN=0XAC,
    RE_UNSIGNED_32INT_MIN_SCAN=0XAD,
    RE_SIGNED_32INT_MIN_SCAN=0XAE,

    RE_MODEWORD_SCAN=0XB4,

    RE_MEADO_OUTERORDER_SCAN=0XBA,
    RE_PLUSADJUST_OUTERORDER_SCAN=0XBB,
    RE_OFFADJUST_OUTERORDER_SCAN=0XBC,
    RE_POCLEAR_OUTERORDER_SCAN=0XBD,
    RE_ZHUBIAN_OUTERORDER_SCAN=0XBE,
    RE_JINXIAN_OUTERORDER_SCAN=0XBF,
    RE_FARSTSCHG_OUTERORDER_SCAN=0XD0,
    RE_YXJXCHG_OUTERORDER_SCAN=0XD1,
    RE_JGSCHG_OUTERORDER_SCAN=0XD2,

    RE_SETTINGSWITCH_SYSSERV_SCAN=0XC0,
    RE_REVERT_SYSSERV_SCAN=0XC1,
    RE_SWITCHFAR_SYSSERV_SCAN=0XC2,
    RE_SWITCHEXAM_SYSSERV_SCAN=0XC3,
    RE_THREEU0WARN_SYSSERV_SCAN=0XC4,
    RE_PLUSADTOVER_SYSSERV_SCAN=0XC5,
    RE_OFFADTOVER_SYSSERV_SCAN=0XC6,
    RE_YABANTT_SYSSERV_SCAN=0XC7,
    RE_SETAUTOSET_SYSSERV_SCAN=0XC8,	/* 参数自动整定 */
    RE_DBYX_SYSSERV_SCAN=0XC9,	/* 遥信 */
};







enum   Tuyuan_IOSignal_TYPE/*图元输入输出信号的类型定义*/
{
    /*图元的输入输出信号的类型,目前设定了这些,以后可以进行修改和添加*/
    CONTROL_WORD_SIGNAL=0x0,/* 控制字,无单位,用于定植访问  */

    LOGIC_SIGNAL=0x4,/*逻辑量信号,无单位*/

    REAL_FORM_DIANLIU_SIGNAL=0X8,/* 实数格式电流,单位为安，（包括多次谐波，零序，正序，负序，都可用此单位），*/
    COMPLEX_FORM_DIANLIU_SIGNAL=0X9,/* 复数格式电流,单位为安，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    VALUE_ANGLE_FORM_DIANLIU_SIGNAL=0XA,/*幅角式电流,单位为安，（包括多次谐波，零序，正序，负序，都可用此单位），*/
    REAL_FORM_DIANLIU_SIGNAL_KA=0XB,/*实数格式电流,单位为千安，（包括多次谐波，零序，正序，负序，都可用此单位）， */
    COMPLEX_FORM_DIANLIU_SIGNAL_KA=0XC,/*// 复数格式电流,单位为千安，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA=0XD,/*// 幅角式电流,单位为千安，（包括多次谐波，零序，正序，负序，都可用此单位），*/

    REAL_FORM_DIANLIU_SIGNAL_MA=0x10,/* 实数格式电流,单位为毫安，（包括多次谐波，零序，正序，负序，都可用此单位），*/

    REAL_FORM_DIANYA_SIGNAL=0X14,/*// 实数格式电压,单位为伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    COMPLEX_FORM_DIANYA_SIGNAL=0X15,/*// 复数格式电压,单位为伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    VALUE_ANGLE_FORM_DIANYA_SIGNAL=0X16,/*// 幅角式电压,单位为伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    REAL_FORM_DIANYA_SIGNAL_KV=0X17,/*// 实数格式电压,单位为千伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    COMPLEX_FORM_DIANYA_SIGNAL_KV=0X18,/*// 复数格式电压,单位为千伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/
    VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV=0X19,/*// 幅角式电压,单位为千伏，（包括多次谐波，零序，正序，负序，都可用此单位）*/

    REAL_FORM_ZUKANG_SIGNAL=0x1A,/*// 实数格式阻抗,单位为欧，*/
    COMPLEX_FORM_ZUKANG_SIGNAL=0X1B,/*// 复数格式阻抗,单位为欧*/
    VALUE_ANGLE_FORM_ZUKANG_SIGNAL=0X1C,/*// 幅角式阻抗,单位为欧*/
    REAL_FORM_ZUKANG_SIGNAL_KO=0x1D,/*// 实数格式阻抗,单位为千欧，*/
    COMPLEX_FORM_ZUKANG_SIGNAL_KO=0X1E,/*// 复数格式阻抗,单位为千欧*/
    VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO=0X1F,/*// 幅角式阻抗,单位为千欧*/

    SHIJIAN_TYPE1_SIGNAL=0X20,/*//时间类型1,单位为秒*/
    SHIJIAN_TYPE2_SIGNAL=0X21,/*//时间类型2,单位为毫秒*/
    SHIJIAN_TYPE3_SIGNAL=0X22,/*//时间类型3,单位为微秒*/
    SHIJIAN_TYPE4_SIGNAL=0X23,/*//时间类型4,单位为h*/

    PINLV_SIGNAL=0X28,/*//频率,单位为赫兹*/
    HAUACHA_SIGNAL=0X2B,/*//滑差,单位为赫兹/秒*/
    DIANYA_BIANHUALV_SIGNAL=0X30,/*//电压变化率,单位为伏/秒*/
    JIAODU_SIGNAL=0X34,/*//角度,单位为度*/
    WENDU_SIGNAL=0X38,/*//温度,单位为摄氏度*/
    JULI_SIGNAL=0X3B,/*//距离,单位为千米*/
    XIANGBIE_SIGAL=0X40,/*//故障相别，为枚举类型，无单位，用0---10整数表示相别，包括11种类型相别*/
    /*//以后根据需要添加*/

    BILIXISHU_SIGNAL=0X48,/*//比例系数,无单位*/
    CEJUXISHU_SIGNAL=0X4B,/*//测距系数,单位为千米/欧*/
    BUCHANGXISHU_SIGNAL=0X50,/*//补偿系数,无单位*/

    GONGLV_TYPE1_SIGNAL=0X54,/*//有功功率类型1,单位为瓦*/
    GONGLV_TYPE2_SIGNAL=0X55,/*//有功功率类型2,单位为千瓦*/
    GONGLV_TYPE3_SIGNAL=0X56,/*//有功功率类型3,单位为兆瓦*/
    GONGLV_TYPE4_SIGNAL=0X57,/*//有功功率类型4,单位为千兆瓦*/
    WUGONG_GONGLV_TYPE1_SIGNAL=0X5C,/*//无功功率类型1,单位为瓦*/
    WUGONG_GONGLV_TYPE2_SIGNAL=0X5D,/*//无功功率类型2,单位为千瓦*/
    WUGONG_GONGLV_TYPE3_SIGNAL=0X5E,/*//无功功率类型3,单位为兆瓦*/
    WUGONG_GONGLV_TYPE4_SIGNAL=0X5F,/*//无功功率类型4,单位为千兆瓦*/

    SHORT_INT_SIGNAL=0X60,  /*//16位整数*/
    LONG_INT_SIGNAL=0X61,   /*//32位整数*/
    REAL_SIGNAL=0X64,        /*//实数*/
    VAR_STRING =0X68,        /*不定长字符串*/
    HEX_MODE_WORD_SIGNAL=0X6B,    /*32位16进制方式字，无单位，32位无符号数*/
    CAPACITY_SIGNAL=0X70,         /*容量，单位为MVA，float型*/

    DIANDU_TYPE1_SIGNAL=0X74,
    DIANDU_TYPE2_SIGNAL=0X75,
    DIANDU_TYPE3_SIGNAL=0X76,
    DIANDU_TYPE4_SIGNAL=0X77,
    WUGONG_DIANDU_TYPE1_SIGNAL=0X7C,
    WUGONG_DIANDU_TYPE2_SIGNAL=0X7D,
    WUGONG_DIANDU_TYPE3_SIGNAL=0X7E,
    WUGONG_DIANDU_TYPE4_SIGNAL=0X7F,

    OHM_PER_METER=0X84,

};




/*  输入输出信号数据值的类型，  */

enum   IO_SIGNAL_VALUE_TYPE /*图元的输入输出信号数据值的类型*/
{
    BOOL_VALUE_TYPE=0, /*布尔*/
    FLOAT_VALUE_TYPE=1,/*实数*/
    COMPLEX_VALUE_TYPE=2,/*复数  */
    SIGNED_32INT_VALUE_TYPE=3,/*32位整数  */
    UNSIGNED_32INT_VALUE_TYPE=4,/*无符号32位整数  */
};


enum   Delay_Timer_Source_TYPE /*//延时继电器的延时来源的类型*/
{
    /*//一定要按这个顺序*/
    DINGZHI_SOURCE=0, /*//定值来源*/
    CONSTVALUE_SOURCE=1, /*//常数来源*/
};


enum   OuterInput_Source_TYPE /*外部通道输入图元类型*/
{
    ANALOG_INPUT_SOURCE=0,
    DIGITAL_INPUT_SOURCE=1,
    PULSE_INPUT_SOURCE=10,
    MEASURE_AI_SOURCE=11,
    PULSE_OUTPUT_SOURCE=12,
    AI_PLUSCOF_SOURCE=20,
    AI_OFFCOF_SOURCE=21,
    CELIANG_PLUSCOF_SOURCE=22,
    CELIANG_OFFCOF_SOURCE=23,
    AI_PROCOF_SOURCE=24,
};

enum OuterOrder_Source_TYPE   /*外部命令图元的输入类型*/
{
    MEA_DO=0,   /*遥控命令*/
    PLUS_ADJUST=1,    /*增益校准命令*/
    OFF_ADJUST=2,  /*偏置校准命令*/
    PULSE_OUTPUT_CLEAR=3, /*脉冲电度输出清零*/
    ZHUBIAN_CHG=4, 		/* 切换主变 */
    JINXIAN_CHG=5, 		/* 切换进线 */
    FAR_STS_CHG=6, 		/* 远方/就地切换 */
    YXJX_CHG=7, 		/* 运行检修切换 */
    JGS_CHG=8, 		/* 解挂锁切换 */
};

enum   Setting_Source_TYPE /*//定植输入信号的来源*/
{
    DINGZHI_INPUT_SOURCE=2,
    ANNLOG_CONSTVALUE_SOURCE=4,
};


enum   OuterOutput_DEST_TYPE /*//外部通道输出信号的目的地*/
{
    DIGITAL_OUTPUT_DEST=0,
    HINTLAMP_OUTPUT_DEST=2,
    AI_VIRTUAL_CH_DEST=6,
    ANALOG_OUTPUT_DEST=10,
    PULSE_OUTPUT_DEST=11,
    AI_PLUSADT_DEST=20,
    AI_OFFADT_DEST=21,
    CELIANG_PLUSADT_DEST=22,
    CELIANG_OFFADT_DEST=23,
    DI_XIAODOU_DEST=24,
    YAOCE_OVERADT_DEST=25,
    CELIANG_OVERADT_DEST=26,
};

enum SysServer_Dest_TYPE
{
    SETTING_SWITCH_DEST=0,
    REVERT_DEST=1,
    SWITCH_TO_FAR_DEST=2,
    SWITCH_TO_EXAM_DEST=4,
    THREEU0_WARN_DEST=6,
    PLUSADTOVER_DEST=7,
    OFFADTOVER_DEST=8,
    YABAN_TT_DEST=9,
    SETAUTOSET_DEST=10,			/* 参数自动整定 */
    DBYX_DEST=11,			/* 遥信 */
};
enum   LUBOSTARTSTOP_TYPE /*//录波启停类型*/
{
    LUBO_ONLY_START=0, /*//录波只启动*/
    LUBO_ONLY_STOP=1, /*//录波只停止*/
    LUBO_STARTSTOP=2, /*//录波先启动后又停止*/
};

enum   TRIGGER_TYPE /*//触发类型*/
{
    ADVANCING_EDGE=0, /*//上升沿*/
    FALLING_EDGE=1, /*//下降沿*/
    ADVC_OR_FALL_EDGE=2,/*上升沿或下降沿*/

};


enum   LUBO_PERSIST_TIME_TYPE /*//录波持续时间类型*/
{
    LUBO_PERSIST_FINITETIME=0, /*//录波持续有限时间*/
    LUBO_PERSIST_INFINITETIME=1,/*/ /录波持续无限时间*/

};



/*****************************保护引擎相关宏定义******************************************/



/* 定义求动态数组的个数的宏
   可用于求算法元件映射表的个数
  */

#define   DIMTABLE(x)   (sizeof(x)/sizeof(x[0]))



/**********允许创建的最多保护功能分图任务数目*************/

#define    MAX_CREATE_RELAYFUNC_TASK_COUNT     2



/**********允许的逻辑图处理的采样点和实际DSP最新的采样点之间的最大延迟点数*************/


#define MAX_LOGIC_SCAN_DELAY_SAMPLE_COUNT (20*uiAiPts_g) /* 扫描任务滞后退出门槛 */
#define DELAY_LOGIC_SCAN_DELAY_SAMPLE_COUNT (3*uiAiPts_g)  /* 逻辑图滞后二级门槛 */
#define ALARM_LOGIC_SCAN_DELAY_SAMPLE_COUNT (2*uiAiPts_g)  /* 逻辑图滞后一级门槛 */

#define RETURN_LOGIC_SCAN_DELAY_SAMPLE_COUNT (uiAiPts_g)  /* 返回门槛 */

/**********允许的最多投入运行的独立保护功能数目********************/

#define    MAX_RELAY_FUNC_COUNT     32



#define   MAX_LOGID_STR_LEN   128/*逻辑标识名的最大长度（不包括”\0“）
                                  必须等于文件读取字符串长度   */


#define    MAX_SUANFA_INPUT_COUNT   250/*算法元件允许的最大输入数   */


#define    MAX_AND_INPUT_COUNT    32/* 与门元件允许的最大输入数 */


#define    MAX_OR_INPUT_COUNT     32/* 或门元件允许的最大输入数 */


#define    MAX_CONTROLWORD_INPUT_COUNT    32/* 控制字元件允许的最大输入数 */


#define    MAX_EVENT_INPUT_COUNT   33/* 事件元件允许的最大输入个数,
                                        包括触发信号和参数 */


#define    MULTIWAYSELECT_INPUT_COUNT    32 /* 多路选通元件允许的最大输入数
                                               包括选通开关输入和选通信号输入
                                               必须为偶数 */

#define   MAX_DISET_OUTPUT_COUNT    32   /*DI集的最大输出个数  */

#define   MAX_DOSET_INPUT_COUNT    32   /*DO集的最大输入个数  */

#define   MAX_AISET_OUTPUT_COUNT    32   /*AI集的最大输出个数  */

#define   MAX_MAX_INPUT_COUNT      32    /*最大值图元的最大输入个数  */

#define   MIN_MAX_INPUT_COUNT      32     /*最小值图元的最大输入个数 */


#define  MAX_OUTERORDER_OUTPUT_COUNT 7  /*外部命令土缘的最大输出个数*/
#define  MAX_SYSSERVER_INPUT_COUNT       4   /*系统服务输出图元的最大输入格数*/
/* 各扫描任务同步刷新定植时最大的
同步延迟时间 ,以微秒为单位 ,目前设定为500毫秒*/
#define    REFRESH_DINGZHI_MAX_OVERTIME   500000



/*  逻辑图扫描任务的堆栈大小定义*/
#define     RELAY_SCAN_TASK_STACK_SIZE   50000  /* 堆栈大小为500k，改为50K DY 1/15/2007  */

/*计算任务每周期平均消耗时间的计数次数  2006-9-21*/
#define     CALC_TASK_TIME_PER_PERIOD_COUNT   0x1000  /* 为了计算提高效率，必须是2的整数次幂 */



/*  逻辑图扫描任务的允许端口引入图元的最大个数  2006-11-4日*/
#define  RELAY_SCAN_TASK_ALLOW_MAX_IMPORT_SIZE  4000  /* 最大个数为4k */

/*  逻辑图扫描任务的允许端口引出图元的最大个数*/
#define     RELAY_SCAN_TASK_ALLOW_MAX_EXPORT_SIZE   2000  /* 最大个数为2k */

/*  逻辑图扫描任务的允许光纵AO图元的最大个数  2006-11-11日  张云*/
#define     RELAY_SCAN_TASK_ALLOW_MAX_OPTAO_SIZE   100  /* 最大个数为100 */


/*********************保护功能模块所用到的数据结构和函数定义********************************/



typedef   struct
{
    /* 读取逻辑图文件的图元所传输的数据  */
    void  (*pfUser)(void *pvParm);/* 注册函数指针  */
    void *pvParm;   /* 注册函数的参数地址  */
    u_int uiPts;     /* 注册函数的采样节拍数*/

}  SAM_REGISTER_FUNC_INFO_TYPE;/* 采样注册函数信息结构定义类型  */




typedef   struct
{
    /* 读取逻辑图文件的图元所传输的数据  */
    EP_CHART_MSG *pchart;/* 所属逻辑分图的信息结构指针.该结构的内容在每次
                                扫描时,都会由上层程序更新  */
    BOOL   *pbScanRefreshDingzhiFlag ;/* 当前扫描需更新定值的标志指针
                                    (对需访问定值的图元有意义)  */
    long  nScanTaskNo;  /*  该图元扫描时所在的任务号,供初始化时设定录波,标志.
                               遥测,遥信时使用*/

}  TUYUAN_READ_FILE_INIT_DATA;/* 读取逻辑图文件的图元所传输的额外数据  */



typedef   struct
{
    /* 独立分图读取后的属性  */
    BOOL   bTaskOutFstFlag;/* 该分图投退标志，为真，表示投入，为假，表示退出  */
    BOOL   bRunFlag;/* 该分图投退标志，为真，表示投入，为假，表示退出  */
    unsigned  int   uiScanDriveInterval;  /* 分图扫描周期  */
    unsigned  char    ucScanAttr;                  /*分图扫描属性ghx20051231添加根据规约pc3.30*/
    long   nScanTaskNo;   /* 该分图所在的任务号，未投入的分图该占用的任务号还是占用 */
    char   strCurPartGrpName[300];/* 为了满足逻辑图中能设定是否退出投入 */

    /* 部分输入输出图元数组 */

    int SetScanNodeNum;   /* 定值图元个数 */
    NODE **ppSetNode;  /* 定值图元数组指针 */

    int DIScanNodeNum;   /* 开入图元个数 */
    NODE **ppDINode;  /* 开入图元数组指针 */

    int DIModScanNodeNum;   /* 开入模件图元个数 */
    NODE **ppDIModNode;  /* 开入模件图元数组指针 */

    int DISetScanNodeNum;   /* 开入集图元个数 */
    NODE **ppDISetNode;  /* 开入集图元数组指针 */

    int FloatAIScanNodeNum;   /* 实数AI图元个数 */
    NODE **ppFloatAINode;  /* 实数AI图元数组指针 */

    int CmplxAIScanNodeNum;   /* 复数AI图元个数 */
    NODE **ppCmplxAINode;  /* 复数AI图元数组指针 */

    int FloatAISetScanNodeNum;   /* 实数AI集图元个数 */
    NODE **ppFloatAISetNode;  /* 实数AI集图元数组指针 */

    int CmplxAISetScanNodeNum;   /* 复数AI集图元个数 */
    NODE **ppCmplxAISetNode;  /* 复数AI集图元数组指针 */

    int EventScanNodeNum;   /* 事件图元个数 */
    NODE **ppEventNode;  /* 事件图元数组指针 */

    int DOScanNodeNum;   /* DO图元个数 */
    NODE **ppDONode;  /* DO图元数组指针 */

    int DOSetScanNodeNum;   /* DO集图元个数 */
    NODE **ppDOSetNode;  /* DO集图元数组指针 */

    int LampScanNodeNum;   /* 指示灯图元个数 */
    NODE **ppLampNode;  /* 指示灯图元数组指针 */

    int OnlyStartLuboScanNodeNum;   /* 启动录波图元个数 */
    NODE **ppOnlyStartLuboNode;  /* 启动录波图元数组指针 */

    int OnlyStopLuboScanNodeNum;   /* 停止录波图元个数 */
    NODE **ppOnlyStopLuboNode;  /* 停止录波图元数组指针 */

    int StartStopLuboScanNodeNum;   /* 启动停止录波图元个数 */
    NODE **ppStratStopLuboNode;  /* 启动停止录波图元数组指针 */

    int externImportNum; 	/* 外部端口输入图元个数 */
    int externExportNum; 	/* 外部端口输入图元个数 */
    int constSource;  /* 常量图元个数 */
    int constZeroNum;    /* 恒0图元个数 */
    int constOneNum;       /* 恒1图元个数 */
    int optAoNum;  /* 光差输出图元个数 */

    int sfNum;  /* 算法图元个数 */
    int andNum;  /* and图元个数 */
    int orNum;  /* or图元个数 */
    int notNum;  /* not图元个数 */

    int timerNum;  /* timer图元个数 */
    int ybNum;  /* 压板图元个数 */
    int cwNum;  /* 控制字图元个数 */

    int bNum;  /* 大于图元个数 */
    int sNum;  /* 小于图元个数 */
    int eNum;  /* 等于图元个数 */

    int mwNum;  /* 多路开关图元个数 */

    int apNum;  /* 幅角图元个数 */
    int riNum;  /* 实虚图元个数 */
    int maxNum;  /* 最大图元个数 */

    int minNum;  /* 最小图元个数 */

    int mwdNum;  /* 模式字图元个数 */
    int owNum;  /* 外部命令图元个数 */
    int soNum;  /* 系统服务图元个数 */
    int rltNum;  /* 保护启动图元个数 */
    int sptNum;  /* 报告启动图元个数 */
}    PARTGRP_ATTRIB_TYPE;/* 独立分图的属性  */





typedef   struct
{
    /*  逻辑图文件读取后返回的属性 */
    long  nNodeListArrDims;    /* 创建的节点连表数组的维数,也是所有的逻辑分图个数
                                   未被投入的逻辑分图也会被创建 */
    long  nAllowMaxTaskCount;  /*逻辑图允许创建的最大任务数，
                                指若所有保护都投入时，可能创建的最大任务数  */

    BOOL   bGrpScanTaskCreateFlagArr
    [MAX_CREATE_RELAYFUNC_TASK_COUNT];    /* 逻辑图所有保护扫描任务被创建标志数组
                                                   TRUE表示被创建，否则表示未被创建， */
    BOOL   bGrpScanTaskScannedFlagArr
    [MAX_CREATE_RELAYFUNC_TASK_COUNT];    /* DQ: 2007-10-26
                                                   逻辑图所有保护扫描任务已经执行一次扫描的标志数组
                                                   TRUE表示已经被扫描过一次，否则表示未扫描*/
    unsigned  int   uiGrpScanDriveSamPeriodIntervalArr
    [MAX_CREATE_RELAYFUNC_TASK_COUNT];    /* 逻辑图所有保护扫描驱动的
                                                 的采样间隔周期数数组  */
    int      iTaskImortScanNodeCntArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];   /*保护扫描任务中端口引入扫描节点个数数组  2006-11-4日张云  */
    NODE **  ppTaskImportScanNodeArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];    /*保护扫描任务中端口引入扫描节点数组  2006-11-4日张云  */
    int      iTaskExportScanNodeCntArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];   /*保护扫描任务中端口引入扫描节点个数数组  2006-11-4日张云  */
    NODE **  ppTaskExportScanNodeArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];    /*保护扫描任务中端口引出扫描节点数组  2006-11-4日张云  */

    int      iTaskOptAOScanNodeCntArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];   /*保护扫描任务中光纵AO扫描节点个数数组  2006-11-11日张云  */
    NODE **  ppTaskOptAOScanNodeArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];    /*保护扫描任务中光纵AO扫描节点数组  2006-11-11日张云  */
}   LOGRP_ATTRIB_TYPE;

typedef   struct
{
    /*逻辑图任务每周期消耗的时间 2006-9-21 张云*/
    int   iTaskNo;/*该任务号  */
    BOOL  bTaskIsRun;/* 该任务是否已经被初次运行 */
    uint32_t  ulCurTimePerPeriod;/*当前最新周期运行时间　微秒*/
    uint32_t  ulMinTimePerPeriod;/*最小每周期运行时间　微秒*/
    uint32_t  ulMaxTimePerPeriod;/*最大每周期运行时间　微秒*/
    uint32_t  ulAverageTimePerPeriod;/*平均每周期运行时间　微秒*/

    BOOL   bBufIsFull;  /*buf是否已满标志  */
    uint64_t  ullTotalTimeAllPeriod;/*所有周期总的运行时间  */
    uint32_t  aulTimeBuf[CALC_TASK_TIME_PER_PERIOD_COUNT];/*周期时间缓冲  */
    uint32_t  ulCurSavePos;  /*当前存储位置  */
    uint64_t ullTotalTimeStat;  /* 统计周期总的运行时间 */
    uint32_t idlePercent;  /* 占用CPU百分比 */
}  LOGRP_COMSUME_TIME_TYPE;

typedef   struct
{
    BOOL   bCurHasFgCmd;  /*本任务本次有复归命令  */
}  TASK_OUT_CMD_STS_TYPE;  /*保护任务外部命令状态维护 目前只有复归命令，以后可以有其他  2006-12-21日 张云  */

/*用户开发的算法图元入口函数指针类型定义*/

typedef      EP_STATUS  (*  USER_INIT_FUNC_TYPE)(EP_ELEMENT *pelm);


/*获得扫描节点的输出IO的指针的函数指针类型定义*/

typedef      EP_ELEM_IO *  (*  GET_OUT_IO_FUNC_TYPE)
(uint16_t   unOutNum,NODE  * pScanNode);


/* 获得初始化节点的初始化扫描节点的函数指针类型定义 */
typedef      EP_STATUS  (*  INIT_SCAN_FUNC_TYPE)
(NODE * pElemInitNode,
 NODE  *pElemScanNode,
 LIST  *pGrpScanNodeList,
 BOOL    bPartGrpRunFlag);

/* 扫描图元函数 */
typedef void (*NODE_SCAN_FUNC)(NODE *pElemScanNode);

/* 扫描节点与扫描函数 */
typedef struct
{
    NODE *pCurScanNode;
    NODE_SCAN_FUNC pScanFunc;
} SCAN_UNIT;

/*初始化图元IO的即时值
   参数  pIO,待操作的IO指针
   返回值,无
*/

void   RE_InitElemIONowValue(EP_ELEM_IO  *pIO);





/******************保护引擎全局公共变量申明******************/

/* 逻辑图通过EP_Restart_Lgc函数方式重启动的标志 */

extern  BOOL   bLogrpIsRestarted_g;

/*  整个逻辑图扫描属性全局变量 */
extern  LOGRP_ATTRIB_TYPE   LogrpAttrib_g;


/*  所有保护任务创建成功标志 */

extern  BOOL   bAllRelayTaskCreateSuccess_g;


/*************************供测试用的算法元件表信息**************************************/
extern  EP_EXT_ELEM_MAP   *    SuanfaElemMapArrayAddr_g;
extern  uint32_t    nSuanfaElemMapCount_g;
extern  EP_DEBUG_PART_FUNC_TYPE     pSuanfaDebugEntryFunc_g;



/******************逻辑图扫描任务的任务序号数组**********************************/

extern  unsigned  long   ulLastRegisteSamFuncCount_g;/*上次启动逻辑图注册过的采样驱动函数个数*/

/* 保护任务扫描计数器，供看门狗检测用 */
extern  unsigned  long  RE_ulGrpScanTaskScanCounterArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 保存上次启动逻辑图注册过的采样函数信息数组 */
extern  SAM_REGISTER_FUNC_INFO_TYPE   LastRegisterSamFuncInfoArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务首次被释放信号量标志  */
extern  BOOL   RE_RelayTaskDriveSemFirstFreeFlagArr_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务首次被释放信号量时的AI计数器值  */
extern  uint32_t   RE_RelayTaskDriveSemFirstFreeAICounterArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/* 逻辑图任务进入硬件测试状态标志数组  */
extern BOOL   RE_bRelayTaskEnterHwTestModeFlagArr_g
[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 逻辑图任务定值整定消息计数  */
extern uint32_t RE_ulRelayTaskSetAtSetCnt_g;

/* 逻辑图任务是否含有自动整定图元  */
extern BOOL bulRelayTaskHasAutoSet_g;

/* 逻辑图任务定值整定图元计数
extern uint32_t RE_ulRelayTaskAutoSetCnt_g;*/


/******************逻辑图扫描任务的任务序号数组**********************************/
extern   unsigned  long   RE_aulLogrpScanTaskNo_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*    所有作为独立任务运行的逻辑分图当前正在执行扫描的状态标志数组。

      该标志数组中的所有标志只能由保护功能模块修改，但可由采样节拍关联函数访问，但不能修改。

      该标志数组对将所有分图在1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,保护功能模块只修改0号数组成员标志

      若该标志数组某成员对应的某逻辑分图任务正在扫描，则该标志为真，
      否则若该逻辑分图任务还未进行首次扫描时，该标志为假，。
**/

extern  BOOL     RE_abLogrpScaningFlag_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];



/*    所有访问作为独立任务运行的逻辑分图扫描状态标志的互斥二进制信号量数组。

      该信号量数组的每个信号量初始化创建为已满，可用。
      当实时数据任务和1个或多个逻辑图扫描任务访问某个扫描标志时，
      则需获得和释放相应信号量，以保证互斥操作

      该信号量数组对将所有分图作为1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,

***/

extern  SEM_ID    RE_aAccessLogrpScaningFlagSem_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    所有作为独立任务运行的逻辑分图的新1次扫描驱动同步计数器信号量数组。

      该信号量数组的每个信号量初始化创建为空，不可用。
      该信号量数组的每个信号量由采样节拍关联函数释放，由保护功能模块取该信号量

      该信号量数组对将所有分图作为1个任务运行的情况,也是有效的,
      此时实际只有0号数组成员有意义,保护功能模块只取走0号数组成员信号量

***/

extern  SEM_ID    RE_aInvokeLogrpNextScanSem_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    逻辑图扫描任务相关信息数组
      数组每个成员是与任务相关的逻辑图扫描任务即时信息

*/
extern  EP_CHART_MSG   RE_aGrpScanTaskMsg[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*   逻辑图扫描任务的当前扫描定值刷新标志数组
     数组每个成员是与任务相关的当前扫描定值需刷新标志
 */

extern  BOOL  RE_aGrpScanDingzhiRefreshFlag[MAX_CREATE_RELAYFUNC_TASK_COUNT];




/*    所有独立保护分图图元初始化节点连表的数组，
      其本身并不占多少空间，因为其是对指针操作
*/
extern  LIST   RE_aPartGrpInitNodeList[MAX_RELAY_FUNC_COUNT];



/*    所有独立保护分图图元扫描节点连表的数组，
      其本身并不占多少空间，因为其是对指针操作
*/
extern  LIST   RE_aPartGrpScanNodeList[MAX_RELAY_FUNC_COUNT];


/*    所有独立保护分图的属性数组，

*/
extern  PARTGRP_ATTRIB_TYPE   RE_aPartGrpAttribArr[MAX_RELAY_FUNC_COUNT];


/*  每个保护任务的每周期资源消耗统计数据数组 2006-9-21  张云*/
extern  LOGRP_COMSUME_TIME_TYPE   RE_aTaskPeriodTimeArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 每个保护任务的外部命令状态维护  2006-12-21日 张云 */
extern TASK_OUT_CMD_STS_TYPE   RE_aTaskOutCmdStsArr_g[MAX_CREATE_RELAYFUNC_TASK_COUNT];

/* 扫描节点入口函数数组 */
extern SCAN_UNIT *RE_arrScanUnit[MAX_RELAY_FUNC_COUNT];

/***********************读取逻辑图顺序化文件时使用的函数和宏********************/

/*从当前文件指针处读取ByteCount个保留字节从供装置解析的顺序化文件*/

BOOL    ReadReserveBytesFromResloveSeqFile(FILE  *fp,short  nByteCount);



/*从当前文件指针处当字符串长度允许大小为1个字节时，读取字符串，
包括1字节字符串长度
先读取1字节字符串长度，再读取字符串内容,
末尾添加结束符,pnRtStrLen返回字符串长度
*/

BOOL   ReadStringOfByteLenFromResloveSeqFile
( FILE  *fp,char  *   strRtRead,unsigned  long  *pnRtStrLen);


/*从当前文件指针处当字符串长度允许大小为2个字节时，读取字符串，包括2字节字符串长度*/
/*先读取2字节字符串长度，再读取字符串内容
末尾添加结束符,pnRtStrLen返回字符串长度*/

BOOL    ReadStringOfWordLenFromResloveSeqFile
(FILE  *fp,char  *   strRtRead,unsigned  long  *pnRtStrLen);


/*从当前文件指针处读取1字节无符号字符型，从文件中*/

BOOL    ReadUnsignedCharFromResloveSeqFile
(FILE  *fp,unsigned  char * pucRtRead);



/*从当前文件指针处读取2字节无符号短整型，从文件中，保存时是*/
/*先低字节，再高字节，读取时按PowerPC格式反序组合成short型*/

BOOL    ReadUnsignedShortFromResloveSeqFile
(FILE *fp,unsigned  short  *punRtRead);


/*从当前文件指针处读取4字节无符号长整型，到文件中，保存时先低字节，再高字节*/
/* 读取时按PowerPC格式反序组合成long型 */
BOOL   ReadUnsignedLongFromResloveSeqFile
(FILE  *fp,unsigned long  * pulRtRead);



/*从当前文件指针处按PowerPC格式读取保存为Intel Float格式的单精度数，
  从文件中，占4个字节。按反序组合成PowerPCfloat型*/

BOOL    ReadFloatFromResloveSeqFileInMotorolaType
(FILE  *fp,float * pfRtRead);




/*定义当前能解析的*/
/*逻辑图顺序化文件版本号为1.4*/

#define    LOGRP_ZHUANGZHI_RESOLVE_SEQ_FILE_VERSION   0X14



/*当在读取供装置解析逻辑图顺序化文件时，当字符长度只为1字节长时，允许读取的*/
/*最大字符串个数为128。*/

#define     MAX_STRLEN_OF_BYTE_AS_READ_FROM_RESLOVE_SEQFILE  0X80



/*当在读取供装置解析的逻辑图顺序化文件时，当字符长度只为2字节长时，允许读取的*/
/*最大字符串个数为1024。*/

#define     MAX_STRLEN_OF_WORD_AS_READ_FROM_RESLOVE_SEQFILE  0X0400

/* */





#endif



