
#include "detailoperate_log.h"
#include "filetool.h"
#include <dirent_compat.h>
#include <sys_stat_compat.h>
#include "edpbase.h"
#include "measure.h"
#include "swcfg.h"
#include "edp_asst.h"
#include "logLib.h"
#include "vxworks_io_compat.h"
#include "taskLib.h"

#define MAX_OPER_MSG_NUM 256 /* 最大操作日志消息条数 */
#define MAX_MSG_LEN 512 /* 消息最大长度 */

static uint32_t unCuroprRptSN = 0 ;/*当前操作报告号*/
static uint8_t oprlogDir = 1; /*添加的哪个文件夹*/
MSG_Q_ID OperMessage;
TASK_ID nOperTaskID_g;

/* global functions */

/***********************************************************************
* GetDataDiskLeftSize - 获取Data盘剩余空间大小
*
* RETURNS: OK, or ERROR
*
*/
STATUS GetDataDiskLeftSize(
    int *piSize		/* 剩余空间大小 */
);

//获取枚举项条目字串
//参数：	pcStr,枚举项字串
//			lIndex,字串序号（0开始）
//			pcItemStr,条目字串存储指针
//返回值：	条目字串长度
int GetEnumItem(char *pcStr,int32_t lIndex,char *pcItemStr)
{
    char ch;
    char *startptr=pcItemStr;

    while(lIndex>0)
    {
        ch=*pcStr++;
        if(ch==';')
            lIndex--;
        else if(ch=='\0')
        {
            *pcItemStr='\0';
            return 0;
        }
    }

    while(1)
    {
        ch=*pcStr++;
        if(ch=='\0'||ch==';')
            break;
        *pcItemStr++=ch;
    }

    *pcItemStr='\0';
    return pcItemStr-startptr;
}

void False_Handle(OPR_LOG_TYPE  *pNewOprLog)
{
    int i;
    if(pNewOprLog->ulMdfItmNum>0)
    {
        for(i = 0; i<pNewOprLog->ulMdfItmNum; i++)
        {
            if(pNewOprLog->pMdfItmBuf[i].ucAttrib == STRINGTYPE)
            {
                if(pNewOprLog->pMdfItmBuf[i].oldVal.str)
                    free(pNewOprLog->pMdfItmBuf[i].oldVal.str);
                if(pNewOprLog->pMdfItmBuf[i].newVal.str)
                    free(pNewOprLog->pMdfItmBuf[i].newVal.str);
            }
        }
        free(pNewOprLog->pMdfItmBuf);
    }
}

void Opre_Wr_File()
{
    int iErrorCount = 0;
    OPR_LOG_TYPE  pNewOprLog;
    int numRecv = 0;
    while(1)
    {
        numRecv =msgQReceive(OperMessage, (char *)&pNewOprLog, sizeof(OPR_LOG_TYPE), WAIT_FOREVER);
        if(numRecv!=-1)
            LOG_Dbg_Msg("the message size is %d",numRecv,0,0,0,0,0);
        if(numRecv == sizeof(OPR_LOG_TYPE))
        {
            logMsg("Log Operation Message coming!\n", 0, 0, 0, 0, 0, 0);
            if(OPR_Write(&pNewOprLog)!=EP_SUCCESS)
            {
                iErrorCount++;
                LOG_Dbg_Msg("记录详细操作日志出错!",0,0,0,0,0,0);
                if(iErrorCount%100==1)
                    LOG_Write(LOG_KERNEL, "记录详细操作日志出错!\n", NULL);

            }
        }

    }
}

int GetErrFolder(int iFolder)
{
    return OPR_FOLDER+iFolder;
}


void CPU_DetailOpr_Log_Init()
{
    char array[33] = {0};
    char diropt[32] = {0};
    uint32_t tempValue = 0;
    DIR *pDir = NULL;
    struct dirent *pEnt = NULL;
    STATUS vxsts;
    int i;
    /*建立/data/opr目录*/


    for(i =0; i<6; i++)
    {
        if(i ==0)
            sprintf(diropt,"%s",OPR_EVT_DIR);
        else
            sprintf(diropt,"%s%d",OPR_EVT_SON_DIR,i);

        if (!FT_Is_Dir(diropt))
        {
            vxsts=mkdir(diropt);
            if(vxsts!=OK)
            {
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "错误码:%02d\n",
                               GetErrFolder(i),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "Error code:%02d\n",
                               GetErrFolder(i),0);
                }
                LOG_Write(LOG_KERNEL,"操作日志目录创建失败!!\n",NULL);
                break;
            }
        }
        else
        {
            if(i==0)
                continue;
            pDir = opendir(diropt);
            if (pDir == NULL)
            {
                LOG_Dbg_Msg("file %s open failed !!!!!!",(int)diropt,0,0,0,0,0);
                continue;
            }
            while ((pEnt=readdir(pDir))!=NULL)
            {
                if(!('.' ==pEnt->d_name[0] || strlen(pEnt->d_name)!=31))
                {
                    memcpy(array,&pEnt->d_name[0],8);
                    tempValue = strtoul(array, NULL, 16);
                    if(tempValue>unCuroprRptSN)
                        unCuroprRptSN =tempValue;
                }
            }
            closedir(pDir);
        }
    }




    OperMessage = msgQCreate(MAX_OPER_MSG_NUM,	  /* max messages that can be queued */
                             MAX_MSG_LEN, 	/* max bytes in a message */
                             MSG_Q_FIFO 	  /* message queue options */
                            );
    if (OperMessage == NULL)
        LOG_Dbg_Msg("message queue creat failed./n", 0, 0, 0, 0, 0, 0);
    else
        LOG_Dbg_Msg("message queue creat sucess.\n", 0, 0, 0, 0, 0, 0);

    nOperTaskID_g = taskSpawn("tOperLog",
                              TSK_PRI_OPRE_LOG,
                              VX_FP_TASK|VX_DEALLOC_STACK,
                              10000,
                              (FUNCPTR)Opre_Wr_File,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    AddTaskToList(nOperTaskID_g, TRUE,
                  "看门狗复位:因操作记录任务异常或退出,看门狗复位CPU.\n", TRUE);
}





BOOL CheckEvtDir()
{
    /*检测日志目录*/
    char array[128]= {0};
    char diropt[33]= {0};
    uint16_t tempValue1 = 0;
    DIR  *pDir=NULL;
    struct dirent *pEnt=NULL;

    int nDataDiskLeftSize;
    STATUS vxsts=FALSE;


    tempValue1 = unCuroprRptSN%(AE_MAX_REPORTS);
    if(tempValue1==0)
    {
        oprlogDir = 5;
    }
    else if(tempValue1<=100)
    {
        oprlogDir = 1;
    }
    else if(tempValue1<=200)
    {
        oprlogDir = 2;
    }
    else if(tempValue1<=300)
    {
        oprlogDir = 3;
    }
    else if(tempValue1<=400)
    {
        oprlogDir = 4;
    }
    else
    {
        oprlogDir = 5;
    }

    sprintf(diropt,"%s%d",OPR_EVT_SON_DIR,oprlogDir);

    if (!FT_Is_Dir(diropt))
        return FALSE;
    if(unCuroprRptSN > (AE_MAX_REPORTS))
    {
        if((unCuroprRptSN%100)==1)
        {
            pDir = opendir(diropt);
            if (pDir == NULL)
            {
                LOG_Dbg_Msg("file %s open failed !!!!!!",(int)diropt,0,0,0,0,0);
                return FALSE;
            }
            while ((pEnt=readdir(pDir))!=NULL)
            {
                if(pEnt->d_name[0]!='.')
                {
                    sprintf(array,"%s/%s",diropt,pEnt->d_name);
                    remove(array);
                }
            }
            closedir(pDir);
        }
    }

    vxsts=GetDataDiskLeftSize(&nDataDiskLeftSize);
    if(vxsts == ERROR)
    {
        if(ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "ERROR, Get  DATA Disk free space failure!\n", NULL);
        else if(ENG_MODE ==0)
            LOG_Write(LOG_KERNEL, "得到空闲空间失败!\n", NULL);
        return FALSE;
    }
    LOG_Dbg_Msg("the data size is %d",nDataDiskLeftSize,0,0,0,0,0);
    if(nDataDiskLeftSize<ALLOW_DATA_DISK_SPACE)
    {
        LOG_Dbg_Msg("remain size is so little!!\n",0,0,0,0,0,0);
        if(unCuroprRptSN<=500)
        {
            sprintf(diropt,"%s%d",OPR_EVT_SON_DIR,1);
        }
        pDir = opendir(diropt);
        if (pDir == NULL)
        {
            LOG_Dbg_Msg("file %s open failed !!!!!!",(int)diropt,0,0,0,0,0);
            return FALSE;
        }
        while ((pEnt=readdir(pDir))!=NULL)
        {
            if(!('.' ==pEnt->d_name[0] || strlen(pEnt->d_name)!=31))
            {
                sprintf(array,"%s/%s",diropt,pEnt->d_name);
                remove(pEnt->d_name);
            }
        }
        closedir(pDir);
    }
    return TRUE;
}

EP_STATUS  OPR_Write(OPR_LOG_TYPE  *pNewOprLog)
{
    int iFd=ERROR;
    int auclength = 0;
    int Oprnumtemp =0;
    BOOL bFileIsOk=TRUE;
    DIR  *pDir=NULL;
    struct dirent *pEnt=NULL;
    uint32_t Oprnum = 0;
    uint8_t ucType = 0;
    char diropt[33]= {0};
    char aucBuf[64]= {0};
    char aucBuftemp[1024]= {0};
    EP_STATUS retcode =EP_ERROR;
    int i;

    /*记录号为最大时,删除目录下所有文件*/
    if(unCuroprRptSN==0xFFFFFFFF)
    {
        unCuroprRptSN=0;

        for(i=1; i<6; i++)
        {
            sprintf(diropt,"%s%d",OPR_EVT_SON_DIR,i);
            pDir = opendir(diropt);
            if (pDir == NULL)
            {
                LOG_Dbg_Msg("file %s open failed !!!!!!",(int)diropt,0,0,0,0,0);
                continue;
            }

            while ((pEnt=readdir(pDir))!=NULL)
            {
                if(pEnt->d_name[0]!='.')
                {
                    sprintf(aucBuf,"%s/%s",diropt,pEnt->d_name);
                    remove(aucBuf);
                }
            }
            closedir(pDir);
        }

    }
    else
        unCuroprRptSN++;
    if(CheckEvtDir()==FALSE)
    {
        False_Handle(pNewOprLog);
        return EP_ERROR;
    }

    sprintf(aucBuf,"%s%d",(char*)OPR_EVT_SON_DIR,oprlogDir);

    sprintf(aucBuf,"%s/%08x%02x%04d%02d%02d%02d%02d%02d%03d.opr",
            aucBuf,
            (unsigned int)unCuroprRptSN,
            pNewOprLog ->ucOprType,
            pNewOprLog->tmOprTime.unYear,
            pNewOprLog->tmOprTime.ucMonth,
            pNewOprLog->tmOprTime.ucDate,
            pNewOprLog->tmOprTime.ucHour,
            pNewOprLog->tmOprTime.ucMinute,
            pNewOprLog->tmOprTime.ucSec,
            pNewOprLog->tmOprTime.unMSEL);
    LOG_Dbg_Msg("the file name is %s",(int)aucBuf,0,0,0,0,0);
    iFd=creat(aucBuf, O_RDWR);
    if(iFd==ERROR)
    {
        False_Handle(pNewOprLog);
        LOG_Dbg_Msg("创建详细操作日志文件失败\n",0,0,0,0,0,0);
        return EP_ERROR ;
    }
    switch(pNewOprLog->ucOprType)
    {
        case OTHEROPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"其他操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Other operations \n");
            break;
        case SETOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"一般定值操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"General setting operating \n");
            break;
        case INSETOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"内部定值操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Internal setting operating \n");
            break;
        case CKSETOPE:
            /* 保证界面和操作日志一致性,记录操作日志时把测控定值改为参数定值 */
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"参数定值操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Parameter and control setting operating \n");
            break;
        case LINKOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"压板操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Enable operating \n");
            break;
        case RELAYOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"保护功能投退操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Protection function enable/disable operating \n");
            break;
        case SYSOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"系统操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"System operating \n");
            break;
        case DIOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"开入强制操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"DI force operating \n");
            break;
        case CKOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"测控功能操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Measurement and control function operating \n");
            break;
        case EXPOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"扩展功能操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Extended function operating \n");
            break;
        case NETOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"网络设置操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Network setting operating \n");
            break;
        case FILEOPE:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"重要文件更新操作\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Main file updating \n");
            break;
        case SYSERROR:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"系统严重错误\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Serious system error \n");
            break;
        case SYSEVENT:
            if(ENG_MODE==0)
                sprintf(aucBuftemp,"系统重要事件\n");
            else if(ENG_MODE==1)
                sprintf(aucBuftemp,"Main system event \n");
            break;
        default:
            break;
    }
    Oprnum = pNewOprLog->ulMdfItmNum;
    if(Oprnum!=0)
        sprintf(aucBuftemp,"%s%s\n%08x\n%04d%02d%02d%02d%02d%02d%03d\n%s\n%ld\n",
                aucBuftemp,
                pNewOprLog->aucOprSourceStr,
                (unsigned int)unCuroprRptSN,
                pNewOprLog->tmOprTime.unYear,
                pNewOprLog->tmOprTime.ucMonth,
                pNewOprLog->tmOprTime.ucDate,
                pNewOprLog->tmOprTime.ucHour,
                pNewOprLog->tmOprTime.ucMinute,
                pNewOprLog->tmOprTime.ucSec,
                pNewOprLog->tmOprTime.unMSEL,
                pNewOprLog->aucOprInf,
                Oprnum);
    else
        sprintf(aucBuftemp,"%s%s\n%08x\n%04d%02d%02d%02d%02d%02d%03d\n%s\n",
                aucBuftemp,
                pNewOprLog->aucOprSourceStr,
                (unsigned int)unCuroprRptSN,
                pNewOprLog->tmOprTime.unYear,
                pNewOprLog->tmOprTime.ucMonth,
                pNewOprLog->tmOprTime.ucDate,
                pNewOprLog->tmOprTime.ucHour,
                pNewOprLog->tmOprTime.ucMinute,
                pNewOprLog->tmOprTime.ucSec,
                pNewOprLog->tmOprTime.unMSEL,
                pNewOprLog->aucOprInf);
    auclength = strlen(aucBuftemp);
    if(write(iFd,aucBuftemp,auclength)!=auclength)
    {
        LOG_Dbg_Msg("write1 error!!!\n",0,0,0,0,0,0);
        bFileIsOk=FALSE ;
        goto exit;
    }
    for(Oprnumtemp = 0; Oprnumtemp<Oprnum; Oprnumtemp++)
    {
        ucType = pNewOprLog->pMdfItmBuf[Oprnumtemp].ucAttrib;
        switch(ucType)
        {
            case UNIT32TYPE:
            {
                sprintf(aucBuftemp,"%s\n%08u\n%08u\n",
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName,
                        (unsigned int)pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.ulVal,
                        (unsigned int)pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.ulVal);
                break;
            }
            case INT32TYPE:
            {
                sprintf(aucBuftemp,"%s\n%08ld\n%08ld\n",
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.lVal,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.lVal);
                break;
            }
            case BOOLTYPE:
            {
                if(pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.bVal)
                    sprintf(aucBuftemp,"%s\nTURE\n",pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName);
                else
                    sprintf(aucBuftemp,"%s\nFALSE\n",pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName);
                if(pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.bVal)
                    sprintf(aucBuftemp,"%sTURE\n",aucBuftemp);
                else
                    sprintf(aucBuftemp,"%sFALSE\n",aucBuftemp);
                break;
            }
            case FLOATTYPE:
            {
                sprintf(aucBuftemp,"%s\n%f\n%f\n",
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.fVal,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.fVal);
                break;
            }
            case COMPLEXTYPE:
            {
                sprintf(aucBuftemp,"%s\n%f+%fi\n%f+%fi\n",
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName,
                        REAL(pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.xVal),
                        IMAGE(pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.xVal),
                        REAL(pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.xVal),
                        IMAGE(pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.xVal));
                break;
            }
            case STRINGTYPE:
            {
                sprintf(aucBuftemp,"%s\n%s\n%s\n",
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].aucItemName,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].oldVal.str,
                        pNewOprLog->pMdfItmBuf[Oprnumtemp].newVal.str);
                break;
            }
        }
        auclength = strlen(aucBuftemp);
        if(write(iFd,aucBuftemp,auclength)!=auclength)
        {
            LOG_Dbg_Msg("write2 error!!!\n",0,0,0,0,0,0);
            bFileIsOk=FALSE ;
            goto exit;
        }
    }
    retcode = EP_SUCCESS;
exit:
    if(iFd!=ERROR)
        close(iFd);
    if(bFileIsOk==FALSE)
    {
        /*文件不合法，则删除*/
        remove(aucBuf);
        if(unCuroprRptSN==0)
            unCuroprRptSN=0xFFFFFFFF;
        else
            unCuroprRptSN--;
    }
    False_Handle(pNewOprLog);
    return retcode ;
}


void    DeleteSetAreaToLog(uint8_t AreaCode, uint16_t usOpSrc)
{
    /*删除定值区记日志ok*/
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"删除%d号定值区(操作源类型%d)!",AreaCode, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Delete No. %d setting group(the operational source type is %d)!",AreaCode, usOpSrc);
    pEventInfo.ulMdfItmNum=0;

    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT, MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void ChangeSetAreaModifiesToLog(uint8_t preArea,uint8_t newArea, uint16_t usOpSrc)
{
    /*切换定值区记日志ok*/

    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"从%d号定值区切换到%d号定值区(操作源类型%d)!",preArea,newArea, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Switch from No. %d setting group to No. %d(the operational source type is %d)!",preArea,newArea,usOpSrc);
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
    else
    {
        LOG_Dbg_Msg("OK!\n", retcode, 0,0,0,0,0);
    }
}

void InsideSetModifiesToLog(SC_SET_ITEM *psetRd,int iNum)
{
    /*修改内部定值记日志*/
    BOOL bIfSendMsg = TRUE;
    int retcode;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;

    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    uint8_t ChangeCount = 0;
    int ChangNum=0;
    char *pStrLengthOld;
    char *pStrLengthNew;


    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=INSETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"修改内部定值区!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Alter internal setting group!");



    for (psetOrg=(SC_Get_Set_Pg_Attr(0))->pset, pset=psetRd; pset<psetRd+iNum; psetOrg++, pset++)
    {
        if(psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            ChangeCount++;
    }
    pEventInfo.ulMdfItmNum=ChangeCount;
    if(ChangeCount==0)
        return;
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(ChangeCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("修改内部定值记日志申请内存出错!\n",0,0,0,0,0,0);
        bIfSendMsg = FALSE;
        return;
    }
    for (psetOrg=(SC_Get_Set_Pg_Attr(0))->pset, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        if(psetOrg->valNow.ulVal!=pset->valNow.ulVal)
        {
            if(IS_INT32_SET(pset->ucUnit))
            {
                sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=INT32TYPE;
                pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=psetOrg->valNow.lVal;
                pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=pset->valNow.lVal;
            }
            else if(IS_UINT32_SET(pset->ucUnit)&&(pset->ucUnit!=0x68)&&(pset->ucUnit!=0x00))
            {
                sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=UNIT32TYPE;
                pEventInfo.pMdfItmBuf[ChangNum].oldVal.ulVal=psetOrg->valNow.ulVal;
                pEventInfo.pMdfItmBuf[ChangNum].newVal.ulVal=pset->valNow.ulVal;
            }
            else if(pset->ucUnit==0x00)
            {
                char *tempold;
                char *tempnew;
                sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                tempold = (char *)malloc(TEMP_INFO_MAX_LEN);
                if(tempold==NULL)
                {
                    LOG_Dbg_Msg("修改内部定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                    bIfSendMsg = FALSE;
                    break;
                }
                GetEnumItem(psetOrg->pucUnitName,psetOrg->valNow.ulVal,tempold);
                pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=tempold;

                tempnew = (char *)malloc(TEMP_INFO_MAX_LEN);
                if(tempnew==NULL)
                {
                    LOG_Dbg_Msg("修改内部定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                    bIfSendMsg = FALSE;
                    break;
                }
                GetEnumItem(psetOrg->pucUnitName,pset->valNow.ulVal,tempnew);
                pEventInfo.pMdfItmBuf[ChangNum].newVal.str=tempnew;

            }
            else if(pset->ucUnit==0x68)
            {
                sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;
                pStrLengthOld = (char *)malloc(psetOrg->valNow.ulVal+1);
                if(pStrLengthOld==NULL)
                {
                    LOG_Dbg_Msg("修改内部定值记日志申请内存出错!\n",0,0,0,0,0,0);
                    bIfSendMsg = FALSE;
                    break;
                }
                strncpy(pStrLengthOld,psetOrg->aucNowStr,psetOrg->valNow.ulVal);
                pStrLengthOld[psetOrg->valNow.ulVal]='\0';
                pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=pStrLengthOld;

                pStrLengthNew= (char *)malloc(pset->valNow.ulVal+1);
                if(pStrLengthNew==NULL)
                {
                    LOG_Dbg_Msg("修改内部定值记日志申请内存出错!\n",0,0,0,0,0,0);
                    bIfSendMsg = FALSE;
                    break;
                }
                strncpy(pStrLengthNew,pset->aucNowStr,pset->valNow.ulVal);
                pStrLengthNew[pset->valNow.ulVal]='\0';
                pEventInfo.pMdfItmBuf[ChangNum].newVal.str=pStrLengthNew;
            }
            else
            {
                sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=FLOATTYPE;
                pEventInfo.pMdfItmBuf[ChangNum].oldVal.fVal=psetOrg->valNow.fVal;
                pEventInfo.pMdfItmBuf[ChangNum].newVal.fVal=pset->valNow.fVal;
            }
            ChangNum++;
        }
    }
    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}


void CKSetModifiesToLog(SC_SET_ITEM *psetRd,int iNum)
{
    /*修改测控定值记日志*/
    BOOL bIfSendMsg = TRUE;
    int retcode;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;

    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    uint8_t ChangeCount = 0;
    int ChangNum=0;
    char *pStrLengthOld;
    char *pStrLengthNew;

    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=CKSETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    /* 保证界面和操作日志一致性,记录操作日志时把测控定值改为参数定值 */
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"修改参数定值区!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Alter parameter and control setting group!");
    psetOrg=SC_Get_CK_Set();

    for ( pset=psetRd; pset<psetRd+iNum; psetOrg++, pset++)
    {
        if(psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            ChangeCount++;
    }
    pEventInfo.ulMdfItmNum=ChangeCount;
    if(ChangeCount==0)
        return;
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(ChangeCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("修改测控定值记日志申请内存出错!\n",0,0,0,0,0,0);
        bIfSendMsg=FALSE;
        return ;
    }
    for (psetOrg=SC_Get_CK_Set(), pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        if(!(psetOrg->bAutoSet))
            if(psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            {
                if(IS_INT32_SET(pset->ucUnit))
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=INT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=psetOrg->valNow.lVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=pset->valNow.lVal;
                }
                else if(IS_UINT32_SET(pset->ucUnit)&&(pset->ucUnit!=0x68)&&(pset->ucUnit!=0x00))
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=UNIT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.ulVal=psetOrg->valNow.ulVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.ulVal=pset->valNow.ulVal;
                }
                else if(pset->ucUnit==0x00)
                {
                    char *tempold;
                    char *tempnew;
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                    tempold = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempold==NULL)
                    {
                        LOG_Dbg_Msg("修改ck定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(psetOrg->pucUnitName,psetOrg->valNow.ulVal,tempold);
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=tempold;

                    tempnew = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempnew==NULL)
                    {
                        LOG_Dbg_Msg("修改ck定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(psetOrg->pucUnitName,pset->valNow.ulVal,tempnew);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=tempnew;

                }
                else if(pset->ucUnit==0x68)
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;
                    pStrLengthOld = (char *)malloc(psetOrg->valNow.ulVal+1);
                    if(pStrLengthOld==NULL)
                    {
                        LOG_Dbg_Msg("修改ck定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    strncpy(pStrLengthOld,psetOrg->aucNowStr,psetOrg->valNow.ulVal);
                    pStrLengthOld[psetOrg->valNow.ulVal]='\0';
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=pStrLengthOld;

                    pStrLengthNew= (char *)malloc(pset->valNow.ulVal+1);
                    if(pStrLengthOld==NULL)
                    {
                        LOG_Dbg_Msg("修改ck定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    strncpy(pStrLengthNew,pset->aucNowStr,pset->valNow.ulVal);
                    pStrLengthNew[pset->valNow.ulVal]='\0';
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=pStrLengthNew;
                }
                else
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=FLOATTYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.fVal=psetOrg->valNow.fVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.fVal=pset->valNow.fVal;
                }
                ChangNum++;
            }
    }
    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}



void SetModifiesToLog(int iFd, int iArea)
{
    /*修改定值记日志*/
    BOOL bIfSendMsg = TRUE;
    int retcode;
    const SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    uint8_t aucBuf[40];
    int iIdx;
    int iSet;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    uint8_t ChangeCount = 0;
    int ChangNum=0;
    int StrLength = 0;
    char *pStrLengthNew;
    char *pStrLengthOld;
    int i;

    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"修改%d号定值区!",iArea);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Alter No. %d setting group!",iArea);
    lseek(iFd, 10, SEEK_SET);

    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        i=read(iFd, aucBuf, 4);
        assert(i==4);

        psetpg=SC_Get_Set_Pg_Attr(0);
        psetpg+=aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            assert(i==9);

            pset=psetpg->pset+U8_TO_U16(aucBuf[3], aucBuf[2]);
            if(pset->valNow.ulVal!=BYTES_TO_U32(aucBuf+5))
                ChangeCount++;
        }
    }
    pEventInfo.ulMdfItmNum=ChangeCount;
    if(ChangeCount==0)
        return;
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(ChangeCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
        return ;
    }
    lseek(iFd, 10, SEEK_SET);
    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {

        i=read(iFd, aucBuf, 4);
        assert(i==4);

        psetpg=SC_Get_Set_Pg_Attr(0)+aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            assert(i==9);

            pset=psetpg->pset+U8_TO_U16(aucBuf[3], aucBuf[2]);
            if(pset->valNow.ulVal!=BYTES_TO_U32(aucBuf+5))
            {
                if(IS_INT32_SET(pset->ucUnit))
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=INT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=pset->valNow.lVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }
                else if(IS_UINT32_SET(pset->ucUnit)&&(pset->ucUnit!=0x68)&&(pset->ucUnit!=0x00))
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=UNIT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=pset->valNow.lVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }

                else if(pset->ucUnit==0x00)
                {
                    char *tempold;
                    char *tempnew;
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                    tempold = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempold==NULL)
                    {
                        LOG_Dbg_Msg("修改定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(pset->pucUnitName,pset->valNow.ulVal,tempold);
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=tempold;

                    tempnew = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempnew==NULL)
                    {
                        LOG_Dbg_Msg("修改定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(pset->pucUnitName,BYTES_TO_U32(aucBuf+5),tempnew);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=tempnew;

                }
                else if(pset->ucUnit==0x68)
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                    StrLength = BYTES_TO_U32(aucBuf+5);
                    i=read(iFd, aucBuf, BYTES_TO_U32(aucBuf+5));
                    aucBuf[(BYTES_TO_U32(aucBuf+5))]='\0';
                    pStrLengthOld = (char *)malloc(pset->valNow.ulVal+1);
                    if(pStrLengthOld==NULL)
                    {
                        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    strncpy(pStrLengthOld,pset->aucNowStr,pset->valNow.ulVal);
                    pStrLengthOld[pset->valNow.ulVal]='\0';
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=pStrLengthOld;

                    pStrLengthNew=(char *)malloc(StrLength+1);
                    if(pStrLengthNew==NULL)
                    {
                        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    sprintf(pStrLengthNew,aucBuf);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=pStrLengthNew;

                }
                else
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=FLOATTYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=pset->valNow.lVal;
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }
                ChangNum++;
            }

        }
        if(!bIfSendMsg)
            break;
    }
    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }

    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);

        return ;
    }
}

/*新建定值区记日志*/
void SetNewToLog( int iArea)
{
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;

    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    sprintf(pEventInfo.aucOprInf,"生成%d号定值区!",iArea);
    pEventInfo.ulMdfItmNum=1;   /* 只记录一条生成新定值区的日志 */
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(1,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("生成新定值区记日志申请内存出错!\n",0,0,0,0,0,0);
        return ;
    }
    sprintf(pEventInfo.pMdfItmBuf[0].aucItemName,"定值修改，原有%d区定值不存在，生成新的定值区。",iArea);
    pEventInfo.pMdfItmBuf[0].ucAttrib=STRINGTYPE;
    pEventInfo.pMdfItmBuf[0].oldVal.str="";
    pEventInfo.pMdfItmBuf[0].newVal.str="";

    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);

        return ;
    }
}

/* 修改非运行定值区记日志 */
void NOWorkSetModifiesToLog(int iFd,int iFdBak, int iArea)
{
    BOOL bIfSendMsg = TRUE;
    int retcode;
    const SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    uint8_t aucBuf[40], aucBufBak[40];
    int iIdx;
    int iSet;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    uint8_t ChangeCount = 0;
    int ChangNum=0;
    int StrLength = 0;
    int StrLengthBak = 0;
    char *pStrLengthNew;
    char *pStrLengthOld;
    int i;

    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    sprintf(pEventInfo.aucOprInf,"修改%d号定值区!",iArea);
    lseek(iFd, 10, SEEK_SET);
    lseek(iFdBak, 10, SEEK_SET);

    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        i=read(iFd, aucBuf, 4);
        assert(i==4);

        i=read(iFdBak, aucBufBak, 4);
        assert(i==4);

        psetpg=SC_Get_Set_Pg_Attr(0);
        psetpg+=aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            assert(i==9);

            i=read(iFdBak, aucBufBak, 9);
            assert(i==9);

            if(BYTES_TO_U32(aucBufBak+5)!=BYTES_TO_U32(aucBuf+5))
                ChangeCount++;
        }
    }
    pEventInfo.ulMdfItmNum=ChangeCount;
    if(ChangeCount==0)
        return;
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(ChangeCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
        return ;
    }
    lseek(iFd, 10, SEEK_SET);
    lseek(iFdBak, 10, SEEK_SET);
    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {

        i=read(iFd, aucBuf, 4);
        assert(i==4);

        i=read(iFdBak, aucBufBak, 4);
        assert(i==4);

        psetpg=SC_Get_Set_Pg_Attr(0)+aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            assert(i==9);

            i=read(iFdBak, aucBufBak, 9);
            assert(i==9);

            pset=psetpg->pset+U8_TO_U16(aucBuf[3], aucBuf[2]);
            if(BYTES_TO_U32(aucBufBak+5)!=BYTES_TO_U32(aucBuf+5))
            {
                if(IS_INT32_SET(aucBuf[4]))  /* 定值类型 */
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=INT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=BYTES_TO_U32(aucBufBak+5);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }
                else if(IS_UINT32_SET(aucBuf[4])&&(aucBuf[4]!=0x68)&&(aucBuf[4]!=0x00))  /* 定值类型 */
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=UNIT32TYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=BYTES_TO_U32(aucBufBak+5);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }

                else if(aucBuf[4]==0x00)  /* 定值类型 */
                {
                    char *tempold;
                    char *tempnew;
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                    tempold = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempold==NULL)
                    {
                        LOG_Dbg_Msg("修改定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(pset->pucUnitName,BYTES_TO_U32(aucBufBak+5),tempold);
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=tempold;

                    tempnew = (char *)malloc(TEMP_INFO_MAX_LEN);
                    if(tempnew==NULL)
                    {
                        LOG_Dbg_Msg("修改定值控制字记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    GetEnumItem(pset->pucUnitName,BYTES_TO_U32(aucBuf+5),tempnew);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=tempnew;

                }
                else if(aucBuf[4]==0x68)
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;

                    StrLengthBak = BYTES_TO_U32(aucBufBak+5);
                    i=read(iFdBak, aucBufBak, BYTES_TO_U32(aucBufBak+5));
                    aucBufBak[(BYTES_TO_U32(aucBufBak+5))]='\0';

                    StrLength = BYTES_TO_U32(aucBuf+5);
                    i=read(iFd, aucBuf, BYTES_TO_U32(aucBuf+5));
                    aucBuf[(BYTES_TO_U32(aucBuf+5))]='\0';

                    pStrLengthOld = (char *)malloc(StrLengthBak+1);
                    if(pStrLengthOld==NULL)
                    {
                        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    sprintf(pStrLengthOld,aucBufBak);
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=pStrLengthOld;

                    pStrLengthNew=(char *)malloc(StrLength+1);
                    if(pStrLengthNew==NULL)
                    {
                        LOG_Dbg_Msg("修改定值记日志申请内存出错!\n",0,0,0,0,0,0);
                        bIfSendMsg = FALSE;
                        break;
                    }
                    sprintf(pStrLengthNew,aucBuf);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.str=pStrLengthNew;

                }
                else
                {
                    sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,pset->aucName);
                    pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=FLOATTYPE;
                    pEventInfo.pMdfItmBuf[ChangNum].oldVal.lVal=BYTES_TO_U32(aucBufBak+5);
                    pEventInfo.pMdfItmBuf[ChangNum].newVal.lVal=BYTES_TO_U32(aucBuf+5);
                }
                ChangNum++;
            }

        }
        if(!bIfSendMsg)
            break;
    }
    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }

    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);

        return ;
    }
}

/* 压板操作日志记录 */
void SybChangeToLog(SC_LINK_ITEM *RybItem, BOOL bOldStas, BOOL bNewStats, uint16_t usOpSrc)
{
    /*软压板操作记日至*/
    int retcode;
    char aucBuf1[20];
    char aucBuf2[20];
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=LINKOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(bOldStas)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"投入");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Enable");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"退出");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Disable");
    }
    if(bNewStats)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"投入");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Enable");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"退出");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Disable");
    }
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf, "压板%s状态由%s切换到%s, 操作源类型%d!",
                RybItem->aucName, aucBuf1, aucBuf2, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf, "Switch enable state %s from %s to %s with the operational source type as %d!",
                RybItem->aucName, aucBuf1, aucBuf2, usOpSrc);

    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
    else
    {
        LOG_Dbg_Msg("OK!\n", retcode, 0,0,0,0,0);
    }
}

void YBTotalToLog(uint16_t oldTotalYbStas,uint8_t newYbTotalStas, uint16_t usOpSrc)
{
    /*切换yb总选择记日志ok*/
    int retcode;
    char aucBuf1[20]= {0};
    char aucBuf2[20]= {0};
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    switch(oldTotalYbStas)
    {
        case LINK_MODE_HW:
            if(ENG_MODE==0)
                sprintf(aucBuf1,"硬压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf1,"Hard enable mode");
            break;
        case LINK_MODE_SW:
            if(ENG_MODE==0)
                sprintf(aucBuf1,"软压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf1,"Soft enable mode");
            break;
        case LINK_MODE_AND:
            if(ENG_MODE==0)
                sprintf(aucBuf1,"软硬相与压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf1,"Hard and soft enable AND mode");
            break;
        case LINK_MODE_OR:
            if(ENG_MODE==0)
                sprintf(aucBuf1,"软硬相或压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf1,"Hard and soft enable OR mode");
            break;
        case LINK_MODE_CUS:
            if(ENG_MODE==0)
                sprintf(aucBuf1,"定制压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf1,"Customize enable mode");
            break;
    }
    switch(newYbTotalStas)
    {
        case LINK_MODE_HW:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"硬压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Hard enable mode");
            break;
        case LINK_MODE_SW:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"软压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Soft enable mode");
            break;
        case LINK_MODE_AND:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"软硬相与压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Hard and soft enable AND mode");
            break;
        case LINK_MODE_OR:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"软硬相或压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Hard and soft enable OR mode");
            break;
        case LINK_MODE_CUS:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"定制压板模式");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Customize enable mode");
            break;
    }

    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=LINKOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;

    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"总压板模式由%s切换到%s(操作源类型%d)!",aucBuf1,aucBuf2, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Switch general enable mode from %s to %s(the operational source type is %d)!"
            ,aucBuf1,aucBuf2, usOpSrc);
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void ProtStatsModifiesToLog(const SC_SUB_LGC_ITEM *ProItem,BOOL bOldProtStats,
                            BOOL bNewProtStats, uint16_t usOpSrc)
{
    /*保护功能投退记日志ok*/
    int retcode;
    char aucBuf1[20];
    char aucBuf2[20];
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=RELAYOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(bOldProtStats)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"投入");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Enable");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"退出");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Disable");
    }
    if(bNewProtStats)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"投入");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Enable");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"退出");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Disable");
    }
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"保护投退功能%s由%s切换到%s(操作源类型%d)!",
                ProItem->aucName,aucBuf1,aucBuf2, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Switch protection function enabling/disabling %s from %s to %s(the operational source type is %d)!"
                ,ProItem->aucName,aucBuf1,aucBuf2, usOpSrc);
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void IPAdressModifiesToLog(EDP_NET_CFG_INFO OldIpStats,EDP_NET_CFG_INFO NewIpStats)
{
    /*网络设置记日志ok*/
    BOOL bIfSendMsg=TRUE;
    int retcode;
    char *aucBuf1[6];
    char *aucBuf2[6];
    int ChangeCount = 0;
    int ChangNum=0;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    int i;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=NETOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"网络设置操作!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Set the Network!");

    for(i=0; i<NewIpStats.iValidNetNum; i++)
    {
        if(OldIpStats.NetInfArr[i].aucIpAddr != NewIpStats.NetInfArr[i].aucIpAddr)
            ChangeCount++;
    }
    pEventInfo.ulMdfItmNum=ChangeCount;
    if(ChangeCount==0)
        return;

    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(ChangeCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("网络设置记日志申请内存出错!\n",0,0,0,0,0,0);
        return ;
    }
    for(i=0; i<NewIpStats.iValidNetNum; i++)
    {
        if(OldIpStats.NetInfArr[i].aucIpAddr != NewIpStats.NetInfArr[i].aucIpAddr)
        {
            aucBuf1[i]=(char *)malloc(20);
            if(aucBuf1[i]==NULL)
            {
                LOG_Dbg_Msg("网络设置记日志申请内存出错!\n",0,0,0,0,0,0);
                bIfSendMsg = FALSE;
                break;
            }
            aucBuf2[i]=(char *)malloc(20);
            if(aucBuf2[i]==NULL)
            {
                LOG_Dbg_Msg("网络设置记日志申请内存出错!\n",0,0,0,0,0,0);
                bIfSendMsg = FALSE;
                break;
            }
            sprintf(pEventInfo.pMdfItmBuf[ChangNum].aucItemName,"ip%d",i);
            pEventInfo.pMdfItmBuf[ChangNum].ucAttrib=STRINGTYPE;
            sprintf(aucBuf1[i],"%d.%d.%d.%d",OldIpStats.NetInfArr[i].aucIpAddr[0],
                    OldIpStats.NetInfArr[i].aucIpAddr[1],
                    OldIpStats.NetInfArr[i].aucIpAddr[2],
                    OldIpStats.NetInfArr[i].aucIpAddr[3]);
            pEventInfo.pMdfItmBuf[ChangNum].oldVal.str=aucBuf1[i];
            sprintf(aucBuf2[i],"%d.%d.%d.%d",NewIpStats.NetInfArr[i].aucIpAddr[0],
                    NewIpStats.NetInfArr[i].aucIpAddr[1],
                    NewIpStats.NetInfArr[i].aucIpAddr[2],
                    NewIpStats.NetInfArr[i].aucIpAddr[3]);
            pEventInfo.pMdfItmBuf[ChangNum].newVal.str=aucBuf2[i];

            ChangNum++;

        }

    }

    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);

    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void DeviceResetToLog()
{
    /*装置复归记日志ok*/
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SYSOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"装置复归!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Reset the IED!");
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void ChannelAdjustToLog()
{
    /*装置通道校准记日志ok*/
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=CKOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"通道校准!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Calibrate the channel!");
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}

void PoClearAdjustToLog(int iChannel)
{
    /*装置电能清零记日志ok*/
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=CKOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(iChannel==0xff)
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"所有测量电能清零!");
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf,"All measuredenergy clearing!");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"通道%d测量电能清零!",iChannel);
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf,"Measuredenergy of channel %d clearing!",iChannel);
    }
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}


void DiForceToLog(DI_CH *pdich,int iCount)
{
    /*装置强制记日志ok*/
    int retcode;
    int i;
    char *pValue=NULL;
    BOOL bIfSendMsg=TRUE;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=DIOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"开入强制操作!");
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"DI force operating!");
    pEventInfo.ulMdfItmNum=iCount;
    pEventInfo.pMdfItmBuf=(OPR_LOG_ITEM_TYPE*)calloc(iCount,sizeof(OPR_LOG_ITEM_TYPE));
    if(pEventInfo.pMdfItmBuf==NULL)
    {
        LOG_Dbg_Msg("装置强制记日志申请内存出错!\n",0,0,0,0,0,0);
        return ;
    }
    for( i=0; i<iCount; i++)
    {

        pEventInfo.pMdfItmBuf[i].ucAttrib=STRINGTYPE;

        pValue =(char *)malloc(10);
        if(pValue==NULL)
        {
            LOG_Dbg_Msg("DI强制记日志申请内存出错!\n",0,0,0,0,0,0);
            bIfSendMsg = FALSE;
            break;
        }
        sprintf(pEventInfo.pMdfItmBuf[i].aucItemName,pdich[i].aucName);
        if(pdich[i].PreStats==1)
        {
            if(ENG_MODE==0)
                sprintf(pValue,"强制合");
            else if(ENG_MODE==1)
                sprintf(pValue,"Forced closing");
        }
        else if(pdich[i].PreStats==0)
        {
            if(ENG_MODE==0)
                sprintf(pValue,"强制分");
            else if(ENG_MODE==1)
                sprintf(pValue,"Forced opening");
        }
        else
        {
            if(ENG_MODE==0)
                sprintf(pValue,"非强制");
            else if(ENG_MODE==1)
                sprintf(pValue,"Non-force");
        }
        pEventInfo.pMdfItmBuf[i].oldVal.str=pValue;

        pValue =(char *)malloc(10);
        if(pValue==NULL)
        {
            LOG_Dbg_Msg("DI强制记日志申请内存出错!\n",0,0,0,0,0,0);
            bIfSendMsg = FALSE;
            break;
        }
        if(pdich[i].NewStats==1)
        {
            if(ENG_MODE==0)
                sprintf(pValue,"强制合");
            else if(ENG_MODE==1)
                sprintf(pValue,"Forced closing");
        }
        else if(pdich[i].NewStats==0)
        {
            if(ENG_MODE==0)
                sprintf(pValue,"强制分");
            else if(ENG_MODE==1)
                sprintf(pValue,"Forced opening");
        }
        else
        {
            if(ENG_MODE==0)
                sprintf(pValue,"非强制");
            else if(ENG_MODE==1)
                sprintf(pValue,"Non-force");
        }
        pEventInfo.pMdfItmBuf[i].newVal.str=pValue;
    }
    if(!bIfSendMsg)
    {
        False_Handle(&pEventInfo);
        return;
    }
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        False_Handle(&pEventInfo);
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}


void  DebugRunSwitchToLog(uint32_t iOpType,BOOL bOldType, uint16_t usOpSrc)
{
    /*测试/运行切换记日志*/

    char aucBuf1[64];
    char aucBuf2[64];
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SYSOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(iOpType==0x5a)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"测试态");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Test state");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf1,"运行态");
        else if(ENG_MODE==1)
            sprintf(aucBuf1,"Operation state");
    }
    if(bOldType)
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"测试态");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Test state");
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(aucBuf2,"运行态");
        else if(ENG_MODE==1)
            sprintf(aucBuf2,"Operation state");
    }
    if(ENG_MODE==0)
        sprintf(pEventInfo.aucOprInf,"装置由%s切换到%s(操作源类型%d)!",aucBuf2,aucBuf1, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(pEventInfo.aucOprInf,"Switch the IED from %s to %s(the operational source type is %d)!",aucBuf2,aucBuf1, usOpSrc);

    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }

}

void VI_New_MeaDoToLog(uint8_t PtNum,uint8_t OptNum,uint32_t OptPara,uint32_t usTqPara,
                       uint8_t *pIpAddr, uint16_t usOpSrc)
{
    char aucBuf1[64];
    char aucBuf2[64];
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SYSOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(ENG_MODE==0)
        sprintf(aucBuf1,"遥控点号为%d(操作源类型%d)",PtNum+1, usOpSrc);
    else if(ENG_MODE==1)
        sprintf(aucBuf1,"Number of telecommand point is %d(the operational source type is %d)",PtNum+1,usOpSrc);
    switch(OptNum)
    {
        case 0x55:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"短脉宽动作");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Short pulse width operating");
            break;
        case 0x5a:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"长脉宽动作");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Long pulse width operating");
            break;
        case 0xa5:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"返回");
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Return");
            break;
        case 0xaa:
            if(ENG_MODE==0)
                sprintf(aucBuf2,"脉宽%ld,同期参数%ld",OptPara,usTqPara);
            else if(ENG_MODE==1)
                sprintf(aucBuf2,"Pulse width %ld, synchronization parameter %ld",OptPara,usTqPara);

            break;
        default:
            break;
    }
    if(pIpAddr!=NULL)
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"遥控命令:%s,%s,源Ip地址:%u.%u.%u.%u",
                    aucBuf1,aucBuf2,pIpAddr[3],pIpAddr[2],pIpAddr[1],pIpAddr[0]);
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf,"Telecommand: %s, %s, source IP address: %u.%u.%u.%u",
                    aucBuf1,aucBuf2,pIpAddr[3],pIpAddr[2],pIpAddr[1],pIpAddr[0]);
    }
    else
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"遥控命令:%s,%s",aucBuf1,aucBuf2);
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf," Telecommand: %s, %s",aucBuf1,aucBuf2);
    }
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}
/*远方就地切换记日志*/
void FarStatsChangeToLog(uint8_t iStats)
{
    int retcode;
    OPR_LOG_TYPE pEventInfo;
    EP_DATE_TIME time;
    TM_Get_Sys_Time(&time);
    pEventInfo.ucOprType=SYSOPE;
    memset(pEventInfo.aucOprSourceStr,0,64);
    pEventInfo.tmOprTime=time;
    if(iStats==0)
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"切换到远方态");
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf,"Switch to the remote state");
    }
    else if(iStats==1)
    {
        if(ENG_MODE==0)
            sprintf(pEventInfo.aucOprInf,"切换到就地态");
        else if(ENG_MODE==1)
            sprintf(pEventInfo.aucOprInf,"Switch to the local state");
    }
    pEventInfo.ulMdfItmNum=0;
    retcode=msgQSend(OperMessage,(char *)&pEventInfo,sizeof(OPR_LOG_TYPE),
                     NO_WAIT,MSG_PRI_NORMAL);
    if(retcode==ERROR)
    {
        LOG_Dbg_Msg("Can't send msg to OPerLog Task\n",0,0,0,0,0,0);
        return ;
    }
}
