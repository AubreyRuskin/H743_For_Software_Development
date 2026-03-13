/* realdata.c - This file contains programs to manager realtime data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 11nov06, dy change the style of program and separate to three file: realdata.c, hwcfg.c, dspai.c.
01c, 26jul03, hdx Updated to version 1.0.
01b, 27feb03, hdx Updated to version 0.2.
01a, 26jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains programs to manager realtime data.
INCLUDES: realdata.h
*/

/* includes */

#include "hwcfg.h"
#include "realdata.h"
#include "errtest.h"
#include "dspai.h"
#include "ext_box.h"
#include "datetime.h"
#include "logmsg.h"
#include "miscfunc.h"
#include "view.h"
#include "filetool.h"
#include "sysinfo.h"
#include "swcfg.h"
#include "dsp.h"
#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"
#include <ioLib.h>
#include <intLib.h>
#include <taskLib.h>
#include <semLib.h>
#include "configerrordisp.h"
#include "FileCRC.h"
#include "intLib.h"
#include "bspinterface.h"

#include "redun_box.h"

#include "OPT_Data.h"

/* 合并版所有平台包含 */
// #include "config04.h"		/* BSP函数接口，励磁装置需要特定的该文件，目前没有提供 */

#include "VTBOX_Interface.h"		/* virtual box. */
#include "VTBOX_SamInterface.h"
#include "VTBOX_Data.h"
#include "POLE_Data.h"
#include "HDL_Data.h"

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "spi_com.h"
#endif

#include  "HDL_VtBox.h"

/* defines */

#define FREE_INIT_MEM 0

/* typedefs */

/* locals */

/* globals */

u_int uiPwrFreq_g;       	/* 电力系统频率，50或者60 */
u_int uiAiRate_g;      /* 模拟量（AI）采样周期，次/秒 */
u_int uiAiPts_g;              /* 模拟量（AI）每周波采样点数 */
u_int uiDioRate_g;                /* 数字量（DI/DO）刷新周期，次/秒 */
BOOL bOneAmpSys_g;               /* 适合1A额定电流系统 */
BOOL bFiveAmpSys_g;      /* 适合5A额定电流系统 */
BOOL bDoubleCPUFlag_g;				/* 是否支持双CPU */
uint8_t ucCPUPos_g;				/* CPU插槽位置 */
uint8_t uiAppType_g;					/* 应用类型 */
BOOL g_bAppAlarm = FALSE;  /* 应用告警 */
BOOL bFstOrSecFlag = FALSE;   /* 一次/二次选择, FALSE: 二次; TRUE: 一次 */

uint32_t ulDwordBitArr_g[32]=
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,
};

/* DB as global for macro/inline function accessing from other modules. */


extern uint16_t usMinInterval;  /* 扫描任务最小间隔点数 */

/* 应用标识匹配表 */
EP_APP_MAP aAppMapArr[] =
{
    {APP_COM, "通用保护装置"},
    {APP_LINE, "中高压线路保护装置"},
    {APP_TRANS, "中高压变压器保护装置"},
    {APP_BUS, "中高压母差保护装置"},
    {APP_PROT_MEA_MERGE, "中高压保护测控一体化装置"},
    {APP_LOW_PROT, "低压保护测控装置"},
    {APP_EXCITE, "励磁调节器装置"},
    {APP_STAB_CONTROL, "稳定控制装置"},
    {APP_MT, "铁路自动化装置"},
    {APP_NO_ELEC, "非电量保护装置"},
    {APP_INTEL_BOX, "智能操作箱"}
};

/* File global variables. */

/* forward declarations */

/***********************************************************************
* RD_Refresh_DI - 更新DI
*
* RETURNS: 无
*
*/
static void RD_Refresh_DI(void);

/* global functions */

/* read the data from substation.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void readDataSub(void);

/***********************************************************************
* AdcintDisable - 中断采样
*
* RETURNS: 无
*
*/
void AdcintDisable(void);

/* static void RD_Clone_AI(int iNewPts); */

/* global declarations */

/***********************************************************************
* RD_Get_Handle - 取得实时数据I/O通道的索引
*
* RETURNS: 用来索引实时数据I/O通道的void指针，或者NULL表示调用出错
*
*/
void *RD_Get_Handle(
    uint8_t *strLgcId,			/* 逻辑标识字符串（名称） */
    int  iHdlType		/* 句柄类型 */
)
{
    RD_LGC_AI_CH *plgcai; /* 各种配置结构 */
    RD_MSU_AI_CH *plgmsu;
    RD_HW_AO_CH *pHwAo;		/* analog output. */
    RD_LGC_DI_CH *plgcdi;
    RD_LGC_DO_CH *plgcdo;
    RD_LGC_LED_CH *plgcled;
    RD_LGC_PI_CH *plgcpi;
    RD_LGC_PO_CH *plgcpo;
    RD_PART_INFO  *pDIMod;

    assert(strLgcId && strlen(strLgcId)<=MAX_ID_LEN);
    if(iHdlType==RD_LGC_AI_HDL)
    {
        for (plgcai=plgcaich_g; plgcai<plgcaich_g+iLgcAiChNum_g; plgcai++)
        {
            if (!strcmp(strLgcId, plgcai->aucId))
                return plgcai;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  AI.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "AI句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else  if(iHdlType==RD_LGC_DI_HDL)
    {
        for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
        {
            if (!strcmp(strLgcId, plgcdi->aucId))
                return plgcdi;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  DI.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "DI句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

#ifndef EDP01_CA_OPT_BUILD		/* 便于调试,无软硬压板开入时,此时为软压板 */
        assert(FALSE);
#endif

        return NULL;
    }
    else if(iHdlType==RD_LGC_DO_HDL)
    {
        for (plgcdo=plgcdoch_g; plgcdo<plgcdoch_g+iLgcDoChNum_g; plgcdo++)
        {
            if (!strcmp(strLgcId, plgcdo->aucId))
                return plgcdo;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  DO.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "DO句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else if(iHdlType==RD_LGC_LED_HDL)
    {
        for (plgcled=plgcledch_g; plgcled<plgcledch_g+iLgcLedChNum_g; plgcled++)
        {
            /* logMsg("%s	%s\n",strLgcId, plgcled->aucId, 0, 0, 0, 0);	 */	/* 测试 */
            if (!strcmp(strLgcId, plgcled->aucId))
                return plgcled;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  LED.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "LED句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else if(iHdlType==RD_LGC_DI_MOD_HDL)
    {
        /*获得开入摸件句柄  */
        for (pDIMod=apartinf_g; pDIMod<apartinf_g+rdinfo_g.unPartNum; pDIMod++)
        {
            if (!strcmp(strLgcId, pDIMod->aucId))
            {
                assert(pDIMod->ucType==0x12);/* 保证是开入板 */
                return pDIMod;
            }
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  DI MOD.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "DI句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }

    /* 获得测量句柄 */ /* Added by DY */
    else if(iHdlType==RD_MSU_AI_HDL)
    {
        for(plgmsu = pmsuaich_g; plgmsu < pmsuaich_g+iMsuAiChNum_g; plgmsu++)
        {
            if (!strcmp(strLgcId, plgmsu->aucId))
                return plgmsu;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  MSU.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "测量AI句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        /* assert(FALSE); */

        return NULL;
    }
    else if(iHdlType == RD_LGC_PI_HDL)
    {
        /* 脉冲输入 */
        for(plgcpi = plgcpich_g; plgcpi < plgcpich_g+iLgcPiChNum_g; plgcpi++)
        {
            if (!strcmp(strLgcId, plgcpi->aucId))
                return plgcpi;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  PI.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "PI句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else if(iHdlType == RD_LGC_PO_HDL)
    {
        /* 脉冲输出 */
        for(plgcpo = plgcpoch_g; plgcpo < plgcpoch_g+iLgcPoChNum_g; plgcpo++)
        {
            if (!strcmp(strLgcId, plgcpo->aucId))
                return plgcpo;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  PO.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n句柄逻辑标志为%s\n", "PO句柄获取出错", strLgcId);
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else if(iHdlType == RD_LGC_AO_HDL)	/* 模拟量输出 */
    {
        /* 获得AO句柄 */
        for (pHwAo=phwaoch_g; pHwAo<phwaoch_g+iHwAoChNum_g; pHwAo++)
        {
            if (!strcmp(strLgcId, pHwAo->aucId))
            {
                return pHwAo;
            }
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  AO.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n", "获取句柄不存在");
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);

        return NULL;
    }
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\".\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        sprintf(MessageStr_g, "%s\n", "获取句柄不存在");
        if(pMessageBox)
        {
            pMessageBox("[获取硬件配置句柄出错]\n", MessageStr_g);
        }
#endif

        assert(FALSE);   /* Not found. */

        return NULL;
    }
}

/***********************************************************************
* RD_Reg_Smpl_Func - 注册采样节拍关联的函数
*
* RETURNS: 无
*
*/
void RD_Reg_Smpl_Func(
    void (*pfUser)(void *pvParm), 		/* 用户指定的关联函数*/
    void *pvParm, 		/* 代用户传递的参数 */
    u_int uiPts		/* 周期性调用关联函数所间隔的采样节拍数 */
)
{
    RD_REG_FUNC_AI *pregf;
    int iLockKey;

    assert(pfUser && uiPts);

    iLockKey=intLock();
    for (pregf=aregf_g; pregf<aregf_g+MAX_AI_FUNC_NUM; pregf++)
    {
        if (!pregf->pfUser)
        {
            pregf->pfUser=pfUser;
            pregf->pvParm=pvParm;
            pregf->uiPts=uiPts;
            pregf->uiCnt=uiPts;
            break;
        }
    }
    intUnlock(iLockKey);

    assert(pregf<aregf_g+MAX_AI_FUNC_NUM);
}

/***********************************************************************
* RD_Del_Smpl_Func - 删除采样节拍关联的函数
*
* RETURNS: 无
*
*/
void RD_Del_Smpl_Func(
    void (*pfUser)(void *pvParm), 		/* 用户指定的关联函数*/
    void *pvParm, 		/* 代用户传递的参数 */
    u_int uiPts						/* 周期性调用关联函数所间隔的采样节拍数 */
)
{
    RD_REG_FUNC_AI *pregf;
    RD_REG_FUNC_AI *pregfLast;
    int iLockKey;

    assert(pfUser && uiPts);

    iLockKey=intLock();
    for (pregf=aregf_g; pregf<aregf_g+MAX_AI_FUNC_NUM; pregf++)
    {
        if (pregf->pfUser==pfUser && pregf->pvParm==pvParm
                && pregf->uiPts==uiPts)
        {
            pregfLast=pregf+1;
            if (pregfLast->pfUser)      /* Valid data must be the header. */
            {
                while (pregfLast->pfUser)
                    pregfLast++;

                *pregf=*(pregfLast-1);
                (pregfLast-1)->pfUser=NULL;
            }
            else
                pregf->pfUser=NULL;

            break;
        }
    }
    intUnlock(iLockKey);

    assert(pregf<aregf_g+MAX_AI_FUNC_NUM);
}

/* 控制LED状态
 * 参数：   pvLedHnd，用来索引LED对象的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          bOn，LED灯的状态，TRUE=点亮；FALSE=熄灭
 * 返回值： 无 */
void RD_Set_LED(void *pvLedHnd, BOOL bOn)
{
    RD_LGC_LED_CH *plgcled;
    VI_RUN_INFO *pinf;

    plgcled=(RD_LGC_LED_CH*)pvLedHnd;

    bOn=bOn?TRUE:FALSE;

    if (plgcled->bSts!=bOn)
    {
        plgcled->bSts=bOn;

        /* Report LED change. */
        pinf=VI_Run_Info_Wr_P();

        if (bViewModIsInit_g)	/* VI模块是否完成标志 */
        {
            pinf->bViewModIsInit = TRUE;
        }
        else
        {
            pinf->bViewModIsInit = FALSE;
        }

        pinf->type=LED_CHG;

        pinf->msg.led.pcfg=plgcled;
        pinf->msg.led.bSts=bOn;
        if (plgcled->bIsHwLED)
            pinf->msg.led.unIdx=plgcled-plgcledch_g;
        else
            pinf->msg.led.unIdx=plgcled-plgcledch_g-iHwLedChNum_g;

        VI_End_Wr_Run_Info();
    }
}

/* 控制DO数据实时输出
 * 参数：   pvDoHnd，用来索引DO对象的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          iVal，DO输出值
 * 返回值： 无 */
void RD_Set_DO(void *pvDoHnd, int iVal)
{
    RD_LGC_DO_CH *plgcdo;
    BOOL bClose;

    plgcdo=(RD_LGC_DO_CH*)pvDoHnd;

    bClose=(iVal==0)?FALSE:TRUE;

    if (plgcdo->iVal!=iVal)
    {
        plgcdo->iVal=iVal;
        if (plgcdo->iForceSts==-1
                &&((!(uiEdpStatus_g & (LOCK_DO | SYS_LOCK_DO)))||(uiAppType_g==APP_INTEL_BOX)))
        {
            if (plgcdo->mod==RD_SPI_DO)
            {

                SIO_Set_DO(plgcdo->pvSrc, bClose);

            }
#ifdef EXCITE_BUILD
            else if(plgcdo->mod==RD_REDUN_DO)
            {
                Redun_Set_DO(plgcdo->pvSrc, bClose);		/* Added by DY 7/3/2006, for excite */
            }
#endif

            else  if((plgcdo->mod==RD_OPT1_DO)||(plgcdo->mod==RD_OPT2_DO))
            {
                /*2006-2-15  */

                /*LOG_Dbg_Msg("Enter OPT DO! \n",0,0,0,0,0,0);*/
                OPT_Set_DO(plgcdo->pvSrc, bClose);
            }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
            else  if(plgcdo->mod==RD_SAME_POLE_DO)
            {
                /*2007-3-29 张云  */

                POLE_Set_DO(plgcdo->pvSrc, bClose);
            }
            else  if(plgcdo->mod==RD_HDL_BOX_DO)
            {
                /*2007-3-29 张云  */

                HDL_Set_DO(plgcdo->pvSrc, iVal);
            }
#endif
            else if (plgcdo->mod == RD_VT_BOX_DO)
            {
                /* virtual box DO. */
                assert (FALSE);
            }
            else
                assert(FALSE);
        }
    }
}

/* 强制DO输出（用于遥控/开出传动等）
 * 参数：   iIdx, 开出量索引，从0开始
 *          iSts, 预设置的状态: TRUE, FALSE or -1 解除强制.
 * 返回值： 无 */
void RD_Force_DO(int iIdx, int iSts)
{
    RD_LGC_DO_CH *plgcdo;
    int iSet;
    STATUS vxsts;

    assert(iIdx>=0 && iIdx<iLgcDoChNum_g);
    assert(iSts==TRUE || iSts==FALSE || iSts==-1);

    plgcdo=plgcdoch_g+iIdx;

    if((plgcdo->mod==RD_OPT1_DO)||(plgcdo->mod==RD_OPT2_DO))
    {
        /*2006-4-15日，若是光纵虚拟DO，则不能进行强制操作，则空操作  */
        return;
    }

    if(plgcdo->mod==RD_SAME_POLE_DO)
    {
        /*2006-4-15日，若是同杆并架虚拟DO，则不能进行强制操作，则空操作  */
        return;
    }

    iSet=-1;

    vxsts=taskLock();
    assert(vxsts==OK);

    if (iSts!=-1)                       /* Force TRUE or FALSE. */
    {
        if (plgcdo->iForceSts!=-1)
        {
            if (plgcdo->iForceSts!=iSts)
                iSet=iSts;
        }
        else if (plgcdo->iVal!=iSts)
            iSet=iSts;
    }
    else                                /* Release force. */
    {
        if (plgcdo->iForceSts==-1)
            goto ret;
        else if (plgcdo->iForceSts!=plgcdo->iVal)
            iSet=plgcdo->iVal;
    }

    plgcdo->iForceSts=iSts;

    if (iSet!=-1)
    {
        if (plgcdo->mod==RD_SPI_DO)
        {
            SIO_Set_DO(plgcdo->pvSrc, iSet);
        }
#ifdef EXCITE_BUILD
        else if(plgcdo->mod==RD_REDUN_DO)
        {
            Redun_Set_DO(plgcdo->pvSrc, iSet);	/* for excite */
        }
#endif

        else  if((plgcdo->mod==RD_OPT1_DO)||(plgcdo->mod==RD_OPT2_DO))
        {
            /*2006-2-15  */
            OPT_Set_DO(plgcdo->pvSrc,iSet );
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        else  if(plgcdo->mod==RD_SAME_POLE_DO)
        {
            /*2006-3-29 张云  */

            POLE_Set_DO(plgcdo->pvSrc,iSet );
        }
        else  if(plgcdo->mod==RD_HDL_BOX_DO)
        {
            /*2006-3-29 张云  */

            HDL_Set_DO(plgcdo->pvSrc,iSet );
        }
#endif
        else if (plgcdo->mod == RD_VT_BOX_DO)
        {
            /* virtual box DO. */
            assert (FALSE);
        }
        else
            assert(FALSE);
    }

ret:
    vxsts=taskUnlock();
    assert(vxsts==OK);
}

/* 获取GOOSE DO关联压板状态
 * Para:
 *     iIdx, 序号.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RD_Get_DO_Link(int iIdx)
{
    RD_LGC_DO_CH *plgcdo;
    BOOL bRtYabanValue;

    plgcdo = plgcdoch_g+iIdx;
    if (plgcdo->mod != RD_HDL_BOX_DO)
    {
        return TRUE;
    }
    else
    {
        if (plgcdo->linkNum == -1)
        {
            return TRUE;
        }
        else
        {
            SCI_Get_Yaban_Value(plgcdo->linkNum, &bRtYabanValue, 0);

            return bRtYabanValue;
        }
    }

    return TRUE;
}

/* Read hardware DO status.
 * Parameters:
 *      iIdx, index of DO(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DO. */
int RD_Mea_Hw_DO(int iIdx)
{
    RD_LGC_DO_CH *plgcdo;

    assert(iIdx>=0 && iIdx<iLgcDoChNum_g);

    plgcdo=plgcdoch_g+iIdx;

    if (plgcdo->iForceSts!=-1)
        return plgcdo->iForceSts | 0x8000;
    else
        return plgcdo->iVal;
}

/* 转换AI采样节拍计数器的值为系统us时钟
 * 参数：   AI采样节拍计数器值
 * 返回值： 该采样时刻对应的系统32位us时钟 */
uint32_t RD_AI_Cnt_To_us(uint32_t ulCnt)
{
    uint32_t ulRslt;
    int iLockKey;
    int32_t iCntDiff;
    uint32_t ulCntDiffAbs;

    iLockKey=intLock();

    iCntDiff=(int)(ulCnt-rdinfo_g.ulSynCnt);

    if(iCntDiff>=0)
    {
        ulCntDiffAbs=(uint32_t)(iCntDiff);
        ulRslt=rdinfo_g.ulSynTime+ulCntDiffAbs*rdinfo_g.uiSmplPeriod;
    }
    else
    {
        ulCntDiffAbs=(uint32_t)(-iCntDiff);
        ulRslt=rdinfo_g.ulSynTime- ulCntDiffAbs*rdinfo_g.uiSmplPeriod;
    }

#if 0 /* 原有实现, 导致录波文件启动时标不正确 */
    ulRslt=(ulCnt-rdinfo_g.ulSynCnt)*rdinfo_g.uiSmplPeriod+rdinfo_g.ulSynTime;
#endif

    intUnlock(iLockKey);

    return ulRslt;
}

#ifdef EDP01_CA_OPT_BUILD /* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */
/* 报告AI引擎采样同步信息
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulTime，采样同步脉冲到来时的系统32位us时钟
 *          ulLastClk，这里指相应的采样CNT计数，2006－5－21张云
 * 返回值： 无 */
void RD_Syn_AI_Clk(void *pvAiMod, uint32_t ulTime, uint32_t ulLastClk)
{
    /*张云针对光CT，重新实现 ，没有中断，只是不太精确，2006－5－21 */
    RD_AI_MOD *paimod;
    int iLockKey;

    paimod=(RD_AI_MOD*)pvAiMod;

    iLockKey=intLock();
    rdinfo_g.ulSynTime=ulTime;
    rdinfo_g.ulSynCnt=ulLastClk;
    intUnlock(iLockKey);
}

#else
/***********************************************************************
* RD_Syn_AI_Clk - 报告AI引擎采样同步信息
*
* RETURNS: 无
*
*/
void RD_Syn_AI_Clk(
    void *pvAiMod,		/* 用来索引AI引擎的void指针，应该由本模块在初始化AI通道的时候提供给底层I/O */
    uint32_t ulTime, 			/* 采样同步脉冲到来时的系统32位us时钟 */
    uint32_t ulLastClk		/* 清零前的采样时钟值 */
)
{
    RD_AI_MOD *paimod;

    paimod=(RD_AI_MOD*)pvAiMod;		/* 读指针，AI索引的指针 */

    /* 同步时间 */
    rdinfo_g.ulSynTime=ulTime; 		/* 当前时间 */

    rdinfo_g.ulSynCnt=paimod->ulNextCnt; 		/* 同步 */
    rdinfo_g.ulSynClk=ulLastClk+1; /* 同步采样点数 */
}
#endif

/* 获得原始采样时钟节拍，该节拍可用来确定预处理通道傅氏变换所采用的系数
 * 参数：   AI采样节拍计数器值
 * 返回值： 该采样时刻对应的原始采样时钟节拍 */
uint32_t RD_Get_Smpl_Clk(uint32_t ulCnt)
{
    uint32_t ulRslt;
    int iLockKey;

    iLockKey=intLock();

    ulRslt=ulCnt-rdinfo_g.ulSynCnt;

    if ((int32_t)ulRslt<0)
        ulRslt+=rdinfo_g.ulSynClk;

    intUnlock(iLockKey);

    assert(ulRslt<rdinfo_g.ulSynClk);

    return ulRslt;
}

/***********************************************************************
* RD_AI_Dat_P - 取得模块AI逻辑通道和预处理数据指针
*
* RETURNS:  指向该AI引擎的第0个逻辑采样通道数据的指针
*
* 注意: 要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
*              AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
*              内部数据按照通道优先的方式进行组织
*
*/
float *RD_AI_Dat_P(
    void *pvAiMod, 		/* 用来索引AI引擎的void指针，应该由本模块在初始化AI通道的时候提供给底层I/O */
    uint32_t ulSmplClk, 			/* 递增采样时钟，该时钟在同步脉冲到来的时刻清零 */
    COMPLEX **ppxWr,		/* 用来返回指向该AI引擎的第0个预处理通道数据的指针 */
    float **dcdata     /* pointer. */
)
{
    RD_AI_MOD *paimod;
    static u_int uiWorkFg;
#define DSP_AI_FG   0x01
#define EXT_AI_FG   0x02
#ifdef EDP01_CA_OPT_BUILD					/* EDP01平台C-A版光CT获取数据，屏蔽本机采样 */
    static u_int uiAllWorkFg=EXT_AI_FG;
#else
    static u_int uiAllWorkFg=DSP_AI_FG;
#endif

    static int iNotSyn;
    int i;
    uint32_t  ulMissedClk;

    int iLockKey;
    RD_AI_MOD *pOptAiMod;
    BOOL *pbWrAiValidDbWork = NULL;  /* 光差通道状态当前设置位置 */
    OPT_CH_STS *pWrOptChStsDbWork = NULL;
    int k;

    char strTaskInfo[512];

    paimod=(RD_AI_MOD*)pvAiMod;

    if (uiWorkFg == uiAllWorkFg)
    {
        /* 机箱标志 */
        /* #ifdef NDEBUG */
        /* Enable this miss point check in debug mode will cause error report! */

        if((ulSmplClk==(paimod->ulHeadClk+1)%(10*uiAiPts_g)))  /* 2011-11-17日 ZY 修改 */
        {
            /* 若未丢点 */
            paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes); 			/* 推向下一点 */
            /* 到了末尾 */
            if (paimod->pfWork >= lgcaidb_g.pfBufEnd)
            {
                paimod->pfWork=paimod->pfDbBgn;
                paimod->pxWork=paimod->pxDbBgn;
                paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
                paimod->pfDcWork=paimod->pfDcDbBgn;

                if(paimod==&aimodDsp_g)
                {
                    for(k=0; k<2; k++)
                    {
                        pOptAiMod=aimodOpt_g+k;
                        iLockKey=intLock();        /*保持数据完整性  */
                        pOptAiMod->ulNextCnt++;
                        pOptAiMod->ulHeadClk=ulSmplClk;
                        pOptAiMod->pfWork=pOptAiMod->pfDbBgn;
                        pOptAiMod->pxWork=pOptAiMod->pxDbBgn;
                        pOptAiMod->pbAiValidDbWork = pOptAiMod->pbAiValidDbBgn;
                        pOptAiMod->pOptChStsDbWork = pOptAiMod->pOptChStsDbBgn;
                        intUnlock(iLockKey);
                    }
                }
            }
            else
            {
                paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
                paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                paimod->pfDcWork = (float *)((uint8_t *)paimod->pfDcWork+lgcaidb_g.uiChBytes);

                if(paimod==&aimodDsp_g)
                {
                    for(k=0; k<2; k++)
                    {
                        pOptAiMod=aimodOpt_g+k;
                        iLockKey=intLock();        /*保持数据完整性  */
                        pOptAiMod->ulNextCnt++;
                        pOptAiMod->ulHeadClk=ulSmplClk;
                        pOptAiMod->pfWork=(float*)((uint8_t*)pOptAiMod->pfWork+lgcaidb_g.uiChBytes);
                        pOptAiMod->pxWork=(COMPLEX*)((uint8_t*)pOptAiMod->pxWork+calcaidb_g.uiChBytes);
                        pOptAiMod->pbAiValidDbWork=pOptAiMod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                        pOptAiMod->pOptChStsDbWork=(OPT_CH_STS *)((uint8_t*)pOptAiMod->pOptChStsDbWork+optstsdb_g.uiChBytes);
                        intUnlock(iLockKey);
                    }

                }

            }
            *paimod->pbAiValidDbWork=TRUE;		/* 表明该点的AI数据有效 */
            paimod->ulHeadClk=ulSmplClk; 			/* 当前处理的同步计数 */
            paimod->ulNextCnt++; 					/* 表示下一个同步点 */

            if(paimod==&aimodDsp_g)
            {
                for(k=0; k<2; k++)
                {
                    pOptAiMod=aimodOpt_g+k;

                    pbWrAiValidDbWork = pOptAiMod->pbAiValidDbWork+MAX_OPS_DELAY_NUM*aivaliddb_g.uiTotalCh;
                    if (pbWrAiValidDbWork >= pOptAiMod->pbAiValidDbEnd)
                    {
                        pbWrAiValidDbWork = pbWrAiValidDbWork - aivaliddb_g.ulBufLen;
                    }

                    pWrOptChStsDbWork = pOptAiMod->pOptChStsDbWork+MAX_OPS_DELAY_NUM*optstsdb_g.uiTotalCh;
                    if (pWrOptChStsDbWork >= pOptAiMod->pOptChStsDbEnd)
                    {
                        pWrOptChStsDbWork = pWrOptChStsDbWork-optstsdb_g.ulBufLen;
                    }

                    *pbWrAiValidDbWork=FALSE;

                    if(OPT_ChCurIsSamSyn(k))/*2009-3-5  ZY  */
                    {
                        pWrOptChStsDbWork->lTsse=0;
                        pWrOptChStsDbWork->bValid=TRUE;   /*通道采样同步有效  */
                    }
                    else
                    {
                        pWrOptChStsDbWork->lTsse=0x7fffffff;
                        pWrOptChStsDbWork->bValid=FALSE;   /*通道采样同步无效  */
                    }
                    pWrOptChStsDbWork->bComValid=FALSE;   /*通道通信数据接收无效  2006-7-28日张云修改*/
                    pWrOptChStsDbWork->bComStable=FALSE;   /* 通讯稳定与否 */
                    pWrOptChStsDbWork->bDataIsCredible=FALSE;   /* 数据可信与否 2009-3-9 ZY */
                    pWrOptChStsDbWork->iRcvSndDiffChgTime=0; /*通道收发时间差是否发生变化  2009-2-13日ZY  */
                }

            }

        }
        else if((ulSmplClk == (paimod->ulHeadClk+2)%(10*uiAiPts_g)) 		/* 由48点改为24点时，由480改为240 */
                ||(ulSmplClk==(paimod->ulHeadClk+3)%(10*uiAiPts_g)))
        {
            /*若丢1点或2点  */
            if(ulSmplClk == (paimod->ulHeadClk+2)%(10*uiAiPts_g))
            {
                ulMissedClk=1;
            }
            else
            {
                ulMissedClk=2;
            }
            if (paimod == &aimodDsp_g) 			/* 主机箱 */
            {
                LOG_Write(LOG_KERNEL, "告警:主机箱DSP采样AI数据节拍丢失.\n", NULL);
            }
            else
            {
                sprintf(strTaskInfo, "告警:扩展机箱DSP采样AI数据节拍丢失. 当前采样节拍%d，前点节拍为%d\n",
                        (int)ulSmplClk, (int)paimod->ulHeadClk);

                LOG_Write(LOG_KERNEL, strTaskInfo, NULL);
            }

            /* 丢点处理，每点处理一次 */
            for(i=0; i<=ulMissedClk; i++)
            {
                /*返回正确的写位置 */
                paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
                if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
                {
                    paimod->pfWork=paimod->pfDbBgn;
                    paimod->pxWork=paimod->pxDbBgn;
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
                    paimod->pfDcWork=paimod->pfDcDbBgn;

                    if(paimod==&aimodDsp_g)
                    {
                        for(k=0; k<2; k++)
                        {
                            pOptAiMod=aimodOpt_g+k;
                            iLockKey=intLock();        /*保持数据完整性  */
                            pOptAiMod->ulNextCnt++;
                            pOptAiMod->ulHeadClk=ulSmplClk;
                            pOptAiMod->pfWork=pOptAiMod->pfDbBgn;
                            pOptAiMod->pxWork=pOptAiMod->pxDbBgn;
                            pOptAiMod->pbAiValidDbWork=pOptAiMod->pbAiValidDbBgn;
                            pOptAiMod->pOptChStsDbWork=pOptAiMod->pOptChStsDbBgn;
                            intUnlock(iLockKey);
                        }

                    }

                }
                else
                {
                    paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                    paimod->pfDcWork = (float *)((uint8_t *)paimod->pfDcWork+lgcaidb_g.uiChBytes);

                    if(paimod==&aimodDsp_g)
                    {
                        for(k=0; k<2; k++)
                        {
                            pOptAiMod=aimodOpt_g+k;
                            iLockKey=intLock();        /*保持数据完整性  */
                            pOptAiMod->ulNextCnt++;
                            pOptAiMod->ulHeadClk=ulSmplClk;
                            pOptAiMod->pfWork=(float*)((uint8_t*)pOptAiMod->pfWork+lgcaidb_g.uiChBytes);
                            pOptAiMod->pxWork=(COMPLEX*)((uint8_t*)pOptAiMod->pxWork+calcaidb_g.uiChBytes);
                            pOptAiMod->pbAiValidDbWork=pOptAiMod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                            pOptAiMod->pOptChStsDbWork=(OPT_CH_STS *)((uint8_t*)pOptAiMod->pOptChStsDbWork+optstsdb_g.uiChBytes);
                            intUnlock(iLockKey);
                        }

                    }

                }

                if(i == ulMissedClk)
                {
                    /*若是有效点  */
                    *paimod->pbAiValidDbWork=TRUE;
                }
                else
                {
                    /*若是无效点  */
                    *paimod->pbAiValidDbWork=FALSE;
                }

                paimod->ulNextCnt++;

                if(paimod==&aimodDsp_g)
                {
                    for(k=0; k<2; k++)
                    {
                        pOptAiMod=aimodOpt_g+k;

                        pbWrAiValidDbWork = pOptAiMod->pbAiValidDbWork+MAX_OPS_DELAY_NUM*aivaliddb_g.uiTotalCh;
                        if (pbWrAiValidDbWork >= pOptAiMod->pbAiValidDbEnd)
                        {
                            pbWrAiValidDbWork = pbWrAiValidDbWork - aivaliddb_g.ulBufLen;
                        }

                        pWrOptChStsDbWork = pOptAiMod->pOptChStsDbWork+MAX_OPS_DELAY_NUM*optstsdb_g.uiTotalCh;
                        if (pWrOptChStsDbWork >= pOptAiMod->pOptChStsDbEnd)
                        {
                            pWrOptChStsDbWork = pWrOptChStsDbWork-optstsdb_g.ulBufLen;
                        }

                        *pbWrAiValidDbWork=FALSE;

                        if(OPT_ChCurIsSamSyn(k))/*2009-3-5  ZY  */
                        {
                            pWrOptChStsDbWork->lTsse=0;
                            pWrOptChStsDbWork->bValid=TRUE;   /*通道采样同步有效  */
                        }
                        else
                        {
                            pWrOptChStsDbWork->lTsse=0x7fffffff;
                            pWrOptChStsDbWork->bValid=FALSE;   /*通道采样同步无效  */
                        }
                        pWrOptChStsDbWork->bComValid=FALSE;   /*通道通信无效  2006-7-28日张云修改*/
                        pWrOptChStsDbWork->bComStable=FALSE;   /* 通讯稳定与否 */
                        pWrOptChStsDbWork->bDataIsCredible=FALSE;   /* 数据可信与否 2009-3-9 ZY */
                        pWrOptChStsDbWork->iRcvSndDiffChgTime=0; /*通道收发时间差是否发生变化  2009-2-13日ZY  */
                    }


                }

            }

            paimod->ulHeadClk=ulSmplClk; /* 回到当前 点 */
        }
        else
        {
            /* 若丢多点,则设置错误,闭锁保护,重启装置 */
            if (paimod == &aimodDsp_g)
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%0x1FFFFF== 1)
                {
                    /* 每0x1FFFFF次报一次，第1次必须报 */

                    char TempInfo[256];
                    if(ENG_MODE==0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                                   "错误码:%02d\n",MAIN_BOX_LOST_DATE, 0);
                    }
                    else if(ENG_MODE==1)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                                   "Error code:%02d\n",MAIN_BOX_LOST_DATE, 0);
                    }
                    sprintf(TempInfo,"主机箱本地DSP采样连续丢点,前次采样点是%d,本次采样点是%d!!\n", (int)paimod->ulHeadClk, (int)ulSmplClk);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            else
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%0x1FFFFF == 1)
                {
                    /* 每0x1FFFFF次报一次，第1次必须报 */

                    char TempInfo[256];
                    if(ENG_MODE==0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                                   "错误码:%02d\n",EXT_BOX_LOST_DATE, 0);
                    }
                    else if(ENG_MODE==1)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                                   "Error code:%02d\n",MAIN_BOX_LOST_DATE, 0);
                    }
                    sprintf(TempInfo,"扩展机箱DSP采样连续丢点,前次采样点是%d,本次采样点是%d!!\n", (int)paimod->ulHeadClk, (int)ulSmplClk);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            paimod->ulHeadClk=ulSmplClk;
        }

        /* #endif */

    }
    else
    {
        if (++iNotSyn>300*uiPwrFreq_g*uiAiPts_g)
        {

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "主机箱与扩展机箱采样失步\n", 0, 0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "AI sampling  betwwen main and ext can not be synchronized\n", 0, 0);
            }
            iNotSyn=0;
        }

        paimod->ulHeadClk=ulSmplClk; 		/* 当前点 */

        /* Initialize uiAllWorkFg(only once). */
        /* 如果没有文件系统 */

        if (bdType_g == BOARD_TYPE_E01)
        {
            /* EDP01平台处理扩展机箱 */
            if (!uiWorkFg && (aimodExt_g.iLgcNum || iExtDiNum_g)) 				/* uiWorkFg为0，扩展机箱的逻辑通道和输入通道配置非0 */
                uiAllWorkFg |= EXT_AI_FG; 				/* 增加扩展机箱 */
        }

        if (paimod == &aimodDsp_g)
            uiWorkFg |= DSP_AI_FG;
        else if (paimod == &aimodExt_g)
        {
            assert(uiAllWorkFg & EXT_AI_FG); 			/* 允许扩展机箱 */
            uiWorkFg |= EXT_AI_FG; 					/* 增加扩展机箱 */
        }
        else
            assert(FALSE);

        if (!(uiAllWorkFg & EXT_AI_FG)) 					/* 没有扩展机箱 */
        {
            assert(paimod == &aimodDsp_g && uiWorkFg==uiAllWorkFg);

            aimodDsp_g.pfWork=aimodDsp_g.pfDbBgn;
            aimodDsp_g.pxWork=aimodDsp_g.pxDbBgn;
            aimodDsp_g.pfDcWork=aimodDsp_g.pfDcDbBgn;
            aimodDsp_g.ulNextCnt=1; 				/* 下一点 */
            aimodDsp_g.pbAiValidDbWork=aimodDsp_g.pbAiValidDbBgn;
            *aimodDsp_g.pbAiValidDbWork=TRUE;

            for(k=0; k<2; k++)
            {
                /*2006-2-12，只有当主机箱采样到达时，才进行光纵如下赋通道无效操作  */
                pOptAiMod=aimodOpt_g+k;
                iLockKey=intLock();        /*保持数据完整性  */
                pOptAiMod->ulNextCnt=1;
                pOptAiMod->ulHeadClk=ulSmplClk;
                pOptAiMod->pfWork=pOptAiMod->pfDbBgn;
                pOptAiMod->pxWork=pOptAiMod->pxDbBgn;
                pOptAiMod->pbAiValidDbWork=pOptAiMod->pbAiValidDbBgn;
                pOptAiMod->pOptChStsDbWork=pOptAiMod->pOptChStsDbBgn;
                intUnlock(iLockKey);

                pbWrAiValidDbWork = pOptAiMod->pbAiValidDbWork+MAX_OPS_DELAY_NUM*aivaliddb_g.uiTotalCh;
                if (pbWrAiValidDbWork >= pOptAiMod->pbAiValidDbEnd)
                {
                    pbWrAiValidDbWork = pbWrAiValidDbWork - aivaliddb_g.ulBufLen;
                }

                pWrOptChStsDbWork = pOptAiMod->pOptChStsDbWork+MAX_OPS_DELAY_NUM*optstsdb_g.uiTotalCh;
                if (pWrOptChStsDbWork >= pOptAiMod->pOptChStsDbEnd)
                {
                    pWrOptChStsDbWork = pWrOptChStsDbWork-optstsdb_g.ulBufLen;
                }

                *pbWrAiValidDbWork=FALSE;
                pWrOptChStsDbWork->lTsse=0x7fffffff;
                pWrOptChStsDbWork->bValid=FALSE;   /*通道采样同步无效  */
                pWrOptChStsDbWork->bComValid=FALSE;   /*通道通信无效 2006-7-28日张云修改 */

                pWrOptChStsDbWork->bComStable=FALSE;   /* 通讯稳定与否 */
                pWrOptChStsDbWork->bDataIsCredible=FALSE;   /* 数据可信与否 2009-3-9 ZY */
                pWrOptChStsDbWork->iRcvSndDiffChgTime=0; /*通道收发时间差是否发生变化  2009-2-13日ZY  */
                pOptAiMod->ulOptRefreshedCnt=0;

            }

        }
        else if (uiWorkFg == uiAllWorkFg) 			/* 有扩展机箱 */
        {
            /* Have Ext-box module. */
            i=aimodDsp_g.ulHeadClk-aimodExt_g.ulHeadClk; 	/* 两者当前采样点之间的差距 */
            if (i >= 0)                   /* DSP module leads to Ext-box module. */
            {
                /* Maximal allowed difference=period/4. */
                if (i<uiAiPts_g/4)
                {
                    /* Let Ext-box be position 0 and adjust position of DSP module. */
                    aimodExt_g.pfWork=aimodExt_g.pfDbBgn;
                    aimodExt_g.pxWork=aimodExt_g.pxDbBgn;
                    aimodExt_g.ulNextCnt=1;
                    aimodExt_g.pbAiValidDbWork=aimodExt_g.pbAiValidDbBgn;
                    *aimodExt_g.pbAiValidDbWork=TRUE;

                    aimodDsp_g.pfWork=aimodDsp_g.pfDbBgn+i*lgcaidb_g.uiTotalCh;
                    aimodDsp_g.pxWork=aimodDsp_g.pxDbBgn+i*calcaidb_g.uiTotalCh;
                    aimodDsp_g.pfDcWork=aimodDsp_g.pfDcDbBgn+i*lgcaidb_g.uiTotalCh;
                    aimodDsp_g.ulNextCnt=i+1; /* 保持超前 */
                    aimodDsp_g.pbAiValidDbWork=aimodDsp_g.pbAiValidDbBgn+i*aivaliddb_g.uiTotalCh;
                    *aimodDsp_g.pbAiValidDbWork=TRUE;
                }
                /* #ifndef EDP_DEBUG */
                else   		/* ulHeadClk near to top. Syn. later. */
                {
                    uiWorkFg &= ~DSP_AI_FG; /* 去掉主机箱 */
                }
                /* #endif */
            }
            else /* 差距绝对值 */
            {
                i=-i;

                /* Maximal allowed difference=period/4. */
                if (i<uiAiPts_g/4)
                {
                    /* Let DSP be position 0 and adjust position of Ext-box module. */
                    aimodDsp_g.pfWork=aimodDsp_g.pfDbBgn;
                    aimodDsp_g.pxWork=aimodDsp_g.pxDbBgn;
                    aimodDsp_g.pfDcWork=aimodDsp_g.pfDcDbBgn;
                    aimodDsp_g.ulNextCnt=1; /* 下一点 */
                    aimodDsp_g.pbAiValidDbWork=aimodDsp_g.pbAiValidDbBgn;
                    *aimodDsp_g.pbAiValidDbWork=TRUE;

                    aimodExt_g.pfWork=aimodExt_g.pfDbBgn+i*lgcaidb_g.uiTotalCh;
                    aimodExt_g.pxWork=aimodExt_g.pxDbBgn+i*calcaidb_g.uiTotalCh;
                    aimodExt_g.ulNextCnt=i+1;
                    aimodExt_g.pbAiValidDbWork=aimodExt_g.pbAiValidDbBgn+i*aivaliddb_g.uiTotalCh;
                    *aimodExt_g.pbAiValidDbWork=TRUE;
                }
                /* #ifndef EDP_DEBUG */
                else                    /* ulHeadClk near to top. Syn. later. */
                {
                    uiWorkFg &= ~EXT_AI_FG; 			/* 去掉扩展机箱 */
                }
                /* #endif */
            }
        }

        if (uiWorkFg==uiAllWorkFg)
        {
            /* Get orignal DI status before booting DSP.
            * Note DI of external box is initialized when communication OK. */
            RD_Get_Org_DI();

            LOG_Dbg_Msg("real-time data engine Init OK.\n", 0, 0, 0, 0, 0, 0);
        }

        /* If any module not working, data is discarded(repeat using the initial
         * last postion in the cycle buffer). */
    }

    *ppxWr=paimod->pxWork;
    if(dcdata != NULL)
    {
        *dcdata=paimod->pfDcWork;
    }

    return paimod->pfWork;
}

#ifdef EDP01_CA_OPT_BUILD		/* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */

/* 报告AI引擎完成一次数据刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void RD_End_Ai_Wr(void *pvAiMod)
{
    uint32_t ulAiCnt;
    RD_REG_FUNC_AI *pregf;

    int iNewPts;
    int i;
    static int iRptSynErr_s;

    ulAiCnt=rdinfo_g.ulCurrAiCnt;

    iNewPts=aimodExt_g.ulNextCnt-1-ulAiCnt;

    if (iNewPts)                        /* All modules write new AI data? */
    {

        /*只有当有第1点采样值刷新后,才设置04初始化成功标志  */
        EP_Set_04CPU_Init_End_Flag(TRUE);

        while (iNewPts--)
        {
#ifdef VIRT_BOX	   /* if define virtual box. */
            readDataSub();
#endif
            RD_Refresh_DI();

            if (++ulAiCnt-rdinfo_g.ulBgnCnt>=rdinfo_g.uiRealBuf)
                rdinfo_g.ulBgnCnt=ulAiCnt;

            rdinfo_g.ulCurrAiCnt=ulAiCnt;
            /* 设置此次系统时刻*/
            RD_Syn_AI_Clk(pvAiMod, TM_Get_usCnt(), rdinfo_g.ulCurrAiCnt);

            /* 扫描任务0驱动DSP */
            if (bDspDrvMod)
            {
                for (pregf=aregf_g; pregf->pfUser; pregf++)
                {
                    if (!--pregf->uiCnt)
                    {
                        pregf->uiCnt=pregf->uiPts;
                        pregf->pfUser(pregf->pvParm);
                    }
                }
            }
        }
    }
}

#else
/***********************************************************************
* RD_End_Ai_Wr - 报告AI引擎完成一次数据刷新
*
* RETURNS: 无
*
*/
void RD_End_Ai_Wr(
    void *pvAiMod		/* pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始化AI通道的时候提供给底层I/O */
)
{
    uint32_t ulAiCnt;
    RD_REG_FUNC_AI *pregf;

    int iNewPts;

    /* #ifndef APP_LINE_SUPPORT */  /* 线路保护不支持扩展机箱 */
    int i;
    static int iRptSynErr_s;
    /* #endif */

    static BOOL bBufFullFlag=FALSE;		/* 数据缓冲区是否满标志 */

    ulAiCnt=rdinfo_g.ulCurrAiCnt; 		/* Ai当前计数, 初始值为-1 */

    iNewPts=aimodDsp_g.ulNextCnt-1-ulAiCnt;		/* ulNextCnt初始值为1 */

    /* #ifndef APP_LINE_SUPPORT */  /* 线路保护不支持扩展机箱 */
    /* 配置处理 */
    if (aimodExt_g.iLgcNum || iExtDiNum_g) 		/* 扩展机箱允许判断 */
    {
        while (EX_Rd_Data() == EP_SUCCESS)
            continue;

        i=aimodExt_g.ulNextCnt-1-ulAiCnt;		/* ulNextCnt初始值为1 */

        if (abs(i-iNewPts)>uiAiPts_g/4 && iRptSynErr_s<2)
        {
            char TempInfo[256];
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "主机箱与扩展机箱采样失步\n", 0, 0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "The main box and the extended box out-of-step\n", 0, 0);
            }
            sprintf(TempInfo, "主机箱与扩展机箱采样失步,从机箱当前采样点数%d,主机箱当前采样点数%d!!\n", i, iNewPts);
            LOG_Write(LOG_KERNEL,TempInfo,NULL);

            iRptSynErr_s++;
        }

        if (i<iNewPts)
            iNewPts=i;
    }
    /* #endif */

    if (iNewPts)                        /* All modules write new AI data? */
    {

        EP_Set_04CPU_Init_End_Flag(TRUE);		/*只有当有第1点采样值刷新后,才设置04初始化成功标志  */

        while (iNewPts--)
        {
#ifdef VIRT_BOX	   /* if define virtual box. */
            readDataSub();
#endif
            /* 带扩展机箱时一并刷新
             */
            if (bDspDrvMod)
            {
                HDL_Refresh_DI(NULL);
            }

            RD_Refresh_DI();		/* 刷新DI */

            if (++ulAiCnt-rdinfo_g.ulBgnCnt >= rdinfo_g.uiRealBuf)		/* 计数满10+1周波，更换起始点*/
            {
                /* 缓冲区满 */
                rdinfo_g.ulBgnCnt=ulAiCnt;
                bBufFullFlag=TRUE;
            }
            rdinfo_g.ulCurrAiCnt=ulAiCnt;		/* 从第0点开始 */

#if defined(EDP03_LOWPROTECT_BUILD) || defined(EDP03_STABCONTROL_BUILD)
            if(bBufFullFlag)
            {
                /* 数据缓冲区满后才驱动逻辑图 */
#endif

                /* 扫描任务0驱动DSP */
                if (bDspDrvMod)
                {
                    for (pregf=aregf_g; pregf->pfUser; pregf++)
                    {
                        if (!--pregf->uiCnt)
                        {
                            pregf->uiCnt=pregf->uiPts;
                            pregf->pfUser(pregf->pvParm);		/* 驱动逻辑图 */
                        }
                    }
                }

#if defined(EDP03_LOWPROTECT_BUILD) || defined(EDP03_STABCONTROL_BUILD)
            }
#endif

        }
    }
}
#endif

/***********************************************************************
* RD_Get_Org_DI - 获得DI通道某个时间点的值
*
* RETURNS: 无
*
*/
void RD_Get_Org_DI(void)
{
    RD_LGC_DI_CH *plgcdi;
    uint32_t ulOrgTime32;
    US_CNT_UTC_TIME OrgTimeUsUtc;
    BOOL bFilterSts;

    /*2013-5-23  ZY */
    TM_High_Get_Sys_Us_UTC_Time(&OrgTimeUsUtc, &ulOrgTime32);

    /* 配置通道 */
    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        plgcdi->ulChgTime = ulOrgTime32;

        /* 是否被控标志 */
        if (plgcdi->iForceSts!=-1)
            plgcdi->iVal = 0x8000 | plgcdi->iForceSts; /* 为被控值 */
        else if (plgcdi->mod==RD_SPI_DI)
        {
            plgcdi->iVal=SIO_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgTime, &plgcdi->utChgTime);
        }
        else if (plgcdi->mod==RD_EXT_DI)
        {
            plgcdi->iVal=EX_Get_DI(plgcdi->pvSrc);
        }

#ifdef EXCITE_BUILD
        else if (plgcdi->mod==RD_REDUN_DI)
        {
            plgcdi->iVal=Redun_Get_DI(plgcdi->pvSrc);
        }
#endif

        else if ((plgcdi->mod==RD_OPT1_DI)||(plgcdi->mod==RD_OPT2_DI))
        {
            plgcdi->iVal=OPT_Get_DI(plgcdi->pvSrc);
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        else if (plgcdi->mod==RD_SAME_POLE_DI)
        {
            /*2007-3-29 张云  */
            plgcdi->iVal=POLE_Get_DI(plgcdi->pvSrc);
        }
        else if (plgcdi->mod==RD_HDL_BOX_DI)
        {
            /*2007-3-29 张云  */

            /* plgcdi->iVal = Hdl_Filt_And_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgNextCnt, &(plgcdi->usQuality)); */
            plgcdi->iVal = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                           &(plgcdi->usQuality),
                                           &bFilterSts);

            /*if((((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime.ucQflag&0x60)==0)*/
            if(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime.ullusCntFrom1970!=0)
            {
                BOOL bTransOK=TRUE;
                plgcdi->utChgTime = ((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime;
                plgcdi->ulChgTime = TM_High_Us_UTC_Time_To_us32Cnt(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime, &bTransOK);
                if(!bTransOK)
                    plgcdi->ulChgTime = ulOrgTime32;
            }
        }
#endif
        else if (plgcdi->mod == RD_VT_BOX_DI)
        {
            /* virtual box DI. */
            assert (FALSE);
        }
        else
            assert(FALSE);
    }
}

/***********************************************************************
* RD_Refresh_DI - 更新DI
*
* RETURNS: 无
*
*/
static void RD_Refresh_DI(void)
{
    BOOL *pb;
    BOOL  bCurAiIsFast=FALSE;
    BOOL  bCurAiIsMid=FALSE;
    BOOL  bCurAiIsSlow=FALSE;
    BOOL  bCurAiISFasterThanFast=FALSE;
    static   uint32_t   ulRefreshCnt=0;

    ulRefreshCnt++;

    if((ulRefreshCnt%DI_SLOW_REFRESH_INTERVAL)==1)
    {
        bCurAiIsSlow=TRUE;
    }
    if((ulRefreshCnt%DI_MID_REFRESH_INTERVAL)==1)
    {
        bCurAiIsMid=TRUE;
    }
    if(((ulRefreshCnt%DI_FAST_REFRESH_INTERVAL)==1)
            ||(DI_FAST_REFRESH_INTERVAL==1))
    {
        bCurAiIsFast=TRUE;
    }
    if((usMinInterval<DI_FAST_REFRESH_INTERVAL) && (usMinInterval>=1)
            && (uiAppType_g != APP_BUS))
    {
        bCurAiISFasterThanFast=TRUE;
    }

    if(bCurAiIsFast||bCurAiIsMid||bCurAiIsSlow||bCurAiISFasterThanFast) /* 判断有需要刷新的 */
    {
        /* 若需要刷新 */
        BOOL  *pBaseWork;
        BOOL  *pbFirst;
        BOOL *pbSecond;
        BOOL *pbThird;
        BOOL *pbForth;
        BOOL *pbFifth;
        BOOL *pbSixth;
        BOOL *pbSeventh;
        BOOL *pbEighth;
        BOOL *pbNinth;
        BOOL *pbTenth;
        BOOL *pbEleventh;
        BOOL *pbTwelvth;
        RD_LGC_DI_CH *plgcdi;
        int  i;
        int iNow;

        if(bCurAiIsSlow)
        {
            /*若是慢速，则获得相应的多次存储的基址  */
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh; /* 向前推一个点 */
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork; /* 第一点 */
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSeventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEighth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbNinth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTenth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEleventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTwelvth=pBaseWork; /* 12点 */

        }
        else  if(bCurAiIsMid)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork; /* 6点 */

        }
        else if(bCurAiIsFast)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork; 					/* 2点 */
        }
        else if(bCurAiISFasterThanFast)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
        }
        else
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork; 					/* 2点 */
        }

        for (plgcdi=plgcdich_g,i=0; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++,i++)
        {
            /*对每个ＤＩ通道判定，是否需要刷新  */

            /* GOOSE DI不在此处更新 */
            if (plgcdi->mod == RD_HDL_BOX_DI)
            {
                continue;
            }

            if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiISFasterThanFast)
            {
                if (plgcdi->iForceSts!=-1)
                {
                    iNow=plgcdi->iForceSts; /* 控制结果 */
                    *(pbFirst+i)=iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);			/* Every kind of DI wil be refresh independently, DY 9/15/2007 */
                }
                else  if (plgcdi->mod==RD_EXT_DI) /* 外部 */
                {
                    iNow=EX_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_SPI_DI) /* 内部 */
                {
                    iNow=SIO_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgTime, &plgcdi->utChgTime);
                    *(pbFirst+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_REDUN_DI) /* 内部 */		/* for excite */
                {
                    iNow=Redun_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else
                {
                }
            }
            else if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiIsFast) /* 需要刷新，并且现在是刷新时刻 */
            {
                /*若是快速通道，且此次是快速刷新节拍，则快速刷新３点  */
                if (plgcdi->iForceSts!=-1)
                {
                    iNow=plgcdi->iForceSts; /* 控制结果 */
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);			/* Every kind of DI wil be refresh independently, DY 9/15/2007 */
                }
                else  if (plgcdi->mod==RD_EXT_DI) /* 外部 */
                {
                    iNow=EX_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_SPI_DI) /* 内部 */
                {
                    iNow=SIO_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgTime, &plgcdi->utChgTime);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_REDUN_DI) /* 内部 */		/* for excite */
                {
                    iNow=Redun_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else
                {
                }
            }
            else  if(bCurAiIsMid
                     &&plgcdi->ucDIRefreshRate==DI_MID_REFRESH_RATE)
            {
                /*若是中速通道，且此次是中速刷新节拍，则中速刷新６点  */

                if (plgcdi->iForceSts!=-1)
                {
                    iNow=plgcdi->iForceSts;
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else  if (plgcdi->mod==RD_EXT_DI)
                {
                    iNow=EX_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_SPI_DI)
                {
                    iNow=SIO_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgTime, &plgcdi->utChgTime);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_REDUN_DI) /* 内部 */		/* for excite */
                {
                    iNow=Redun_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else
                {
                }
            }
            else if(bCurAiIsSlow
                    &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)
            {
                /*若是慢速通道，且此次是慢速刷新节拍，则慢速刷新１２点，若是其他，则对该通道空操作  */
                if (plgcdi->iForceSts!=-1)
                {
                    iNow=plgcdi->iForceSts;
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;

                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else  if (plgcdi->mod==RD_EXT_DI)
                {
                    iNow=EX_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_SPI_DI)
                {
                    iNow=SIO_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgTime, &plgcdi->utChgTime);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else if (plgcdi->mod==RD_REDUN_DI) /* 内部 */		/* for excite */
                {
                    iNow=Redun_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;
                    RD_Modify_DI(plgcdi,iNow);
                }
                else
                {
                }

            }/*else if(bCurAiIsSlow
        &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)结束  */
        }/*for循环结束  */

    }/*if结束 */

    /*更新最新位置 */
    pb=didb_g.pbWork+didb_g.uiTotalCh;
    if (pb>=didb_g.pbBufEnd)
        pb=didb_g.pbBufBgn;
    didb_g.pbWork=pb;
    return;
}

/***********************************************************************
* RD_Modify_DI - 写入开入板状态，处理变位等信息
*
* RETURNS: 无
*
*/
void RD_Modify_DI(
    RD_LGC_DI_CH *plgcdi,		/* DI配置 */
    int  iNowVal		/* 配置值 */
)
{
    /*设置开入摸件的32位开入值中的相关位  */
    if(!(iNowVal & 0x7FFF))
    {
        /*若为0  */
        plgcdi->p_part->ulDIModCurVaule &= (~(ulDwordBitArr_g[plgcdi->ucModCh]));
    }
    else
    {
        /*若为1  */
        plgcdi->p_part->ulDIModCurVaule |= (ulDwordBitArr_g[plgcdi->ucModCh]);
    }

    /* 品质位 */
    if (iNowVal&0x8000)
    {
        plgcdi->usQuality |= DI_FORCE_STS; /* 原先强制变位则置为检修, 后用专门的状态标 */
    }
    else
    {
        plgcdi->usQuality &= (~DI_FORCE_STS);
    }

    if (plgcdi->iVal != iNowVal)
    {
        /* 有变位 */
        RE_SetLogDIUpdateCnt();

        if((plgcdi->iVal & 0x7FFF) != (iNowVal & 0x7FFF))
        {
            /* Report SOE. */
            if ((plgcdi->iVal | iNowVal) & 0x8000)		/* 强制 */
            {
                /*2013-5-23  ZY */
                TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                //logMsg("Di index=%d, time Q=%X\n",(int)(plgcdich_g-plgcdi),plgcdi->utChgTime.ucQflag,0,0,0,0);
                if (plgcdi->iMeaCh != -1)
                    VI_New_SOE(plgcdi->iMeaCh, (iNowVal & 0x7FFF), plgcdi->ulChgTime, plgcdi->bSOE,plgcdi->usQuality);
            }
            else	/* 非强制 */
            {
#ifdef EDP03_BUILD
                SIOGetDIChangeTime(plgcdi->pvSrc, &plgcdi->ulChgTimeAfterFilt);
#endif
                if (plgcdi->mod==RD_EXT_DI)
                {
                    plgcdi->ulChgTime=RD_AI_Cnt_To_us(
                                          aimodExt_g.ulNextCnt-1);
                    plgcdi->ulChgTimeAfterFilt=plgcdi->ulChgTime;		/* 增加滤波后 */
                }
                if (plgcdi->mod==RD_REDUN_DI) /* for excite */
                {
                    plgcdi->ulChgTime=RD_AI_Cnt_To_us(
                                          aimodRedun_g.ulNextCnt-1);
                    plgcdi->ulChgTimeAfterFilt=plgcdi->ulChgTime;		/* 增加滤波后 */
                }

#ifdef EDP03_BUILD		/* EDP03平台使用滤波后的时间 */
                if((!plgcdi->ReserveAttribute)&&(plgcdi->iMeaCh != -1))		/* 保护使用才上传SOE */
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, plgcdi->usQuality);				/* 滤波之前的变位时间 */
                /* VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTimeAfterFilt, plgcdi->bSOE); */ 				/* 测试，滤波之后的变位时间 */
#else		/* 其它平台使用滤波前的时间 */
                if((!plgcdi->ReserveAttribute)&&(plgcdi->iMeaCh != -1))		/* 保护使用才上传SOE */
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, plgcdi->usQuality); 		/* 滤波之前的变位时间 */
#endif
                /* VI_DI_Change(plgcdi, iNowVal, plgcdi->ulChgTimeAfterFilt); */			/* 暂时不用DY 7/24/2007 */
            }
        }
        plgcdi->iVal=iNowVal;
    }
}

/* Get hardware AI channel attribution.
 * Parameters:
 *      iIdx, index of the hardware AI(from 0).
 * Return value:
 *      Pointer to the hardware AI attribution structure.
 *      NULL if iIdx is invalid(>=iHwAiChNum_g). */
const RD_HW_AI_CH *RD_Get_Hw_AI_Attr(int iIdx)
{
    if (iIdx<iHwAiChNum_g)
        return phwaich_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get DI channel attribution.
 * Parameters:
 *      iIdx, index of the DI(from 0).
 * Return value:
 *      Pointer to the DI attribution structure.
 *      NULL if iIdx is invalid(>=iLgcDiNum_g). */
const RD_LGC_DI_CH *RD_Get_DI_Attr(int iIdx)
{
    if (iIdx<iLgcDiChNum_g)
        return plgcdich_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get DO channel attribution.
 * Parameters:
 *      iIdx, index of the DO(from 0).
 * Return value:
 *      Pointer to the DO attribution structure.
 *      NULL if iIdx is invalid(>=iLgcDoChNum_g). */
const RD_LGC_DO_CH *RD_Get_DO_Attr(int iIdx)
{
    if (iIdx<iLgcDoChNum_g)
        return plgcdoch_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get hardware LED channel attribution.
 * Parameters:
 *      iIdx, index of the hardware LED(from 0).
 * Return value:
 *      Pointer to the LED attribution structure.
 *      NULL if iIdx is invalid(>=iHwLedChNum_g). */
const RD_LGC_LED_CH *RD_Get_Hw_Led_Attr(int iIdx)
{
    if (iIdx<iHwLedChNum_g)
        return plgcledch_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get software LED channel attribution.
 * Parameters:
 *      iIdx, index of the software LED(from 0).
 * Return value:
 *      Pointer to the LED attribution structure.
 *      NULL if iIdx is invalid(>=iLgcLedNum_g-iHwLedChNum_g). */
const RD_LGC_LED_CH *RD_Get_Sw_Led_Attr(int iIdx)
{
    if (iIdx<iLgcLedChNum_g-iHwLedChNum_g)
        return plgcledch_g+iHwLedChNum_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Read all hardware LEDs' value.
 * Parameters:
 *      pbRslt, to save all hardware LEDs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pbRslt must contains space to save iHwLedChNum_g BOOL numbers. */
void RD_Rd_Hw_Led_Val(BOOL *pbRslt)
{
    RD_LGC_LED_CH *plgcled;
    STATUS vxsts;

    vxsts=taskLock();
    assert(vxsts==OK);

    for (plgcled=plgcledch_g; plgcled<plgcledch_g+iHwLedChNum_g; plgcled++)
        *pbRslt++=plgcled->bSts;

    taskUnlock();
    assert(vxsts==OK);
}

/* Read all software LEDs' value.
 * Parameters:
 *      pbRslt, to save all software LEDs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pbRslt must contains space to save iSwLedChNum_g BOOL numbers. */
void RD_Rd_Sw_Led_Val(BOOL *pbRslt)
{
    RD_LGC_LED_CH *plgcled;
    STATUS vxsts;

    vxsts=taskLock();
    assert(vxsts==OK);

    for (plgcled=plgcledch_g+iHwLedChNum_g;
            plgcled<plgcledch_g+iLgcLedChNum_g; plgcled++)
        *pbRslt++=plgcled->bSts;

    taskUnlock();
    assert(vxsts==OK);
}

/* 检查测量AI对应的实时AI数据索引
 * 参数：   strMeaAiId，遥测逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Mea_AI_Hnd(uint8_t *strMeaAiId)
{
    RD_LGC_AI_CH *plgcai;

    assert(strMeaAiId && strlen(strMeaAiId)<=MAX_ID_LEN);

    for (plgcai=plgcaich_g; plgcai<plgcaich_g+iLgcAiChNum_g; plgcai++)
    {
        if (plgcai->bMea && !strcmp(strMeaAiId, plgcai->aucMeaId))
            return plgcai;
    }

    return NULL;
}

/* 检查测量DI对应的实时DI数据索引
 * 参数：   strMeaDiId，遥信逻辑标识字符串
 *          iMeaCh，遥信通道号
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Mea_DI_Hnd(uint8_t *strMeaDiId, int iMeaCh, BOOL  bSOE)
{
    RD_LGC_DI_CH *plgcdi;

    assert(strMeaDiId && strlen(strMeaDiId)<=MAX_ID_LEN && iMeaCh>=0);

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->bMea && !strcmp(strMeaDiId, plgcdi->aucMeaId))
        {
            plgcdi->iMeaCh=iMeaCh;
            plgcdi->bSOE=bSOE;
            return plgcdi;
        }
    }

    return NULL;
}

/* 检查录波AI对应的实时AI数据索引
 * 参数：   strRecAiId，录波AI逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Rec_AI_Hnd(uint8_t *strRecAiId)
{
    RD_LGC_AI_CH *plgcai;

    assert(strRecAiId && strlen(strRecAiId)<=MAX_ID_LEN);

    for (plgcai=plgcaich_g; plgcai<plgcaich_g+iLgcAiChNum_g; plgcai++)
    {
        if (plgcai->bRec && !strcmp(strRecAiId, plgcai->aucRecId))
            return plgcai;
    }

    return NULL;
}

/* 检查录波DI对应的实时DI数据索引
 * 参数：   strRecDiId，录波DI逻辑标识字符串
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Rec_DI_Hnd(uint8_t *strRecDiId)
{
    RD_LGC_DI_CH *plgcdi;

    assert(strRecDiId && strlen(strRecDiId)<=MAX_ID_LEN);

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->bRec && !strcmp(strRecDiId, plgcdi->aucRecId))
            return plgcdi;
    }

    return NULL;
}

/* 检查标志量AI对应的实时AI数据索引
 * 参数：   strFlagAiId，标志量AI逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Flag_AI_Hnd(uint8_t *strFlagAiId)
{
    RD_LGC_AI_CH *plgcai;

    assert(strFlagAiId && strlen(strFlagAiId)<=MAX_ID_LEN);

    for (plgcai=plgcaich_g; plgcai<plgcaich_g+iLgcAiChNum_g; plgcai++)
    {
        if (plgcai->bFlag && !strcmp(strFlagAiId, plgcai->aucFlagId))
            return plgcai;
    }

    return NULL;
}

/* 检查标志量DI对应的实时DI数据索引
 * 参数：   strFlagDiId，标志量DI逻辑标识字符串
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Flag_DI_Hnd(uint8_t *strFlagDiId)
{
    RD_LGC_DI_CH *plgcdi;

    assert(strFlagDiId && strlen(strFlagDiId)<=MAX_ID_LEN);

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->bFlag && !strcmp(strFlagDiId, plgcdi->aucFlagId))
            return plgcdi;
    }

    return NULL;
}

/***********************************************************************
* RD_Mea_Hw_AI - Read all hardware AI measurment value
*
* RETURNS: None
*
* Alert:
*        phwmeaRslt must contains space to save iHwAiChNum_g members.
*/
void RD_Mea_Hw_AI(
    RD_HW_AI_MEA *phwmeaRslt,		/* to save all hardware AI measurement value */
    BOOL bIsCalc
)
{
    static float *pfSinTbl;
    static RD_HW_AI_MEA *phwmeaCalc = NULL;
    static uint32_t ulCalcTime;
    float fSmplAng;
    float fMaxSmpl;
    float fDeltaAng;
    RD_HW_AI_MEA *phwmea;
    RD_HW_AI_CH *phwai; 				/* 硬件通道配置结构 */
    float *pfBase;
    float *pf;
    float *pfSin;
    float *pfCos;
    float fReal;
    float fImage;
    float fDC;
    float f1, f2, f3, f4;
    int i;
    STATUS vxsts;
    BOOL bFindValidChFlag=FALSE;

    vxsts=semTake(semHwAiMea, WAIT_FOREVER); 		/* 读取物理通道标志 */
    assert(vxsts == OK);

    if (!phwmeaCalc)
    {
        if ((pfSinTbl=calloc(uiAiPts_g/4+1, sizeof(*pfSinTbl))) == NULL) 			/* 申请1/4多一点数据 */
            goto ret;

        if ((phwmeaCalc=calloc(iHwAiChNum_g, sizeof(*phwmeaCalc))) == NULL) 			/* 申请的物理通道总数 */
        {
            EP_free(pfSinTbl);
            goto ret;
        }

        assert(uiAiPts_g%4==0); 		/* 每周波采样点数为4的倍数 */
        for (i=0; i<=uiAiPts_g/4; i++)
        {
#if defined(EDP_01_02_BUILD)
            pfSinTbl[i]=sin(2*M_PI*i/uiAiPts_g)*(1.0*2.0/M_SQRT2)/uiAiPts_g;
#else	/* 模拟采样，考虑滤波衰减 */
            if (appType_g == APP_TYPE_DIG)
            {
                /* 数字化应用 */
                pfSinTbl[i]=sin(2*M_PI*i/uiAiPts_g)*(1.0*2.0/M_SQRT2)/uiAiPts_g;
            }
            else if (appType_g == APP_TYPE_TRAD)
            {
                /* 传统应用 */
                pfSinTbl[i]=sin(2*M_PI*i/uiAiPts_g)*(1.026482*2.0/M_SQRT2)/uiAiPts_g;
            }
#endif
        }
        /* M_SQRT2为sqrt(2) */
        /* 以前是1.0271，现改为1.013731，改为1.026482 DY 12/13/2006 */
        /* 1.0271为RC二阶滤波基波增益系数的倒数 */

        for (phwmea=phwmeaCalc, phwai=phwaich_g;
                phwmea<phwmeaCalc+iHwAiChNum_g; phwmea++, phwai++)
        {
            phwmea->ucType=phwai->ucUnit; 			/* 单位 */
            phwmea->fGain=phwai->fGain; 										/* 增益 */
        }
    }
    else if (TM_Get_usCnt()-ulCalcTime<500000L) 					/* 判断时间间距 */
        goto saverslt;

    assert(phwmeaCalc && pfSinTbl);

    vxsts=taskLock();
    /* 变比系数可能会更新，需要实时修改 */
    for (phwmea=phwmeaCalc, phwai=phwaich_g;
            phwmea<phwmeaCalc+iHwAiChNum_g; phwmea++, phwai++)
    {
        phwmea->fGain=phwai->fGain; 										/* 增益 */
    }
    vxsts=taskUnlock();

    fMaxSmpl=0.0;
    fDeltaAng=0.0;
    pfBase=RD_Base_Lgc_AI_P(RD_AI_Cnt()); 		/* 获得当前逻辑通道计算结果基准地址 */


    for (phwmea=phwmeaCalc, phwai=phwaich_g;
            phwmea<phwmeaCalc+iHwAiChNum_g; phwmea++, phwai++)  		/* 配置通道和读取通道配合在一起 */
    {
        if (phwai->iSmplCh==-1
                ||(phwai->paimod==(&(aimodOpt_g[0])))
                ||(phwai->paimod==(&(aimodOpt_g[1])))
           ) 		/* 检测是否通过逻辑通道进行了计算，没有计算则退出 */
        {
            phwmea->fMean=0;
            phwmea->fRmsVal=0;
            phwmea->fAngle=1000.0;      /* As angle invalid flag. */

            continue;
        }

        /* 判断是否需要计算, 满足压板退出时部分通道计算的要求 */
        if (!bIsCalc)
        {
            if (phwai->bCalcMeaFlag)
            {
                phwai->bCalcMeaFlag = FALSE;
            }
            else
            {
                continue;
            }
        }

        pf=pfBase+phwai->iSmplCh; 			/* 指向采样通道 */

        fDC=RD_Lgc_AI(pf, 0)
            +RD_Lgc_AI(pf, -(int)uiAiPts_g/4)
            +RD_Lgc_AI(pf, -(int)uiAiPts_g/2)
            +RD_Lgc_AI(pf, -(int)(3*uiAiPts_g/4));

        pfCos=pfSinTbl+uiAiPts_g/4; 	/* 指向最后一个 */
        fReal=*pfCos*(RD_Lgc_AI(pf, 0)-RD_Lgc_AI(pf, -(int)uiAiPts_g/2)); 			/* 实部 */
        fImage=-*pfCos*
               (RD_Lgc_AI(pf, -3*(int)uiAiPts_g/4)-RD_Lgc_AI(pf, -(int)uiAiPts_g/4)); 		/* 虚部 */
        pfCos--;
        for (i=1, pfSin=pfSinTbl+1; i<uiAiPts_g/4; i++, pfSin++, pfCos--)
        {
            f1=RD_Lgc_AI(pf, -(int)uiAiPts_g+i); /* 一周波前加 */
            f2=RD_Lgc_AI(pf, -(int)uiAiPts_g/2-i);
            f3=RD_Lgc_AI(pf, -(int)uiAiPts_g/2+i);
            f4=RD_Lgc_AI(pf, -i);

            fDC+=f1+f2+f3+f4;

            fReal+=*pfCos*(f1-f2-f3+f4);					/* 求实部 */
            fImage-=*pfSin*(f1+f2-f3-f4);		/* 求虚部 */
        }

        phwmea->fMean=fDC/uiAiPts_g;

        fSmplAng=atan2(fImage, fReal)*(180.0/M_PI)+180.0; 				/* 相角 */
        phwmea->fRmsVal=sqrt(fReal*fReal+fImage*fImage); 				/* 幅值 */

        if (phwmea->fRmsVal>phwai->fThreshold)				/* Valid input(>threshold)? */
        {
            phwmea->fAngle=fSmplAng;
            if(!bFindValidChFlag)
            {
                fDeltaAng=0.01-fSmplAng;		/* 张云修改过.选第1路有效的结果为基准通道 */
                bFindValidChFlag=TRUE;
            }
        }
        else
            phwmea->fAngle=1000.0;      /* As angle invalid flag. */


        /* 转换为一次值 */
        if (bFstOrSecFlag)
        {
            if (appType_g == APP_TYPE_TRAD)
            {
                phwmea->fMean *= phwai->fTradSecToFstCoff;
                phwmea->fRmsVal *= phwai->fTradSecToFstCoff;
            }
            else
            {
                phwmea->fMean *= phwai->fSecToFstCoff;
                phwmea->fRmsVal *= phwai->fSecToFstCoff;
            }
        }
    }

    for (phwmea=phwmeaCalc; phwmea<phwmeaCalc+iHwAiChNum_g; phwmea++)
    {
        if (phwmea->fAngle>500.0)       /* Check angle invalid flag. */
            phwmea->fAngle=0;
        else
        {
            phwmea->fAngle+=fDeltaAng; 		/* 角度测试 */

            if (phwmea->fAngle>360.0)
                phwmea->fAngle-=360.0;
            else if (phwmea->fAngle<0.0)
                phwmea->fAngle+=360.0;

            assert(phwmea->fAngle>(0.0-FLT_PRECISION) &&
                   phwmea->fAngle<(360.0+FLT_PRECISION)); 			/* Modifeid by DY 4/11/2006 */
        }
    }

    ulCalcTime=TM_Get_usCnt();

saverslt:
    memcpy(phwmeaRslt, phwmeaCalc, sizeof(*phwmeaRslt)*iHwAiChNum_g); /* 拷贝到目标空间 */

ret:
    vxsts=semGive(semHwAiMea);
    assert(vxsts==OK);
}

/***********************************************************************
* RD_Mea_Po - Read all pulse output measurment value
*
* RETURNS: None
*
* Alert:
*        ppomeaRslt must contains space to save iLgcPoChNum_g members.
*/
void RD_Mea_Po(
    RD_PO_MEA *ppomeaRslt		/* to save all pulse output measurement value */
)
{
    RD_LGC_PO_CH *pch;
    STATUS vxsts;

    assert(ppomeaRslt);
    vxsts=semTake(semPoMea, WAIT_FOREVER); /* 读取标志 */
    assert(vxsts == OK);

    for (pch=plgcpoch_g; pch<plgcpoch_g+iLgcPoChNum_g; pch++)
    {
        ppomeaRslt->fVal=pch->Val.TotalEnergy;
        ppomeaRslt->ucType=pch->ucType;
        ppomeaRslt++;
    }

    vxsts=semGive(semPoMea);
    assert(vxsts == OK);
}

/* Change force DI status.
 * Parameters:
 *      iIdx, index of DI(from 0).
 *      iSts, new status: TRUE, FALSE or -1 means release force.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS RD_Chg_Force_DI(int iIdx, int iSts)
{
    RD_LGC_DI_CH *pdich;
    EP_STATUS sts;
    uint8_t *pucVal;
#if 0
    uint16_t unComCrc;
#endif

    assert(iIdx>=0 && iIdx<iLgcDiChNum_g);
    assert(iSts==TRUE || iSts==FALSE || iSts==-1);

    pdich=plgcdich_g+iIdx;

    if (pdich->iForceSts==iSts)
        return EP_SUCCESS;

    /* 仅处理操作箱DI, 同时悬空或裁剪后不处理 */
    if ((pdich->mod == RD_HDL_BOX_DI)
            && pdich->bPended
            && (uiAppType_g != APP_BUS)
            && (iSts != -1))
    {
        return EP_ERROR;
    }

    if (iSts==-1)
        pucVal=DI_NORMAL;
    else if (iSts)
        pucVal=DI_CLOSE;
    else
        pucVal=DI_OPEN;

    sts=SI_Chg_Link_Sts_File_Item(EP_DI_STS_FILE, pucDiFileHead_g,
                                  iLgcDiChNum_g, iIdx, pucVal);

    if (sts!=EP_SUCCESS)
        return EP_FILE_ERR;
    else
    {
#if 0
        unComCrc =0;
        unComCrc=FT_File_CRC16(EP_DI_STS_FILE);
        FT_Wr_INI_CRC("[CRC]",CRC_ITEM_DIFDORCE,unComCrc);
#endif

        pdich->iForceSts=iSts;
        return EP_SUCCESS;
    }
}

/* 多个开入一起强制.
 * Parameters:
 *     pRcvDiBuf, 报文指针.
 * Return value:
 *     EP_SUCCESS, force DI OK.
 *     EP_ERROR, failure else.
 *     EP_FILE_ERR, file operating failure.
 */
EP_STATUS RD_Chg_Force_Multi_Di(uint8_t *pRcvDiBuf)
{
    uint8_t ucDiNum = 0;
    uint8_t ucDisn = 0;
    uint8_t ucDiVal = 0;
    int32_t i = 0;
    RD_LGC_DI_CH *pdich = NULL;
    int32_t iSts = 0;
    uint8_t *pucVal = NULL;
    STATUS sts;
    uint8_t *pucBuf;
    uint32_t ulLen;
    int32_t iFd;
    uint16_t usCrc = 0;
    uint8_t aucLine[ITEM_LEN];
    uint8_t aucTmp[ITEM_LEN];

    ucDiNum = pRcvDiBuf[22];
    if (ucDiNum > iLgcDiChNum_g)
    {
        return EP_ERROR;
    }

    /* 任意不匹配则返回出错
     */
    for(i = 0; i < ucDiNum; i++)
    {
        ucDisn = pRcvDiBuf[23+i*2];
        ucDiVal = pRcvDiBuf[24+i*2];
        if (ucDisn >= iLgcDiChNum_g)
        {
            return EP_ERROR;
        }
        else if ((ucDiVal != 0x55) && (ucDiVal != 0xAA) && (ucDiVal != 0x5A))
        {
            return EP_ERROR;
        }

        pdich = plgcdich_g+ucDisn;

        if (ucDiVal == 0x55)
        {
            iSts = 1;
            pucVal = DI_CLOSE;
        }
        else if (ucDiVal == 0xAA)
        {
            iSts = 0;
            pucVal = DI_OPEN;
        }
        else if (ucDiVal == 0x5A)
        {
            iSts = -1;
            pucVal = DI_NORMAL;
        }

        /* 仅处理操作箱DI, 同时悬空或裁剪后不处理
         */
        if ((pdich->mod == RD_HDL_BOX_DI)
                && pdich->bPended
                && (uiAppType_g != APP_BUS)
                && (iSts != -1))
        {
            return EP_ERROR;
        }
    }

    /* 检测都通过, 开始切换操作, 先写文件
     */
    if ((pucBuf = FT_File_To_Mem(EP_DI_STS_FILE, &ulLen)) == NULL)
    {
        return EP_FILE_ERR;
    }

    /* 增加CRC */
    if (ulLen != ITEM_LEN+iLgcDiChNum_g*ITEM_LEN+ITEM_LEN
            || memcmp(pucBuf, pucDiFileHead_g, ITEM_LEN) ||
            (iFd = FT_Bgn_Update(EP_DI_STS_FILE))<0)
    {
        EP_free(pucBuf);

        return EP_FILE_ERR;
    }

    for(i = 0; i < ucDiNum; i++)
    {
        ucDisn = pRcvDiBuf[23+i*2];
        ucDiVal = pRcvDiBuf[24+i*2];

        if (ucDiVal == 0x55)
        {
            iSts = 1;
            pucVal = DI_CLOSE;
        }
        else if (ucDiVal == 0xAA)
        {
            iSts = 0;
            pucVal = DI_OPEN;
        }
        else if (ucDiVal == 0x5A)
        {
            iSts = -1;
            pucVal = DI_NORMAL;
        }

        pdich = plgcdich_g+ucDisn;

        if (pdich->iForceSts == iSts)
        {
            continue;
        }

        memcpy(pucBuf+ITEM_LEN+ucDisn*ITEM_LEN+ITEM_NAME_LEN,
               pucVal, ITEM_VALUE_LEN-1);
    }

    /* 计算并写入CRC
     */
    usCrc = EP_CCITT_CRC16(pucBuf, ITEM_LEN+iLgcDiChNum_g*ITEM_LEN, usCrc);

    sprintf(aucTmp, "%04x", usCrc);
    SI_Tag_Str_Cpy(aucLine, aucTmp, ITEM_VALUE_LEN);

    memcpy(pucBuf+ITEM_LEN+iLgcDiChNum_g*ITEM_LEN+ITEM_NAME_LEN,
           aucLine, ITEM_VALUE_LEN);

    i = write(iFd, pucBuf, ulLen);
    if(!(i == ulLen))
    {
        EP_free(pucBuf);
        FT_Abort_Update(EP_DI_STS_FILE, iFd);

        return EP_FILE_ERR;
    }

    EP_free(pucBuf);

    sts = FT_End_Update(EP_DI_STS_FILE, iFd);
    if (!(sts == EP_SUCCESS))
    {
        return EP_FILE_ERR;
    }

    /* 文件写成功了在切换内存的值
     */
    for(i = 0; i < ucDiNum; i++)
    {
        ucDisn = pRcvDiBuf[23+i*2];
        ucDiVal = pRcvDiBuf[24+i*2];

        if (ucDiVal == 0x55)
        {
            iSts = 1;
            pucVal = DI_CLOSE;
        }
        else if (ucDiVal == 0xaa)
        {
            iSts = 0;
            pucVal = DI_OPEN;
        }
        else if (ucDiVal == 0x5a)
        {
            iSts = -1;
            pucVal = DI_NORMAL;
        }

        pdich = plgcdich_g+ucDisn;
        pdich->iForceSts = iSts;
    }

    return EP_SUCCESS;
}

/* Read force DI information from setting file.
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, read file OK.
 *      EP_FILE_ERR, file format error. */
EP_STATUS RD_Rd_Force_DI(void)
{
    uint8_t *pucBuf;
    uint32_t ulLen;
    uint8_t *puc;
    uint8_t aucTemp[ITEM_NAME_LEN];
    EP_STATUS stsRet;
    RD_LGC_DI_CH *pdich;
    uint16_t usCrc = 0;
    uint16_t usCalcCrc = 0;

    if ((pucBuf=FT_File_To_Mem(EP_DI_STS_FILE, &ulLen))==NULL)
    {
        LOG_Write(LOG_KERNEL, "开入强制文件读取出错.\n", NULL);
        return EP_FILE_ERR;
    }

    /* 添加CRC */
    if ((ulLen != ITEM_LEN*(iLgcDiChNum_g+2)) || memcmp(pucBuf, pucDiFileHead_g, ITEM_LEN))
    {
        LOG_Write(LOG_KERNEL, "开入强制文件格式出错.\n", NULL);

        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    /* CRC项自身不计算 */
    usCalcCrc = EP_CCITT_CRC16(pucBuf, ITEM_LEN*(iLgcDiChNum_g+1), usCalcCrc);

    stsRet=EP_SUCCESS;

    puc=pucBuf+ITEM_LEN;

    for (pdich=plgcdich_g; pdich<plgcdich_g+iLgcDiChNum_g; pdich++)
    {
        SI_Tag_Str_Cpy(aucTemp, pdich->aucABRV, ITEM_NAME_LEN);
        if (!memcmp(puc, aucTemp, ITEM_NAME_LEN))
        {
            puc+=ITEM_NAME_LEN;
            if (!memcmp(puc, DI_CLOSE"\n", ITEM_VALUE_LEN))
            {
                pdich->iForceSts=TRUE;
                pdich->iVal=TRUE | 0x8000;
            }
            else if (!memcmp(puc, DI_OPEN"\n", ITEM_VALUE_LEN))
            {
                pdich->iForceSts=FALSE;
                pdich->iVal=FALSE | 0x8000;
            }
            else
            {
                if (memcmp(puc, DI_NORMAL"\n", ITEM_VALUE_LEN))
                    stsRet=EP_FILE_ERR;

                pdich->iForceSts=-1;
            }
            puc+=ITEM_VALUE_LEN;
        }
        else
        {
            stsRet=EP_FILE_ERR;
            puc+=ITEM_LEN;
        }
    }


    /* 读取CRC */
    memcpy(aucTemp, puc+ITEM_NAME_LEN, ITEM_VALUE_LEN);
    aucTemp[4] = '\0';

    usCrc = strtoul(aucTemp, NULL, 16);

    /* CRC是否一致判断 */
    if (usCrc != usCalcCrc)
    {
        LOG_Write(LOG_KERNEL, "开入强制文件CRC不一致.\n", NULL);

        stsRet = EP_FILE_ERR;
    }

    EP_free(pucBuf);
    return stsRet;
}

/* 检查开入强制文件是否有CRC
 * Para:
 *     pbCrc, 检查结果指针.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SC_Judge_Force_Di(BOOL *pbCrc)
{
    uint8_t *pucBuf = NULL;
    uint32_t ulLen;

    if (pbCrc == NULL)
    {
        return EP_FILE_ERR;
    }

    if ((pucBuf = FT_File_To_Mem(EP_DI_STS_FILE, &ulLen)) == NULL)
    {
        return EP_FILE_ERR;
    }

    /* 增加CRC项, 考虑兼容处理 */
    if (((ulLen != ITEM_LEN*(iLgcDiChNum_g+1)) && (ulLen != ITEM_LEN*(iLgcDiChNum_g+2)))
            || memcmp(pucBuf, pucDiFileHead_g, ITEM_LEN))
    {
        EP_free(pucBuf);

        return EP_FILE_ERR;
    }

    /* 根据是否有CRC项进行判断
     */
    if (ulLen == ITEM_LEN*(iLgcDiChNum_g+1))
    {
        *pbCrc = FALSE;
    }
    else
    {
        *pbCrc = TRUE;
    }

    EP_free(pucBuf);

    return EP_SUCCESS;
}

/* Read hardware DI measurement value.
 * Parameters:
 *      iIdx, index of DI(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DI. */
int RD_Mea_Hw_DI(int iIdx)
{
    assert(iIdx>=0 && iIdx<iLgcDiChNum_g);

    return plgcdich_g[iIdx].iVal;
}

/* 取得采样数据有效状态,
 * 参数：   ulCnt，AI采样节拍计数器值，该32位计数器自由运行，用来表示AI
 *              数据的采样时刻
 * 返回值： 该采样点的实时数据有效性,TRUE为有效，FALSE为无效 */

BOOL   RD_Get_Data_Valid(uint32_t ulCnt)
{

    BOOL *pb;

    assert(rdinfo_g.ulCurrAiCnt-ulCnt<rdinfo_g.uiBufPts); /* RD_SYS_INFO 结构，ulCurrAiCnt为当前节拍，uiBufPts为当前缓冲区点数 */

    /* 配置处理 */
    if (aimodExt_g.iLgcNum || iExtDiNum_g)
    {
        /*若有扩展机箱  */

        pb=aivaliddb_g.pbBufBgn+(int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*aivaliddb_g.uiTotalCh;
        if(pb<aivaliddb_g.pbBufBgn)
        {
            pb=pb+aivaliddb_g.ulBufLen;
        }

        assert(pb>=aivaliddb_g.pbBufBgn && pb<aivaliddb_g.pbBufEnd);
        return  ((*pb)&&(*(pb+1)));	/*只有主机箱和扩展机箱在同一采样点同时有效时,才有效  */
    }
    else
    {
        /*若只有主机箱  */

        pb=aivaliddb_g.pbBufBgn+(int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*aivaliddb_g.uiTotalCh;
        if(pb<aivaliddb_g.pbBufBgn)
        {
            pb=pb+aivaliddb_g.ulBufLen;
        }

        assert(pb>=aivaliddb_g.pbBufBgn && pb<aivaliddb_g.pbBufEnd);
        return  *pb;
    }
}

/* 函数名: RD_AI_Msu_Dat_P()
* 函数功能: 返回测量计算结果存储首地址
* 参数: 无
* 返回值：测量计算结果存储首地址 */
float *RD_AI_Msu_Dat_P(void)
{
    return (float *)msucaidb_g.pxBufBgn;
}

/* 检查测量MSU对应的实时MSU数据索引
*  参数：   strMsuLgAiId，测量通道逻辑标识字符串
*  返回值： 用来索引逻辑通道的void指针，或者NULL表示找不到 */
void *RD_Msu_AI_Hnd(uint8_t *strMsuLgAiId)
{
    RD_MSU_AI_CH *pmsu;

    assert(strMsuLgAiId && strlen(strMsuLgAiId)<=MAX_ID_LEN);
    for (pmsu=pmsuaich_g; pmsu<pmsuaich_g+iMsuAiChNum_g; pmsu++)
    {
        /* logMsg("%s %s\n",strMsuLgAiId,pmsu->aucId,0,0,0,0); */
        if (!strcmp(strMsuLgAiId, pmsu->aucFlagId))
            return pmsu;
    }
    return NULL;
}

/* tesing the data buffer.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void BufTest(void)
{
    int i;
    uint32_t ulAiCnt;
    float *pf;

    ulAiCnt=rdinfo_g.ulCurrAiCnt;

#ifndef NO_DEBUG
    logMsg(" Num: %d\n",iLgcAiChNum_g,0,0,0,0,0);
#endif

    for(i=0; i<iLgcAiChNum_g; i++)
    {
        if(IS_REAL_AI(plgcaich_g[i].ucUnit))
        {
            pf = RD_Lgc_AI_P(plgcaich_g,ulAiCnt);
            logMsg("Log: %d %d\n",(int)(*pf*100),plgcaich_g[i].ucUnit,0,0,0,0);
        }
        if(IS_CPLX_AI(plgcaich_g[i].ucUnit))
        {
            pf = (float *)RD_Calc_AI_P(plgcaich_g+i,ulAiCnt);
            logMsg("Cal: %d  %d %d\n",(int)(pf[0]*100),(int)(pf[1]*100),plgcaich_g[i].ucUnit,0,0,0);
        }
    }
}

/* tesing the data buffer.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void BufRdTest(void)
{
    int i;
    uint32_t ulAiCnt;
    float *pf;

    ulAiCnt=rdinfo_g.ulCurrAiCnt;

    logMsg(" Num: %d\n",iLgcAiChNum_g,0,0,0,0,0);

    for(i=0; i<iLgcAiChNum_g; i++)
    {
        if(IS_REAL_AI(plgcaich_g[i].ucUnit))
        {
            pf = RD_Lgc_AI_P(plgcaich_g+i,ulAiCnt);
            logMsg("Channel %d:\nLog: %s = %d\n", plgcaich_g[i].phwai->ucModCh, (int)plgcaich_g[i].aucId, (int)(*pf*100), 0, 0, 0);
        }
        if(IS_CPLX_AI(plgcaich_g[i].ucUnit))
        {
            pf = (float *)RD_Calc_AI_P(plgcaich_g+i,ulAiCnt);
            logMsg("Channel %d:\nCal: %s = %d  %d\n", plgcaich_g[i].phwai->ucModCh, (int)plgcaich_g[i].aucId, (int)(pf[0]*100), (int)(pf[1]*100), 0, 0);
        }
    }
}

/* showing all data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowData(void)
{
    uint32_t i;

    for(i=0; i<lgcaidb_g.ulBufLen; i++)
        printf("%d %d %d %d\n",
               (int)((lgcaidb_g.pfBufBgn-lgcaidb_g.ulBufLen)[i]*100),
               (int)(lgcaidb_g.pfBufBgn[i]*100),
               (int)(((float *)(calcaidb_g.pxBufBgn-calcaidb_g.ulBufLen))[i]*100),
               (int)(((float *)calcaidb_g.pxBufBgn)[i]*100));
}

/* showing one point data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowDataChn1(void)
{
    uint32_t i;

    AdcintDisable();

    for(i=0; i<lgcaidb_g.ulBufLen/lgcaidb_g.uiTotalCh; i++)
        printf("%d %d\n",(int)((lgcaidb_g.pfBufBgn-lgcaidb_g.ulBufLen)[i*lgcaidb_g.uiTotalCh]*100),(int)(lgcaidb_g.pfBufBgn[i*lgcaidb_g.uiTotalCh]*100));
}

/* showing measuring buffer.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowDataMsu(void)
{
    uint32_t i;

    logMsg("MSU Data: %d\n",msucaidb_g.uiTotalCh,0,0,0,0,0);

    for(i=0; i<msucaidb_g.ulBufLen; i++)
    {
        logMsg("%d\n",(int)(((float *)msucaidb_g.pxBufBgn)[i]*1000),0,0,0,0,0);
    }
}

/* measuring result showing.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void MsuDataOut(void)
{
    RD_MSU_AI_CH *paichmsu;
    float *pftmp;
    float *pftmp2;

    pftmp2 = (float *)(aimodDsp_g.pxmsuDbBgn);

    logMsg("%d %d %d %d\n",(int)(pftmp2[0]*100),(int)(pftmp2[1]*100),(int)(pftmp2[2]*100),(int)(pftmp2[3]*100),0,0);

    for(paichmsu = pmsuaich_g; paichmsu<pmsuaich_g+iMsuAiChNum_g; paichmsu++)
    {
        pftmp = (float *)(paichmsu->pxMsucAI);
        logMsg("%s %d %d\n",(int)paichmsu->aucId, (int)(pftmp[0]*100),(int)(pftmp[1]*100), 0, 0, 0);
    }
}

/* DSP result showing.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void DSPDataOutRd(void)
{
    RD_LGC_AI_CH *paich;
    float *pftmp;

    logMsg("DSP Data Showing Begin!%d\n", iLgcAiChNum_g, 0, 0, 0, 0, 0);

    for (paich = plgcaich_g; paich < plgcaich_g + iLgcAiChNum_g; paich++)
    {
        if (IS_REAL_AI(paich->ucUnit))
        {
            pftmp = (float *)(paich->pdat.pfLgcAI);
            logMsg("实数: %s =%d\n", (int)paich->aucId, (int)(pftmp[0]*1000), 0, 0, 0, 0);
        }
        if (IS_CPLX_AI(paich->ucUnit)) /* 幅度相角计算 */
        {
            pftmp = (float *)(paich->pdat.pxCalcAI);
            logMsg("复数: %s = %d	%d\n", (int)paich->aucId, (int)(pftmp[0]*1000), (int)(pftmp[1]*1000), 0, 0, 0);
        }
    }
}

/* address check.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void AddrCheckLog(uint32_t CheckAddr)
{
    assert((uint32_t *)CheckAddr >= (uint32_t *)lgcaidb_g.pfBufBgn && (uint32_t *)CheckAddr<(uint32_t *)lgcaidb_g.pfBufEnd);
}

/* address check.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void AddrCheckCalc(uint32_t CheckAddr)
{
    assert((uint32_t *)CheckAddr >= (uint32_t *)calcaidb_g.pxBufBgn && (uint32_t *)CheckAddr<(uint32_t *)calcaidb_g.pxBufEnd);
}

/***********************************************************************
* ShowLedState - 显示信号灯状态
*
* RETURNS: 无
*
*/
void ShowLedState(void)
{
    RD_LGC_LED_CH *plgcled;

    plgcled = plgcledch_g;
    LOG_Dbg_Msg("iHwLedChNum_g = %d iSwLedChNum_g = %d\n",iHwLedChNum_g,iSwLedChNum_g,0,0,0,0);		/* 软灯和硬灯的总数 */
    for(plgcled=plgcledch_g; plgcled<plgcledch_g+iHwLedChNum_g+iSwLedChNum_g; plgcled++)
    {
        /* 当前状态 */
        if(plgcled->bSts)
        {
            LOG_Dbg_Msg("%s: 亮%s\n",(int)plgcled->aucId,(int)plgcled->aucName,0,0,0,0);
        }
        else
        {
            LOG_Dbg_Msg("%s: 灭\n",(int)plgcled->aucId,(int)plgcled->aucName,0,0,0,0);
        }
    }
}

/***********************************************************************
* ShowDoState - 显示开出状态
*
* RETURNS: 无
*
*/
void ShowDoState(void)
{
    RD_LGC_DO_CH *plgcdo;

    plgcdo = plgcdoch_g;
    logMsg("iLgcDoChNum_g = %d\n", iLgcDoChNum_g, 0, 0, 0, 0, 0);		/* 开出总数 */
    for(plgcdo=plgcdoch_g; plgcdo<plgcdoch_g+iLgcDoChNum_g; plgcdo++)
    {
        /* 当前状态 */
        if(plgcdo->iVal)
        {
            logMsg("%s--%s: 合位, iForceSts = %d.\n",
                   (int)plgcdo->aucId,
                   (int)plgcdo->aucName,
                   (int)plgcdo->iForceSts, 0, 0, 0);
            printf("%s--%s: 合位.\n", plgcdo->aucId, plgcdo->aucName);
        }
        else
        {
            logMsg("%s--%s: 分位, iForceSts = %d.\n",
                   (int)plgcdo->aucId,
                   (int)plgcdo->aucName,
                   (int)plgcdo->iForceSts, 0, 0, 0);
            printf("%s--%s: 分位.\n", plgcdo->aucId, plgcdo->aucName);
        }
    }
}

/* showing the DI status.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowDiState(void)
{
    RD_LGC_DI_CH *plgcdi;

    plgcdi = plgcdich_g;

    LOG_Dbg_Msg("iLgcDiChNum_g = %d\n", iLgcDiChNum_g, 0, 0, 0, 0, 0);	 /* 开入总数 */

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        /* 当前状态 */
        if (plgcdi->iVal & 0x8000)
        {
            LOG_Dbg_Msg("%s--%s: 强制态. %d %lu\n", (int)plgcdi->aucId, (int)plgcdi->aucName, plgcdi->bPended, plgcdi->ulChgTime, 0, 0);
            printf("%s--%s: 强制态. %d %lu\n", plgcdi->aucId, plgcdi->aucName, plgcdi->bPended, plgcdi->ulChgTime);
        }
        else if (plgcdi->iVal & 0x7FFF)
        {
            LOG_Dbg_Msg("%s--%s: 合位. %d %d %lu\n", (int)plgcdi->aucId, (int)plgcdi->aucName, plgcdi->bPended,
                        plgcdi->iVal, plgcdi->ulChgTime, 0);
            printf("%s--%s: 合位. %d %d %lu\n", plgcdi->aucId, plgcdi->aucName, plgcdi->bPended, plgcdi->iVal, plgcdi->ulChgTime);
        }
        else
        {
            LOG_Dbg_Msg("%s--%s: 分位. %d %lu\n", (int)plgcdi->aucId, (int)plgcdi->aucName, plgcdi->bPended, plgcdi->ulChgTime, 0, 0);
            printf("%s--%s: 分位. %d %lu\n", plgcdi->aucId, plgcdi->aucName, plgcdi->bPended, plgcdi->ulChgTime);
        }
    }
}

/***********************************************************************
* ShowPoVal - 显示PO
*
* RETURNS: 无
*
*/
void ShowPoState(void)
{
    RD_LGC_PO_CH *plgpo;
    FLT_U32_UNION data;
    FLT_U32_UNION data2;

    plgpo = plgcpoch_g;
    LOG_Dbg_Msg("iLgcPoChNum_g = %d\n", (int)iLgcPoChNum_g, 0, 0, 0, 0, 0);		/* 开出总数 */
    for(plgpo=plgcpoch_g; plgpo<plgcpoch_g+iLgcPoChNum_g; plgpo++)
    {
        /* 当前值 */
        data.ulVal = plgpo->Val.ulVal;
        data2.ulVal = plgpo->OriginVal.ulVal;
        LOG_Dbg_Msg("%s: Current Val = %d, Origin Val = %d.\n",
                    (int)plgpo->aucName,
                    (int)(data.fVal*1000),
                    (int)(data2.fVal*1000), 0, 0, 0);
    }
}

/***********************************************************************
* GetUnitType - 获得单位类型
*
* RETURNS: 无
*
*/
void GetUnitType(void)
{
    DSP_LGC_AI_CFG *plcfg;
    RD_HW_AI_CH *pch;

    for(plcfg = pdspl_cfg_g; plcfg<pdspl_cfg_g+aimodDsp_g.iLgcNum; plcfg++)
    {
#ifndef NO_DEBUG
        LOG_Dbg_Msg("plcfg->ucHdCh = %d plcfg->ucFiltNum = %d\n",plcfg->ucHdCh, plcfg->ucFiltNum,0,0,0,0);
#endif
        for(pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
        {
            if((plcfg->ucHdCh -1)== pch->ucModCh)
            {
                plcfg->ucUnit = pch->ucUnit;
#ifndef NO_DEBUG
                /* LOG_Dbg_Msg("plcfg->ucUnit  = %d aimodDsp_g.iLgcNum =%d iHwAiChNum_g = %d plcfg->ucHdCh = %d plcfg->ucFiltNum = %d\n", plcfg->ucUnit, aimodDsp_g.iLgcNum, iHwAiChNum_g, plcfg->ucHdCh, plcfg->ucFiltNum, 0); */
#endif
                break;
            }
        }
    }
}

/***********************************************************************
* GetDICode -得到开入量的序号根据开入量的名称
*
* RETURNS: 序号
*
*/
int  GetDICode(char * DiName)
{
    RD_LGC_DI_CH *pch;
    int i ;

    pch = plgcdich_g;
    for(i=0; i<iLgcDiChNum_g; i++)
    {
        /* logMsg("%d %d %s %s \n",iLgcDiChNum_g,pch-plgcdich_g,pch->aucId,DiName,0,0); */
        if(strcmp(pch->aucId,DiName)==0)
            return (pch-plgcdich_g);
        pch++;
    }

    return -1;
}

/***********************************************************************
* GetLedCode -得到信号灯的序号根据信号灯的名称
*
* RETURNS: 序号
*
*/
int  GetLedCode(char * LedName)
{
    RD_LGC_LED_CH *pch;
    int i ;

    pch = plgcledch_g;
    for(i=0; i<iLgcLedChNum_g; i++)
    {
        /* logMsg("%d %d %s %s \n",iLgcDiChNum_g,pch-plgcdich_g,pch->aucId,DiName,0,0); */
        if(strcmp(pch->aucId, LedName) == 0)
            return (pch-plgcledch_g);
        pch++;
    }

    return -1;
}

/***********************************************************************
* GetDOCode -得到开入量的序号根据开入量的名称
*
* RETURNS: 序号
*
*/
int  GetDOCode(char * DoName)
{
    RD_LGC_DO_CH *pch;
    int i ;

    pch = plgcdoch_g;
    for(i=0; i<iLgcDoChNum_g; i++)
    {
        /* logMsg("%d %d %s %s \n",iLgcDiChNum_g,pch-plgcdich_g,pch->aucId,DiName,0,0); */
        if(strcmp(pch->aucId,DoName)==0)
            return (pch-plgcdoch_g);
        pch++;
    }

    return -1;
}

/***********************************************************************
* VI_DI_Change -DI状态改变记录
*
* RETURNS: NONE
*
*/
void VI_DI_Change(
    RD_LGC_DI_CH *pDi, 		/* RD_LGC_DI_CH channel */
    BOOL bSts, 		/* new DI status */
    uint32_t ulTime			/* us time */
)
{
    VI_RUN_INFO *pinf;
    int iLockKey;

    assert(pDi);
    assert(pDi>=plgcdich_g && (pDi<(plgcdich_g+iLgcDiChNum_g)));

    pinf=VI_Run_Info_Wr_P();

    if(bViewModIsInit_g)				/* VI模块是否完成标志 */
    {
        pinf->bViewModIsInit=TRUE;
    }
    else
    {
        pinf->bViewModIsInit=FALSE;
    }

    pinf->type=DI_CHG;
    /* logMsg("DI_CHG\n", 0, 0, 0, 0, 0, 0); */

    pinf->msg.di.ucFtype=17;
    pinf->msg.di.ucCOT=1;

    if (bSts)
        pinf->msg.di.ucDIQ=0x21;
    else
        pinf->msg.di.ucDIQ=0x20;

    pinf->msg.di.pcfg=pDi;
    pinf->msg.di.bSts=bSts;


    pinf->msg.di.ulTime=ulTime;


    pinf->msg.di.unIdx = pDi - plgcdich_g;
    pinf->msg.di.unCh=pDi - plgcdich_g;
    iLockKey=intLock();

    if (rptsts_g.iFault)
    {
        pinf->msg.di.unRptSN=rptsts_g.unRptSN;
        pinf->msg.di.ucRecSN=rptsts_g.ucRecSN;
    }
    else
    {
        pinf->msg.di.unRptSN=rptsts_g.unRptSN++;
        pinf->msg.di.ucRecSN=0;
    }

    intUnlock(iLockKey);

    VI_End_Wr_Run_Info();
}

/***********************************************************************
* GetPoVal - 获取PO值
*
* RETURNS: 无
*
*/
void GetPoVal(RD_PO_MEA *phwmeaRslt)
{
    RD_LGC_PO_CH *pch;
    STATUS vxsts;

    assert(phwmeaRslt);
    vxsts=semTake(semPoMea, WAIT_FOREVER); /* 读取物理通道标志 */

    for (pch=plgcpoch_g; pch<plgcpoch_g+iLgcPoChNum_g; pch++)
    {
        phwmeaRslt->fVal = pch->Val.TotalEnergy;
        phwmeaRslt->ucType = pch->ucType;
        phwmeaRslt++;
    }

    vxsts=semGive(semPoMea);
    assert(vxsts==OK);
}

/***********************************************************************
* GetPofVal - 获取PO值
*
* RETURNS: 无
*
*/
void GetPofVal(float *fRslt)
{
    RD_LGC_PO_CH *pch;
    STATUS vxsts;

    assert(fRslt);
    vxsts=semTake(semPoMea, WAIT_FOREVER); /* 读取标志 */
    assert(vxsts == OK);

    for (pch=plgcpoch_g; pch<plgcpoch_g+iLgcPoChNum_g; pch++)
    {
        *fRslt++=pch->Val.TotalEnergy;
    }

    vxsts=semGive(semPoMea);
    assert(vxsts == OK);
}

/***********************************************************************
* ForceDiLogicTestBegin - 强制开入逻辑测试开始
*
* RETURNS: 无
*
*/
void ForceDiLogicTestBegin()
{
    RD_LGC_DI_CH *pdich;

    pdich=plgcdich_g+4;
    pdich->iForceSts=TRUE;
}

/***********************************************************************
* ForceDiLogicTestEnd - 强制开入逻辑测试结束
*
* RETURNS: 无
*
*/
void ForceDiLogicTestEnd()
{
    RD_LGC_DI_CH *pdich;

    pdich=plgcdich_g+4;
    pdich->iForceSts=-1;
}

/* clear all the DO status using in the reverting operation.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RD_Clear_All_Phy_DO()
{
    RD_LGC_DO_CH *plgcdo;
    int i;

    taskLock();

    for (i=0; i<iLgcDoChNum_g; i++)
    {
        plgcdo=plgcdoch_g+i;
        if (plgcdo->mod==RD_SPI_DO)
        {
            RD_Set_DO(plgcdo,0);/* 必须调用该函数，因为我们对DO操作只允许通过两个函数操作，
                                       RD_Set_DO和RD_Force_DO ，否则实现无法保证状态的完整性 */
        }
    }
    taskUnlock();
}

#ifdef EXCITE_BUILD
/***********************************************************************
* RD_Get_Handle_InnerSymbol - 取得实时数据I/O通道的索引
*
* RETURNS: 用来索引实时数据I/O通道的void指针，或者NULL表示调用出错
*
*/
void *RD_Get_Handle_InnerSymbol
(
    uint8_t *strLgcId,		/* 逻辑标识字符串（名称）*/
    int  iHdlType			/* 句柄类型 */
)
{
    RD_LGC_DO_CH *plgcdo;

    if(iHdlType==RD_LGC_DO_HDL)
    {
        for (plgcdo=plgcdoch_g; plgcdo<plgcdoch_g+iLgcDoChNum_g; plgcdo++)
        {
            if (!strcmp(strLgcId, plgcdo->aucName))
                return plgcdo;
        }
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  DO.\n",
                    (int)strLgcId, 0, 0, 0, 0, 0);
        assert(FALSE);

        return NULL;
    }

    return NULL;
}

/***********************************************************************
* GetDOCodeInnerSymbol - 通过内部标识获得开入量的配置序号
*
* RETURNS: 无
*
*/
int  GetDOCodeInnerSymbol(char * DoName)
{
    RD_LGC_DO_CH *pch;
    int i ;

    pch = plgcdoch_g;
    for(i=0; i<iLgcDoChNum_g; i++)
    {
        /* logMsg("%d %d %s %s \n",iLgcDiChNum_g,pch-plgcdich_g,pch->aucId,DiName,0,0); */
        if(strcmp(pch->aucName,DoName)==0)
            return (pch-plgcdoch_g);
        pch++;
    }

    return -1;
}
#endif

/***********************************************************************
* SlakeUnkeepedLed - 熄灭非自保持信号灯
*
* RETURNS: 无
*
*/
void  SlakeUnkeepedLed(void)
{
    RD_LGC_LED_CH *pch;

    for(pch=plgcledch_g; pch<plgcledch_g+iLgcLedChNum_g; pch++)
    {
        if(!pch->bKeep)
        {
            RD_Set_LED(pch, FALSE);
        }
    }
}

/***********************************************************************
* RD_Get_DSP_MOD_Info - 获得DSP MOD的当前的信息，供其他机箱的MOD来使用
*
* RETURNS:
*              EP_SUCCESS, 成功
*              EP_ERROR, 失败
*
*/
EP_STATUS RD_Get_DSP_MOD_Info(
    uint32_t *pulRtNextCnt,
    int *piRtDbOfst
)
{
#ifdef EDP01_CA_OPT_BUILD		/* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */
    *pulRtNextCnt=aimodExt_g.ulNextCnt;
    *piRtDbOfst=aimodExt_g.pfWork-aimodExt_g.pfDbBgn;
#else
    *pulRtNextCnt=aimodDsp_g.ulNextCnt;
    *piRtDbOfst=aimodDsp_g.pfWork-aimodDsp_g.pfDbBgn;
#endif

    assert((*piRtDbOfst)>=0 && (*piRtDbOfst)<lgcaidb_g.ulBufLen);

    return EP_SUCCESS;
}

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
/***********************************************************************
* SeneLedState - 显示灯状态
*
* RETURNS: 无
*
*/
void SeneLedState(void)
{
    RD_LGC_LED_CH *pch;

    for(pch=plgcledch_g; pch<plgcledch_g+iLgcLedChNum_g; pch++)
    {
        if(pch->bSts && pch->bKeep)
        {
            SpiSetLampModeTest(GetLedCode(pch->aucName)+1, 0x03, 0, 0);
        }
        else
        {
            SpiSetLampModeTest(GetLedCode(pch->aucName)+1, 0x01, 0, 0);
        }
    }
}
#endif

/***********************************************************************
* VI_New_SOE_Test - SOE测试
*
* RETURNS: 无
*
*/
void VI_New_SOE_Test(void)
{
    RD_LGC_DI_CH *plgcdi;

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if(plgcdi->p_part->ucPosition == RD_LOCAL_BOX)
        {
            VI_New_SOE(plgcdi->iMeaCh, plgcdi->iVal, plgcdi->ulChgTime, plgcdi->bSOE, 0);
            VI_New_SOE(plgcdi->iMeaCh, plgcdi->iVal, plgcdi->ulChgTimeAfterFilt, plgcdi->bSOE, 0);
        }
    }
}

/***********************************************************************
* Sam_Adjust_Test - 节拍测试
*
* RETURNS: 无
*
*/
void Sam_Adjust_Test(
    uint8_t uSam,
    int iDeta
)
{
    uint8_t uResult;

    if(iDeta<0)
    {
        printf("%d = 商%d 余数%d\n", iDeta, -((-iDeta)/RD_SAM_SYN_CLK), -((-iDeta)%RD_SAM_SYN_CLK));
        iDeta=-((-iDeta)/RD_SAM_SYN_CLK);
    }
    else
    {
        printf("%d = 商%d 余数%d\n", iDeta, iDeta/RD_SAM_SYN_CLK, iDeta%RD_SAM_SYN_CLK);
        iDeta=iDeta%RD_SAM_SYN_CLK;
    }

    uResult=SynSamAdjust(uSam, iDeta);
    logMsg("uResult=%d\n", uResult, 0, 0, 0, 0, 0);
    printf("uResult=%d\n", uResult);
}

#if defined(EDP_01_02_BUILD)
/***********************************************************************
* RD_LightRunLamp - 保护CPU点亮或熄灭运行灯
*
* RETURNS: None
*
* Alert: 保护应用程序需要在快速保护任务中调用该函数。函数调用频率必须大于50HZ，否则运行灯熄灭
*
*/
void RD_LightRunLamp(
    BOOL bLightLamp			/* 点亮运行灯，TRUE: 点亮，FALSE: 熄灭 */
)
{
    static BOOL uiLstRunFlag_s=FALSE;

    Turn_Run_Led_High(uiLstRunFlag_s);
    uiLstRunFlag_s=!uiLstRunFlag_s;

    Turn_Flash_Led_High(bLightLamp);

    return;
}

/***********************************************************************
* Turn_Run_Led_High - 驱动运行灯LED，但不点亮，应该按高于50HZ的频率，变换驱动，否则，LED认为CPU未运行，不点亮,1.3BSP no
*
* RETURNS: 无
*
* Alert: 以前是BSP提供，合并版本后，由平台提供
*
*/
void Turn_Run_Led_High(
    BOOL ledStatus				/* TRUE; on    FALSE; off */
)
{
    if(ledStatus)
    {
        IoPinOutputHigh(IO_OUT_RUN_LED_PULSE, IO_PIN_HIGH);
    }
    else
    {
        IoPinOutputHigh(IO_OUT_RUN_LED_PULSE, IO_PIN_LOW);
    }
}

/***********************************************************************
* Turn_Flash_Led_High - 设置LED点亮与否，前提是Turn_Run_Led_High函数频繁变位;,1.3BSP no
*
* RETURNS: 无
*
* Alert: 以前是BSP提供，合并版本后，由平台提供
*/
void Turn_Flash_Led_High(
    BOOL ledStatus				/* TRUE; on    FALSE; off */
)
{

    if(ledStatus)
    {
        IoPinOutputHigh(IO_OUT_RUN_LED_ON, IO_PIN_HIGH);
    }
    else
    {
        IoPinOutputHigh(IO_OUT_RUN_LED_ON, IO_PIN_LOW);
    }
}
#endif

/***********************************************************************
* RD_LightAlarmLamp - 保护CPU点亮或熄灭告警灯
*
* RETURNS: None
*
* Alert: 保护应用程序调用该函数。
*函数调用只需变位调用,和平台产生的告警是或逻辑
*
*/
void RD_LightAlarmLamp(
    BOOL bLightLamp			/* 点亮告警灯，TRUE: 点亮，FALSE: 熄灭 */
)
{
    static BOOL uiLstAlarmFlag_s=FALSE;

    if (bLightLamp != uiLstAlarmFlag_s)
    {
        if (bLightLamp)
            EP_Set_Sts_Bit(SET_ALARM_FLAG);
        else
            EP_Clr_Sts_Bit(SET_ALARM_FLAG);
        uiLstAlarmFlag_s=bLightLamp;
        g_bAppAlarm = bLightLamp;
    }
    return;
}

/* 根据采样通道指针获取状态标指针(8位)
 * Para:
 *     pf, 采样通道指针.
 * Return:
 *     状态指针(8位).
 */
uint8_t  *RD_Cnvrt_AI_P_to_Sts_P(float *pf)
{
    int a=0;
    uint8_t *pu;

    a=(pf-lgcaidb_g.pfBufBgn);
    pu = (uint8_t *)(a + lgcaistsdb_g.pBufBgn);

    return pu+3;
}

/* 根据句柄和采样节拍获取状态标指针(8位)
 * Para:
 *     pvLgcAiHnd, 句柄.
 *     ulSampleCnt, 节拍.
 * Return:
 *     状态指针(8位).
 */
uint8_t  *RD_Get_Chn_Sts (void *pvLgcAiHnd, uint32_t ulSampleCnt)
{
    float *pf;

    pf = RD_Lgc_AI_P(pvLgcAiHnd, ulSampleCnt);
    return (uint8_t *)RD_Cnvrt_AI_P_to_Sts_P(pf);
}

/* 根据采样通道指针获取状态标指针(32位)
 * Para:
 *     pf, 采样通道指针.
 * Return:
 *     状态指针(32位).
 */
uint32_t *RD_Cnvrt_AI_P_to_Sts_All_P(float *pf)
{
    int a = 0;

    a = (pf-lgcaidb_g.pfBufBgn);
    return (uint32_t *)(a + lgcaistsdb_g.pBufBgn);
}

/* 根据句柄和采样节拍获取状态标指针(32位)
 * Para:
 *     pvLgcAiHnd, 句柄.
 *     ulSampleCnt, 节拍.
 * Return:
 *     状态指针(32位).
 */
uint32_t *RD_Get_Chn_Sts_All (void *pvLgcAiHnd, uint32_t ulSampleCnt)
{
    float *pf;
    uint32_t *pu;

    pf = RD_Lgc_AI_P(pvLgcAiHnd, ulSampleCnt);
    pu = RD_Cnvrt_AI_P_to_Sts_All_P(pf);

    return pu;
}


/* 获得当前所有采样源最新有效的采样节拍,
   注意:目前只处理本地采样（包括本地，扩展，数字化（智能操作箱，9-1，数据集中器））和光差采样，
        其他的暂时不考虑（比如励磁，同杆，虚拟机箱）
   参数:无
   返回:返回所有采样源的有效采样节拍
*/
uint32_t   RD_GetAllAIValidCnt()
{
    uint32_t  ulOptVldAICnt,ulVldAICnt;

    ulVldAICnt=RD_AI_Cnt();

    if(OPT_ChIsComNormal(0,&ulOptVldAICnt))
    {
        //若光差通信正常
        if((int)(ulOptVldAICnt-ulVldAICnt)<0)
        {
            ulVldAICnt=ulOptVldAICnt;
        }
    }

    if(OPT_ChIsComNormal(1,&ulOptVldAICnt))
    {
        if((int)(ulOptVldAICnt-ulVldAICnt)<0)
        {
            ulVldAICnt=ulOptVldAICnt;
        }
    }

    return  ulVldAICnt;
}

/* 查询通道网络端口来源(根据通道逻辑标志,通道正常后,即AI_COM_ERR清0时才能读取到有效对应关系)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 *     pSlvPortNum, 前级CC板端口号,返回值为0xFF时无效
 * Return:
 *     本级CC板端口号,返回值为0xFF时无效
 */
uint8_t RD_Get_Chn_Src (void *pvLgcAiHnd, uint8_t *pSlvPortNum)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道配置 */

    assert(pvLgcAiHnd);
    plgcai = (RD_LGC_AI_CH*)pvLgcAiHnd;

    if (plgcai->phwai != NULL)
    {
        if (pSlvPortNum)
        {
            *pSlvPortNum = plgcai->phwai->ucSlvPortNum;
        }
        return plgcai->phwai->ucMstPortNum;
    }
    else
    {
        if (pSlvPortNum)
        {
            *pSlvPortNum = 0xFF;
        }

        return 0xFF;
    }
}

/* 查询通道svID(根据通道逻辑标志,通道正常后,即AI_COM_ERR清0时才能读取到有效对应关系)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 * Return:
 *     svID存储地址
 */
uint8_t *RD_Get_svID_Src (void *pvLgcAiHnd)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道配置 */

    assert(pvLgcAiHnd);
    plgcai = (RD_LGC_AI_CH*)pvLgcAiHnd;

    if (plgcai->phwai != NULL)
    {
        return plgcai->phwai->arrSVID;
    }
    else
    {
        return NULL;
    }
}

/***********************************************************************
* RD_Get_AI_Quality - 取得实时AI数据品质
*
* RETURNS: 品质位.
*
*/
uint16_t RD_Get_AI_Quality(
    void *pvAiHnd			/* 用来索引AI数据元素的void指针(RD_LGC_AI_CH)，应该通过调用本模块提供的RD_Get_Handle得到 */
)
{
    RD_LGC_AI_CH *plgcai;
    HDL_AI_HND * pAiHdl;
    assert(pvAiHnd);

    plgcai=(RD_LGC_AI_CH*)pvAiHnd;
    assert(plgcai->phwai);
    assert(plgcai->phwai->pAiHdl);
    pAiHdl = (HDL_AI_HND *)plgcai->phwai->pAiHdl;

    return pAiHdl->usQuality;
}

/***********************************************************************
* SynSamAdjust - 同步节拍调整
*
* RETURNS: 调整后节拍
*
*/
uint8_t SynSamAdjust(
    uint8_t uInputSam,		/* 当前节拍 */
    int iDelta		/* 需调整节拍 */
)
{
    /* 供光纵使用 */
    int iResult;

    if(iDelta>(int)RD_SAM_SYN_CLK)
    {
        /* 向前循环 */
        iDelta=iDelta%RD_SAM_SYN_CLK;
    }
    else if(iDelta<-(int)RD_SAM_SYN_CLK)
    {
        /* 向后循环 */
        iDelta=-((-iDelta)%RD_SAM_SYN_CLK);
    }

    iResult=uInputSam+iDelta;

    if(iResult<0)
    {
        iResult=iResult+RD_SAM_SYN_CLK;
    }
    else if(iResult >= RD_SAM_SYN_CLK)
    {
        iResult=iResult-RD_SAM_SYN_CLK;
    }

    return (uint8_t)iResult;
}

/***********************************************************************
* RD_Lgc_AI - 取得实时实数AI数据
*
* RETURNS:  相应时刻相应通道的AI值
*
*/
float RD_Lgc_AI(
    float *pfLgcAi, 		/* 指向AI逻辑通道数据的指针，应该通过调用本模块提供的RD_Lgc_AI_P或者通过基准指针+偏移运算得到 */
    int iDelta			/* 距离该逻辑通道索引的采样间隔，只能<=0表示从前的点 */
)
{
#ifndef NO_DBL_BUF
    if(!(pfLgcAi>=lgcaidb_g.pfBufBgn && pfLgcAi<lgcaidb_g.pfBufEnd &&
            iDelta<=0))
    {
        return  0.0;
    }

    pfLgcAi=(float*)((uint8_t*)pfLgcAi+iDelta*lgcaidb_g.uiChBytes);
#else
    pfLgcAi = (float *)((uint8_t *)pfLgcAi+iDelta*lgcaidb_g.uiChBytes);

    if (pfLgcAi>=lgcaidb_g.pfBufEnd)
        pfLgcAi-=lgcaidb_g.ulBufLen;
    else if (pfLgcAi<lgcaidb_g.pfBufBgn)
        pfLgcAi+=lgcaidb_g.ulBufLen;

    if(!(pfLgcAi>=lgcaidb_g.pfBufBgn && pfLgcAi<lgcaidb_g.pfBufEnd))
    {
        return  0.0;
    }
#endif

    return *pfLgcAi;
}

/***********************************************************************
* RD_Adj_Calc_AI_P - 调整复数实时AI数据指针
*
* RETURNS: 指向相应时刻相应预处理通道的AI值（复数）的指针
* 注意: 该函数用来实现RD_Calc_Ai，绕过了inline返回值为COMPLEX的编译器bug，一般用户不应当使用。
*
*/
COMPLEX *RD_Adj_Calc_AI_P(
    COMPLEX *pxCalcAi, 			/* 指向AI预处理通道数据的指针，应该通过调用本模块提供的RD_Calc_AI_P或通过基准指针+偏移运算得到 */
    int iDelta			/* 距离该逻辑通道索引的采样间隔，只能<=0表示从前的点 */
)
{
#ifndef NO_DBL_BUF
    if(!(pxCalcAi>=calcaidb_g.pxBufBgn && pxCalcAi<calcaidb_g.pxBufEnd &&
            iDelta<=0)&&calcaidb_g.uiChBytes)
    {
        return  NULL;
    }

    pxCalcAi=(COMPLEX*)((uint8_t*)pxCalcAi+iDelta*calcaidb_g.uiChBytes);
#else
    pxCalcAi+=iDelta*calcaidb_g.uiTotalCh;

    if (pxCalcAi>=calcaidb_g.pxBufEnd)
        pxCalcAi-=calcaidb_g.ulBufLen;
    else if (pxCalcAi<calcaidb_g.pxBufBgn)
        pxCalcAi+=calcaidb_g.ulBufLen;

    if(!(pxCalcAi>=calcaidb_g.pxBufBgn && pxCalcAi<calcaidb_g.pxBufEnd)&&calcaidb_g.uiChBytes)
    {
        return  NULL;
    }
#endif

    return pxCalcAi;
}

/***********************************************************************
* RD_Wr_Calc_Vt_AI - 写复数虚拟通道AI数据
*
* RETURNS: 无
*
*/
void RD_Wr_Calc_Vt_AI(
    COMPLEX *pxCalcVtAi,		/* 指向AI虚拟预处理通道数据的指针，应该通过调用本模块提供的RD_Calc_AI_P或者通过基准指针+偏移运算得到 */
    int iDelta, 		/* 距离该逻辑通道索引的采样间隔，负数表示从前的点，正数表示 */
    COMPLEX xVal				/* 欲写的值 */
)
{
    COMPLEX *px;

    px=(COMPLEX*)((uint8_t*)pxCalcVtAi+iDelta*calcaidb_g.uiChBytes);

    if (px>=calcaidb_g.pxBufEnd)
        px=(COMPLEX*)((uint8_t*)px-calcaidb_g.ulBufBytes);
    else if (px<calcaidb_g.pxBufBgn)
        px=(COMPLEX*)((uint8_t*)px+calcaidb_g.ulBufBytes);

    *px=xVal;

#ifndef NO_DBL_BUF
    px=(COMPLEX*)((uint8_t*)px-calcaidb_g.ulBufBytes);

    *px=xVal;
#endif
}

/***********************************************************************
* RD_Wr_Lgc_Vt_AI - 写实数虚拟通道AI数据
*
* RETURNS: 无
*
*/
void RD_Wr_Lgc_Vt_AI(
    float *pfLgcVtAi, 		/* 指向AI虚拟逻辑通道数据的指针，应该通过调用本模块提 */
    int iDelta, 		/* 距离该逻辑通道索引的采样间隔，负数表示从前的点，正数表示将来的点 */
    float fVal		/* 欲写的值 */
)
{
    float *pf;

    pf=(float*)((uint8_t*)pfLgcVtAi+iDelta*lgcaidb_g.uiChBytes);

    if (pf>=lgcaidb_g.pfBufEnd)
        pf=(float*)((uint8_t*)pf-lgcaidb_g.ulBufBytes);
    else if (pf<lgcaidb_g.pfBufBgn)
        pf=(float*)((uint8_t*)pf+lgcaidb_g.ulBufBytes);

    *pf=fVal;

#ifndef NO_DBL_BUF
    pf=(float*)((uint8_t*)pf-lgcaidb_g.ulBufBytes);

    *pf=fVal;
#endif
}

/***********************************************************************
* RD_Lgc_AI_P - 取得AI逻辑通道数据指针
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Lgc_AI调用来获得历史数据
*
*/
float *RD_Lgc_AI_P(
    void *pvLgcAiHnd, 		/* 用来索引AI逻辑通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道配置*/
    float *pf;
    uint32_t ulDif;

    plgcai=(RD_LGC_AI_CH*)pvLgcAiHnd;

    /* 越界判断
     */

    if ((rdinfo_g.ulCurrAiCnt-ulCnt)<0x7fffffff)
    {
        ulDif = rdinfo_g.ulCurrAiCnt - ulCnt;
    }
    else
    {
        ulDif = ulCnt - rdinfo_g.ulCurrAiCnt;
    }

    if (ulDif<rdinfo_g.uiBufPts) /* 获取点数和当前点数以及总的点数的关系 */
    {
        pf=(float*)((uint8_t*)plgcai->pdat.pfLgcAI+
                    (int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*lgcaidb_g.uiChBytes); /* 一个采样点所有通道采集到的数据占的字节数 */

        if (pf<lgcaidb_g.pfBufBgn)
            pf=(float*)((uint8_t*)pf+lgcaidb_g.ulBufBytes);
        else if (pf >= lgcaidb_g.pfBufEnd)
        {
            pf = (float *)((uint8_t *)pf-lgcaidb_g.ulBufBytes);
        }

        return pf;
    }
    else
    {
        return NULL;
    }
}

/***********************************************************************
* RD_Lgc_AI_P_MC - 取得AI逻辑通道数据指针(测控使用提高效率)
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Lgc_AI调用来获得历史数据
*
*/
float *RD_Lgc_AI_P_MC(
    void *pvLgcAiHnd, 		/* 用来索引AI逻辑通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道配置*/
    HDL_AI_HND *pAiHdl;

    plgcai=(RD_LGC_AI_CH*)pvLgcAiHnd;
    if(plgcai->phwai->paimod == &aimodHdl_g)
    {
        pAiHdl = (HDL_AI_HND*)plgcai->phwai->pAiHdl;
        return &pAiHdl->fBufVal;
    }

    return RD_Lgc_AI_P(pvLgcAiHnd, ulCnt);

}

/***********************************************************************
* RD_Calc_AI_P - 取得AI预处理通道数据指针
*
* RETURNS:  指向该AI预处理通道数据（复数）的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Calc_AI调用来获得历史数据
*
*/
COMPLEX *RD_Calc_AI_P(
    void *pvCalcAiHnd, 	/* 用来索引AI预处理通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道读配置 */
    COMPLEX *px;

    plgcai=(RD_LGC_AI_CH*)pvCalcAiHnd; /* 逻辑通道配置 */
    if(!(rdinfo_g.ulCurrAiCnt-ulCnt<rdinfo_g.uiBufPts))
    {
        return  NULL;

    }
    int32_t temp=(int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*calcaidb_g.uiChBytes;
    px=(COMPLEX*)((uint8_t*)plgcai->pdat.pxCalcAI+temp);

    if (px<calcaidb_g.pxBufBgn)
        px=(COMPLEX*)((uint8_t*)px+calcaidb_g.ulBufBytes);

    if(!(px>=calcaidb_g.pxBufBgn && px<calcaidb_g.pxBufEnd)&&calcaidb_g.uiChBytes)
    {
        return  NULL;

    }

    return px;
}

/***********************************************************************
* RD_Base_Lgc_AI_P - 取得AI逻辑0通道数据指针
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以把它作为AI通道数据指针的
*                  基准，通过偏移运算得到用户逻辑通道数据的指针
*
*/
float *RD_Base_Lgc_AI_P(
    uint32_t ulCnt				/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI 数据的采样时刻 */
)
{
    float *pf;

    if(!(rdinfo_g.ulCurrAiCnt-ulCnt<rdinfo_g.uiBufPts)) /* ulCurrAiCnt为当前计数，uiBufPts为所有点数 */
    {
        return  NULL;
    }


    pf=(float*)((uint8_t*)lgcaidb_g.pfBufBgn+
                (int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*lgcaidb_g.uiChBytes);

    if (pf<lgcaidb_g.pfBufBgn)
        pf=(float*)((uint8_t*)pf+lgcaidb_g.ulBufBytes);

    if(!((pf>=lgcaidb_g.pfBufBgn && pf<lgcaidb_g.pfBufEnd) ||
            !lgcaidb_g.uiChBytes))
    {
        return  NULL;
    }

    return pf;
}

/***********************************************************************
* RD_Base_Calc_AI_P - 取得AI预处理0通道数据指针
*
* RETURNS:  指向该AI预处理0通道数据（复数）的指针，可以把它作为AI通道数据指针的
*                  基准，通过偏移运算得到用户预处理通道数据的指针
*
*/
COMPLEX *RD_Base_Calc_AI_P(
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
)
{
    COMPLEX *px;

    if(!(rdinfo_g.ulCurrAiCnt-ulCnt<rdinfo_g.uiBufPts))
    {
        return  NULL;
    }

    px=(COMPLEX*)((uint8_t*)calcaidb_g.pxBufBgn+
                  (int32_t)(ulCnt-rdinfo_g.ulBgnCnt)*calcaidb_g.uiChBytes); /* 第一个数据的采样节拍 */

    if (px<calcaidb_g.pxBufBgn)
        px=(COMPLEX*)((uint8_t*)px+calcaidb_g.ulBufBytes);

    if(!((px>=calcaidb_g.pxBufBgn && px<calcaidb_g.pxBufEnd) ||
            !calcaidb_g.uiChBytes))
    {
        return  NULL;

    }

    return px;
}

/***********************************************************************
* RD_His_DI - 取得历史DI数据
*
* RETURNS: TRUE，开入量闭合
*                 FALSE，开入量打开
*
*/
BOOL RD_His_DI(
    void *pvDiHnd, 			/* 用来索引DI数据元素的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t ulAiCnt						/* AI采样节拍计数器值 */
)
{
    RD_LGC_DI_CH *plgcdi;
    BOOL *pb;

    if(!(rdinfo_g.ulCurrAiCnt-ulAiCnt<rdinfo_g.uiBufPts))
    {

        return  FALSE;
    }

    plgcdi=(RD_LGC_DI_CH*)pvDiHnd;

    pb=plgcdi->pbDI+
       (int32_t)(ulAiCnt-rdinfo_g.ulBgnCnt)*didb_g.uiTotalCh;

    if (pb<didb_g.pbBufBgn)
        pb+=didb_g.ulBufLen;

    return *pb;
}

/* 取得历史DI数据缓冲的某采样节拍的基址
 * 参数：   pvDiHnd，用来索引DI数据元素的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          ulAiCnt，AI采样节拍计数器值
 * 返回值： TRUE，开入量闭合
 *          FALSE，开入量打开 */
BOOL * RD_Base_His_DI_P( uint32_t ulAiCnt)
{
    /* 供光纵使用，2006-2-14 */
    BOOL *pb;


    if(!(rdinfo_g.ulCurrAiCnt-ulAiCnt<rdinfo_g.uiBufPts))
    {
        return NULL;
    }

    pb=didb_g.pbBufBgn+
       (int32_t)(ulAiCnt-rdinfo_g.ulBgnCnt)*didb_g.uiTotalCh;

    if (pb<didb_g.pbBufBgn)
        pb+=didb_g.ulBufLen;

    return pb;
}

/***********************************************************************
* RD_Get_Src_DI - 取得原始实时DI数据,若为本地开入,来源为SPI通讯的结构
*
* RETURNS: TRUE，开入量闭合
*                 FALSE，开入量打开
*
*/
BOOL RD_Get_Src_DI(void *pSrc)
{
    RD_LGC_DI_CH *plgcdi;
    DI_CHANNEL *pdich;
    uint32_t ulChgTime;
    US_CNT_UTC_TIME utChgtm;

    plgcdi = (RD_LGC_DI_CH *)pSrc;
    pdich = (DI_CHANNEL *)plgcdi->pvSrc;

    if(plgcdi->mod==RD_HDL_BOX_DI)
    {
        return (plgcdi->iVal&0x7FFF);
    }
    else
    {
        return SIO_Get_DI(pdich, &ulChgTime, &utChgtm);
    }
}

