/********************************************************************************/
/*                                                                              */
/*      Copyright (c) 2002 SNAC(Guodian Nanjing Automation Co., Ltd.)           */
/*      All Rights Reserved.                                                    */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/* FILE NAME                                            VERSION                 */
/*                                                                              */
/*      edpcomtrade.h                                  EDP02-05-0.1                 */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      EP - edp02 for comtrade wave                            				*/
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      Caowg, SNAC                                                       */
/*                                                                              */
/* DATA STRUCTURES                                                              */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* FUNCTIONS                                                                    */
/*                                                                              */
/*      <TODO>                                                                  */
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*      Caowg      2007.07.04      Created first version 0.1.              */
/*                                                                              */
/********************************************************************************/


#ifndef EDPCOMTRADE_H
#define EDPCOMTRADE_H
#include "datetime.h"


#undef NO_TALKABOUT_PARA_INFO
#define MaxTripItem 64
#define MaxFaultInfo 64
#define RAM_DISK_MOUNT_POINT "/mrec"
#define EP_SYS_COMTRADE_DIR       RAM_DISK_MOUNT_POINT "/COMTRADE"


typedef struct XML_FAULT_INFO_STRUCT
{
//   char FaultName[64];
//   char FaultVal[64];
    char *FaultName;
    char *FaultVal;
    XML_FAULT_INFO_STRUCT()
    {
        FaultName =NULL;
        FaultVal =NULL;
    }
    ~XML_FAULT_INFO_STRUCT()
    {
        if(FaultName)
        {
            delete[] FaultName;
            FaultName =NULL;
        }
        if(FaultVal)
        {
            delete[] FaultVal;
            FaultVal =NULL;
        }
    }
} XML_FAULT_INFO;

typedef struct
{
    char RelTime[32];/*相对基准的偏差时间*/
    char TripName[64];/*事件名称*/
    char TripPhase[32];
    char TripVal[32];/*动作或返回*/
#ifndef NO_TALKABOUT_PARA_INFO
    uint16_t ParaNum;/*参数个数*/
    XML_FAULT_INFO FaultInfo[MaxFaultInfo];/*参数信息*/
#endif
} XML_TRIP_INFO;


typedef struct
{
    EP_DATE_TIME EDPBaseTime ;
    char basetime[128] ;
    char DatFileSize[64];
    uint16_t TripNum;
//   XML_TRIP_INFO TripInfo[MaxTripItem];

    XML_TRIP_INFO *TripInfo;
} XML_EVENT_REPORT;

BOOL Wave_Syn_Init();
int recLoopFunction(int cpucode);
BOOL Has_Free_Space(char *pCheckDir,uint32_t uLimitSpace);
BOOL Has_Free_Space(char *pCheckDir,uint32_t uTotalSpace,uint32_t uLimitSpace);
BOOL Get_04_Rec_List(char *pProCPURecDir,uint16_t *pRecNum,FileList_Info *pRecFileList,int cpucode);
BOOL Get_05_Rec_List(char *pHMICPURecDir,uint16_t *pRecNum,FileList_Info *pRecFileList,int cpucode);
BOOL Compare_Record_List(char *pHMICPURecDir,FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uProRecNum,uint16_t uHMIRecNum,int cpucode);
FileList_Info * Find_Oldest_HMI_Rec(char *pHMICPURecDir,FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uProRecNum,uint16_t uHMIRecNum);
void Del_HMI_Record_List(char *pHMICPURecDir,FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uProRecNum,uint16_t uHMIRecNum,int cpucode);
BOOL Is_New_Record(FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uHMIRecNum);
uint16_t Need_HMI_Space(FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uProRecNum,uint16_t uHMIRecNum,uint32_t *pNeedSpace,int cpucode);
BOOL Find_EventReport_By_Record(FileList_Info *pRecFileList,char *pEvt04Dir,char *pEvt05Dir,BOOL bNeedCreatHdr,int cpucode);
void Del_All_HMI_Record_List(char *pHMICPURecDir,FileList_Info *pProRecFileList,FileList_Info *pHMIRecFileList,uint16_t uProRecNum,uint16_t uHMIRecNum,int cpucode);
EP_STATUS FlushEvent(char *pEventFileName,uint16_t RecordNo,int cpucode);
void FormatEvent(uint8_t *buf,EP_DATE_TIME *basetime,uint16_t EventNo,int cpucode);
BOOL Transfer_Record_To_HMI(char *pProCPURecDir,char *pHMICPURecDir,FileList_Info *pProRecFileList,int cpucode);
int GetData_B(uint8_t *pcStr,uint8_t lIndex,uint8_t *m_pcItemStr,uint8_t char_B,uint8_t char_E);

BOOL CMT_Initialize();
BOOL CMT_Create_File(char *pHMICPURecDir,FileList_Info *pProRecList,int cpucode);
EP_STATUS Create_CMT_Head_File(char *pFileName,int cpucode,BOOL bCreat91HDR=FALSE);
BOOL Create_XML_Element(char *pElementName,char *pElementVal,char *pXMLElement);
BOOL Fill_XML_Trip(char *pTripTime,char *pTripName,char *pTripPhase,char *pTripVal,char *pXMLTrip);
BOOL Fill_XML_FaultInfo(char *pTripName,char *pTripVal,char *pXMLFaultInfo);
void Temp_File_Name(char *temp_filename, char  *strFile,char insChar );
void Temp_File_Name(char *temp_filename, char  *strFile);
BOOL utf8_str_copy_to_du(char *pRetStr, char *pSrcStr);
EP_STATUS Rename_Temp_Files(FileList_Info *pProRecList,int cpucode);
void Delete_Temp_Files(int cpucode);
void rcd_made_signal(uint8_t state,char  *pRecfile,int cpucode);
extern "C"
{
    bool comtrade_use_new_dir();
}
extern XML_EVENT_REPORT EventReport[4];
#endif
