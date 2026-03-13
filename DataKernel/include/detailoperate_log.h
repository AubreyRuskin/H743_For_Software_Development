

#ifndef OPERATE_LOG_H
#define OPERATE_LOG_H


#include "stdio_compat.h"
//#include "ioLib.h"

#include "sys_ioctl_compat.h"
#include "fcntl_compat.h"
#include "unistd_compat.h"

#include "errtest.h"
#include "edpbase.h"
#include "realdata.h"
#include "swcfg.h"
#include "EdpNetCfg.h"
#include "MMI_MasterCPU.h"
#include "swcfg.h"
#ifdef	__cplusplus
extern "C" {
#endif

#define OPR_EVT_DIR 	"/data/opr"
#define OPR_EVT_SON_DIR	"/data/opr/opr"



#define AE_MAX_REPORTS  500 /*总共的录波事件个数*/
#define MMI_ACT_LOG_VERSION 1 /*操作日志文件格式版本号*/
#define ALLOW_DATA_DISK_SPACE 0x200000

enum
{
    OTHEROPE=0,
    SETOPE,
    INSETOPE,
    CKSETOPE,
    LINKOPE,
    RELAYOPE,
    SYSOPE,
    DIOPE,
    CKOPE,
    EXPOPE,
    NETOPE,
    FILEOPE,
    SYSERROR,
    SYSEVENT,
};

enum
{
    UNIT32TYPE=0,
    INT32TYPE,
    BOOLTYPE,
    FLOATTYPE,
    COMPLEXTYPE,
    STRINGTYPE,
};

/* 条目值结构  */
typedef  union
{
    COMPLEX xVal;
    float fVal;
    uint32_t ulVal;
    int32_t lVal;
    BOOL bVal;
    char *str;
}   VALUE_TYPE_ITEM;


/* 每个条目的结构 */
typedef   struct
{
    uint8_t  aucItemName[129];
    uint8_t  ucAttrib;
    VALUE_TYPE_ITEM  oldVal;
    VALUE_TYPE_ITEM  newVal;

} OPR_LOG_ITEM_TYPE;

/*操作日志信息结构  */
typedef  struct
{
    uint8_t    ucOprType;
    uint8_t    aucOprSourceStr[64];
    uint32_t   ulOprNum;
    EP_DATE_TIME tmOprTime;
    uint8_t aucOprInf[128];
    uint32_t ulMdfItmNum;
    OPR_LOG_ITEM_TYPE  *pMdfItmBuf;
} OPR_LOG_TYPE;

EP_STATUS  OPR_Write(OPR_LOG_TYPE  *pNewOprLog);

void CPU_DetailOpr_Log_Init();


//删除定值区记日志
extern void  DeleteSetAreaToLog(uint8_t AreaCode, uint16_t usOpSrc);

/*切换定值区记日志ok*/
extern void ChangeSetAreaModifiesToLog(uint8_t preArea,uint8_t newArea, uint16_t usOpSrc);

/*修改内部定值记日志*/
void InsideSetModifiesToLog(SC_SET_ITEM *psetRd,int iNum);

/*修改运行定值区定值记日志*/
void SetModifiesToLog(int iFd, int iArea);

/*新建定值区记日志*/
void SetNewToLog( int iArea);

/*修改非运行定值区定值记日志*/
void NOWorkSetModifiesToLog(int iFd,int iFdBak, int iArea);

/*切换yb总选择记日志ok*/
extern void YBTotalToLog(uint16_t oldTotalYbStas,uint8_t newYbTotalStas, uint16_t usOpSrc);

/*保护功能投退记日志ok*/
extern void ProtStatsModifiesToLog(const SC_SUB_LGC_ITEM *ProItem,BOOL bOldProtStats,
                                   BOOL bNewProtStats, uint16_t usOpSrc);

/*网络设置记日志ok*/
void IPAdressModifiesToLog(EDP_NET_CFG_INFO OldIpStats,EDP_NET_CFG_INFO NewIpStats);

/*装置复归记日志ok*/
void DeviceResetToLog();

/*装置通道校准记日志ok*/
void ChannelAdjustToLog();

/*装置电能清零记日志ok*/
void PoClearAdjustToLog(int iChannel);


/*装置强制记日志ok*/
void DiForceToLog(DI_CH *pdich,int iCount);

/*软压板操作记日志*/
extern void SybChangeToLog(SC_LINK_ITEM *SybName, BOOL bOldStats, BOOL bStats, uint16_t usOpSrc);

/*测试/运行切换记日志*/
extern void  DebugRunSwitchToLog(uint32_t iOpType,BOOL bOldType, uint16_t usOpSrc);

/*远方就地切换记日志*/
void FarStatsChangeToLog(uint8_t iStats);

void CKSetModifiesToLog(SC_SET_ITEM *psetRd,int iNum);

extern void VI_New_MeaDoToLog(uint8_t PtNum,uint8_t OptNum,uint32_t OptPara,uint32_t usTqPara,
                              uint8_t *pIpAddr, uint16_t usOpSrc);


#ifdef	__cplusplus
}
#endif

#endif
