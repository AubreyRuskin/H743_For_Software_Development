/* spi_mutual.c - subroutine library for handling SPI and IO Module mutual operation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 25may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling SPI and IO Module mutual operation.
INCLUDE: spi_mutual.h
*/

/* includes */

#include "spi_mutual.h"
#include "spiio.h"
#include "semLib.h"
#include "realdata.h"
#include "errtest.h"
#include "logmsg.h"
// #include "sbcm8260Sio.h"
// #include "config04.h"   /* 底层提供头文件 */
// #include "sbcm8260Sio.h"
// #include "config04.h"   /* 底层提供头文件 */
#include "EdpVer.h"
#include "taskLib.h"

// #include <drv/intrCtl/m8260IntrCtl.h>
// #include <tickLib.h>

// void SPI_Write(int iModAddr, uint8_t *pucData){
//     return;
// }

#define MAX_IOCONFIRM_CNT 16	/* IO板确认次数*/

/* static funcitons */

/* change the base register of IO module.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_ChgSnBkPos(SPI_IO_BUF *pspibuf, uint8_t ucBase);

/* set adjusting command.
 * Para:
 *     ucModAddr, address.
 *     ucAdjCtrl, adjusting control word.
 *     ucChnStrl, channel control word.
 * Return: NONE.
 */
static void IO_SetAdjCmdOnCom(uint8_t ucModAddr, uint8_t ucAdjCtrl, uint8_t ucChnStrl);

/* get adjusting command.
 * Para:
 *     ucModAddr, address.
 * Return: TRUE, or FALSE.
 */
static BOOL IO_GetAdjCmdOnCom(uint8_t ucModAddr);

/* get the fault information of a special IO module.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetIoFaultInfo(SPI_IO_BUF *pspibuf);

/* get the DI status of DO board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetDiStsOnDo(SPI_IO_BUF *pspibuf);

/* get the DI status of CKDO board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetDiStsOnCKDo(SPI_IO_BUF *pspibuf);

/* send adjusting command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SndAdjCmdOnCom(SPI_IO_BUF *pspibuf);

/* set DO board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetDoAttr(SPI_IO_BUF *pspibuf);

/* set CKDIO board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetCKDioAttr(SPI_IO_BUF *pspibuf);

/* set DI board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetCKDiAttr(SPI_IO_BUF *pspibuf);

/* get the DI status on common board.
 * Para:
 *     pspibuf, IO buffer.
 * Return:
 *     NONE.
 */
static void IO_GetDiStsOnCom(SPI_IO_BUF *pspibuf);

/* change the AI channel number on common board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_ChgAiChnOnCom(SPI_IO_BUF *pspibuf);

/* functions */

/***********************************************************************
* ShowIOSts - 显示IO模件状态
*
* RETURNS: 无
*
*/
void ShowIOSts(void)
{
    int i;

    LOG_Dbg_Msg("SPI接收中断次数%d.\n\n", spiinfo.ulIsrCnt, 0, 0, 0, 0, 0);

    for (i=0; i<MAX_MOD_NUM; i++)
    {
        if (aspibuf_g[i].bUsed)
        {
            LOG_Dbg_Msg("模件%d状态: 模件类型为%s, 是否使用%d, 软件版本%x, 校验模式%d(%d).\n",
                        i+1, (int)IO_GetModDesInfo(aspibuf_g[i].ucModType), aspibuf_g[i].bUsed, aspibuf_g[i].unVer,
                        aspibuf_g[i].bCRCCheckMod,
                        aspibuf_g[i].bCheckModAffirm);

            LOG_Dbg_Msg("模件资源: AI = %d, AO = %d, DI = %d, DO = %d.\n",
                        aspibuf_g[i].unAiChNum, aspibuf_g[i].unAoChNum, aspibuf_g[i].unDiChNum, aspibuf_g[i].unDoChNum, 0, 0);

            LOG_Dbg_Msg("本模件通讯状态: 异常状态%d, 总异常次数%d, 校验和检验出错次数%d, 重启次数%d, 是否缺省值%d, 缺省值切换次数%d.\n",
                        aspibuf_g[i].bDIDOExecFlag,
                        aspibuf_g[i].ulTotalExcCnt,
                        aspibuf_g[i].ulCheckSumErrStat,
                        aspibuf_g[i].ulRebootCnt,
                        aspibuf_g[i].bDefaultFlag,
                        aspibuf_g[i].ulSwitchDefaultCnt);

            LOG_Dbg_Msg("本次发送数据帧内容: %x %x %x %x %x %x.\n",
                        aspibuf_g[i].aucDownFrame[0],
                        aspibuf_g[i].aucDownFrame[1],
                        aspibuf_g[i].aucDownFrame[2],
                        aspibuf_g[i].aucDownFrame[3],
                        aspibuf_g[i].aucDownFrame[4],
                        aspibuf_g[i].aucDownFrame[5]);

            LOG_Dbg_Msg("本次接收数据帧内容: %x %x %x %x %x %x.\n",
                        aspibuf_g[i].aucUpFrame[0],
                        aspibuf_g[i].aucUpFrame[1],
                        aspibuf_g[i].aucUpFrame[2],
                        aspibuf_g[i].aucUpFrame[3],
                        aspibuf_g[i].aucUpFrame[4],
                        aspibuf_g[i].aucUpFrame[5]);

            if (aspibuf_g[i].ulCheckSumErrStat)
            {
                LOG_Dbg_Msg("异常接收数据帧内容: %x %x %x %x %x %x.\n",
                            aspibuf_g[i].aucUpErrFrame[0],
                            aspibuf_g[i].aucUpErrFrame[1],
                            aspibuf_g[i].aucUpErrFrame[2],
                            aspibuf_g[i].aucUpErrFrame[3],
                            aspibuf_g[i].aucUpErrFrame[4],
                            aspibuf_g[i].aucUpErrFrame[5]);
            }
            LOG_Dbg_Msg("版本号确定上行校验模式%d, 下行帧校验出错次数%d.\n",
                        aspibuf_g[i].bIOCheckMod, aspibuf_g[i].ulUpChkErr, 0, 0, 0, 0);

            LOG_Dbg_Msg("SUM Check = %d CRC Check = %d\n", aspibuf_g[i].ulSumCheckCnt, aspibuf_g[i].ulCRCCheckCnt,
                        0, 0, 0, 0);

            LOG_Dbg_Msg("\n", 0, 0, 0, 0, 0, 0);
        }
    }
}

/* stop SPI communication.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void IO_StopSpi(void)
{
    // intDisable(INUM_TIMER3);		/* Disable timer3 interrupt */
    // intDisable(INUM_TIMER3);		/* Disable timer3 interrupt */
}

/* start SPI communication.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void IO_StartSpi(void)
{
    // intEnable(INUM_TIMER3);		/* Enable timer3 interrupt */
    // intEnable(INUM_TIMER3);		/* Enable timer3 interrupt */
}

/* change the base register of IO module.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_ChgSnBkPos(SPI_IO_BUF *pspibuf, uint8_t ucBase)
{
    uint8_t aucBuf[6];

    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
    aucBuf[1] = 0;   		/* according to 0x09. */
    aucBuf[2] = SPI_BYTES(4) | ucBase;
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* set adjusting command.
 * Para:
 *     ucModAddr, address.
 *     ucAdjCtrl, adjusting control word.
 *     ucChnStrl, channel control word.
 * Return: NONE.
 */
static void IO_SetAdjCmdOnCom(uint8_t ucModAddr, uint8_t ucAdjCtrl, uint8_t ucChnStrl)
{
    // SPI_IO_BUF *pspibuf;
    // int iLockKey;

    // LOG_Dbg_Msg("IO_SetAdjCmdOnCom: %d %x %x.\n", ucModAddr, ucAdjCtrl, ucChnStrl, 0, 0, 0);
    // iLockKey=intLock();		/* lock interrupt. */
    // pspibuf=aspibuf_g+ucModAddr;
    // pspibuf->adjattr.ucAdjCtrl=ucAdjCtrl;
    // pspibuf->adjattr.ucChnStrl=ucChnStrl;
    // pspibuf->adjattr.uccmdType = 0;
    // pspibuf->ulCmdSts |= IO_COM_ADJ_CMD;
    // pspibuf->ulCmdSndCnt[3] = spiinfo.ulCommandDelayCnt;
    // pspibuf->bCmdExecuteResult[3] = FALSE;
    // intUnlock(iLockKey);
}

/* get adjusting command.
 * Para:
 *     ucModAddr, address.
 * Return: TRUE, or FALSE.
 */
static BOOL IO_GetAdjCmdOnCom(uint8_t ucModAddr)
{
    SPI_IO_BUF *pspibuf;

    pspibuf=aspibuf_g+ucModAddr;
    pspibuf->adjattr.ucAdjCtrl=0x00;
    pspibuf->adjattr.ucChnStrl=0x00;

    if (pspibuf->ulCmdSts&IO_COM_ADJ_CMD)
    {
        pspibuf->ulCmdSts &= ~IO_COM_ADJ_CMD;

        return FALSE;
    }
    else
    {
        if (pspibuf->bCmdExecuteResult[3])
        {
            pspibuf->bCmdExecuteResult[3] = FALSE;

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

/* get the fault information of a special IO module.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetIoFaultInfo(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
    aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
    aucBuf[2] = SPI_BYTES(4) | DO_INVALID_STATUS_REG;		/* 开出板失效状态寄存器开始 */
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* get the DI status of DO board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetDiStsOnDo(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    /* 通知允许DO板上送系统事件，开出模件的输入监视，通道数，IO故障信息 */
    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
    aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
    aucBuf[2] = SPI_BYTES(4) | DO_SYS_EVENT;				/* 开出板系统事件寄存器开始 */
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* get the DI status of CKDO board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_GetDiStsOnCKDo(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    /* 通知允许CKDO板上送开出返回，及IO故障信息 */
    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
    aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
    aucBuf[2] = SPI_BYTES(4) | CKDOFEEDBAKC1;	/* 测控开出板返回字节1开始 */
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* send adjusting command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SndAdjCmdOnCom(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    if (pspibuf->adjattr.uccmdType == 0)
    {
        aucBuf[0] = SPI_BYTES(2) | IO_ADJ_CTRL;
        aucBuf[1] = pspibuf->adjattr.ucAdjCtrl;
        aucBuf[2] = pspibuf->adjattr.ucChnStrl;
        aucBuf[3] = 0x5A;
        aucBuf[4] = 0xA5;
        SPI_Write(pspibuf->ucModAddr, aucBuf);
    }
    else if (pspibuf->adjattr.uccmdType == 1)  /* send back the adjusting result. */
    {
        IO_ChgSnBkPos(pspibuf, IO_ADJ_CTRL);
        pspibuf->bChgBaseReg = TRUE;
    }

    pspibuf->adjattr.uccmdType++;
}

/* set DO board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetDoAttr(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
    aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
    aucBuf[2] = SPI_BYTES(4) | DO_SYS_EVENT;				/* 开出板系统事件寄存器开始 */
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* set CKDIO board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetCKDioAttr(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
    aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
    aucBuf[2] = SPI_BYTES(4) | CKDOFEEDBAKC1;	/* 测控开出板返回字节1开始 */
    aucBuf[3] = 0x5A;
    aucBuf[4] = 0xA5;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* set DI board attribution.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_SetCKDiAttr(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
    aucBuf[1] = 0x00;    /* 按上行帧属性字执行 */
    aucBuf[2] = SPI_BYTES(4) | DI_INPUT_REG;
    aucBuf[3] = 0xA5;
    aucBuf[4] = 0x5A;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
}

/* get the DI status on common board.
 * Para:
 *     pspibuf, IO buffer.
 * Return:
 *     NONE.
 */
static void IO_GetDiStsOnCom(SPI_IO_BUF *pspibuf)
{
    if (pspibuf->bCycleCommandSndFlag || (!pspibuf->bGetDiFlag))
    {
        return;
    }

    if (pspibuf->bChgBaseReg)
    {
        LOG_Dbg_Msg("重新设定IO模件发送基址寄存器.\n", 0, 0, 0, 0, 0, 0);
        pspibuf->bChgBaseReg = FALSE;
        IO_ChgSnBkPos(pspibuf, DI_INPUT_GROUP1_BASE_REG);
        pspibuf->bCycleCommandSndFlag = TRUE;

        return;
    }

    if (pspibuf->ucDiGroupNum == 0)
    {
        IO_ChgSnBkPos(pspibuf, DI_INPUT_GROUP1_BASE_REG);
    }
    else if (pspibuf->ucDiGroupNum == 1)
    {
        IO_ChgSnBkPos(pspibuf, DI_INPUT_GROUP2_BASE_REG);
    }

    pspibuf->bCycleCommandSndFlag = TRUE;

    pspibuf->ucDiGroupNum++;

    if (pspibuf->ucDiGroupNum == GROUP_NUM_COM_DI)
    {
        pspibuf->ucDiGroupNum = 0;

        if (pspibuf->unAiChNum)
        {
            pspibuf->bGetDiFlag = FALSE;
            pspibuf->bGetAiFlag = TRUE;
        }
    }

    return;
}

/* change the AI channel number on common board.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
static void IO_ChgAiChnOnCom(SPI_IO_BUF *pspibuf)
{
    uint8_t aucBuf[6];

    if (pspibuf->bCycleCommandSndFlag
            || (!pspibuf->bGetAiFlag))
    {
        return;
    }

    if (pspibuf->bChgBaseReg)
    {
        LOG_Dbg_Msg("重新设定IO模件发送基址寄存器.\n", 0, 0, 0, 0, 0, 0);
        pspibuf->bChgBaseReg = FALSE;
        IO_ChgSnBkPos(pspibuf, IO_CHN_RD_CTRL);
        pspibuf->bCycleCommandSndFlag = TRUE;

        return;
    }

    if (!pspibuf->ucAiChnCnt)
    {
        /* the first. */
        IO_ChgSnBkPos(pspibuf, IO_CHN_RD_CTRL);
        pspibuf->bCycleCommandSndFlag = TRUE;
        pspibuf->ucAiChnCnt++;

        return;
    }

    aucBuf[0] = SPI_BYTES(2) | IO_CHN_RD_CTRL;
    aucBuf[1] = 2;
    aucBuf[2] = pspibuf->ucAiChnCnt;		/* read channel. */
    aucBuf[3] = 0xA5;
    aucBuf[4] = 0x5A;
    SPI_Write(pspibuf->ucModAddr, aucBuf);
    pspibuf->bCycleCommandSndFlag = TRUE;

    pspibuf->ucAiChnCnt++;

    if (pspibuf->ucAiChnCnt == (pspibuf->unAiChNum+1))
    {
        pspibuf->ucAiChnCnt = 0;

        if (pspibuf->unDiChNum)
        {
            pspibuf->bGetAiFlag = FALSE;
            pspibuf->bGetDiFlag = TRUE;
        }
    }

    return;
}

/* clear common command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
void IO_ClrComCmd(SPI_IO_BUF *pspibuf)
{
    char TempInfo[256];

    if (pspibuf->ulCmdSts&IO_GET_FAULT_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | DO_INVALID_STATUS_REG))
        {
            pspibuf->ulIOConfirmCnt[0]++;
            if(pspibuf->ulIOConfirmCnt[0] > MAX_IOCONFIRM_CNT)
            {
                pspibuf->ulCmdSts &= ~IO_GET_FAULT_CMD;
            }
        }
        else if (!pspibuf->ulCmdSndCnt[0])  /* send command for many times. */
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_DIDO_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "模件地址:%d,错误码:%02d\n",
                           pspibuf->ucModAddr+1, DO_PROBLOM);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_DIDO_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "Module address:%d, error code:%02d\n",
                           pspibuf->ucModAddr+1, DO_PROBLOM);
            }
            sprintf(TempInfo, "%d#开出模件自检异常,原因未明!!\n", pspibuf->ucModAddr+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            pspibuf->ulCmdSts &= ~IO_GET_FAULT_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[0]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_GET_DI_ONDO_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | DO_SYS_EVENT))
        {
            pspibuf->ulIOConfirmCnt[1]++;
            if(pspibuf->ulIOConfirmCnt[1] > MAX_IOCONFIRM_CNT)
            {
                LOG_Dbg_Msg("获取开出板开入成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_GET_DI_ONDO_CMD;
            }
        }
        else if (!pspibuf->ulCmdSndCnt[1])
        {
            LOG_Dbg_Msg("长时间没有获取开出板开入.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_GET_DI_ONDO_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[1]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_GET_DI_CKDO_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | CKDOFEEDBAKC1))
        {
            pspibuf->ulIOConfirmCnt[2]++;
            if(pspibuf->ulIOConfirmCnt[2] > MAX_IOCONFIRM_CNT)
            {
                LOG_Dbg_Msg("获取测控板开入成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_GET_DI_CKDO_CMD;
            }
        }
        else if (!pspibuf->ulCmdSndCnt[2])
        {
            LOG_Dbg_Msg("长时间没有获取测控板开入.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_GET_DI_CKDO_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[2]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_COM_ADJ_CMD)
    {
        if ((pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | IO_ADJ_CTRL))
                && (pspibuf->aucUpFrame[1] == 0x00)
                && (pspibuf->aucUpFrame[2] == 0x00))
        {
            static uint32_t ulSuccessCnt = 0;

            ulSuccessCnt++;
            if (ulSuccessCnt>CMD_EXCUTE_ACK_NUM)
            {
                LOG_Dbg_Msg("校准成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_COM_ADJ_CMD;
                pspibuf->bCmdExecuteResult[3] = TRUE;
                pspibuf->ulCmdSndCnt[3] = 0;
                ulSuccessCnt = 0;
            }
        }
        else if (!pspibuf->ulCmdSndCnt[3])
        {
            LOG_Dbg_Msg("校准长时间没响应.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_COM_ADJ_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[3]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_SET_DO_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | DO_SYS_EVENT))
        {
            pspibuf->ulIOConfirmCnt[4]++;
            if(pspibuf->ulIOConfirmCnt[4] > MAX_IOCONFIRM_CNT)
            {
                LOG_Dbg_Msg("IO板已达到最大确认次数16\n", 0, 0, 0, 0, 0, 0);

                LOG_Dbg_Msg("设置DO模件成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_SET_DO_CMD;
                if (pspibuf->bIORebootFlag)
                {
                    pspibuf->bIORebootFlag = FALSE;
                }
            }
        }
        else if (!pspibuf->ulCmdSndCnt[4])
        {
            LOG_Dbg_Msg("设置DO模件长时间没有响应.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_SET_DO_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[4]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_SET_CKDIO_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | CKDOFEEDBAKC1))
        {
            pspibuf->ulIOConfirmCnt[5]++;
            if(pspibuf->ulIOConfirmCnt[5] > MAX_IOCONFIRM_CNT)
            {
                LOG_Dbg_Msg("设置CKDO模件成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_SET_CKDIO_CMD;
                if (pspibuf->bIORebootFlag)
                {
                    pspibuf->bIORebootFlag = FALSE;
                }
            }
        }
        else if (!pspibuf->ulCmdSndCnt[5])
        {
            LOG_Dbg_Msg("设置CKDO模件长时间没有响应.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_SET_CKDIO_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[5]--;
        }
    }
    else if (pspibuf->ulCmdSts&IO_SET_DI_CMD)
    {
        if (pspibuf->aucUpFrame[0] == (SPI_BYTES(4) | DI_INPUT_REG))
        {
            pspibuf->ulIOConfirmCnt[6]++;
            if(pspibuf->ulIOConfirmCnt[6] > MAX_IOCONFIRM_CNT)
            {
                LOG_Dbg_Msg("设置DI模件成功.\n", 0, 0, 0, 0, 0, 0);
                pspibuf->ulCmdSts &= ~IO_SET_DI_CMD;
                if (pspibuf->bIORebootFlag)
                {
                    pspibuf->bIORebootFlag = FALSE;
                }
            }
        }
        else if (!pspibuf->ulCmdSndCnt[6])
        {
            LOG_Dbg_Msg("设置DI模件长时间没有成功.\n", 0, 0, 0, 0, 0, 0);
            pspibuf->ulCmdSts &= ~IO_SET_DI_CMD;
        }
        else
        {
            pspibuf->ulCmdSndCnt[6]--;
        }
    }
}

/* the second board get the overflow value.
 * Para:
 *     pspibuf, IO buffer.
 *     pSrc, Rcv spi packet buf
 * Return: NONE.
 */
void IO_Num_2_Get_OvVal(SPI_IO_BUF *pspibuf, uint8_t *pSrc)
{
    int i = 0;
    uint8_t ucSum = 0;
    uint8_t acBuf[6];
    uint8_t *pSrcTmp = NULL;

    pSrcTmp = pSrc;

    for (i =0; i < 5; i++, pSrcTmp +=16)
    {
        acBuf[i] = *pSrcTmp;
        ucSum += *pSrcTmp;
    }
    acBuf[i] = *pSrcTmp;

    ucSum = (uint8_t)~ucSum;

    if(ucSum == acBuf[5])
    {
        if ((acBuf[0] == IO_CHN_RD_CTRL)		/* base register. */
                && (acBuf[1] == 0x01))	/* send overflow flag. */
        {
            pspibuf->aOvValBuf[acBuf[2]-1] = U8_TO_U16(acBuf[4], acBuf[3]);
        }
    }
}

/* send common command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
void IO_SndComCmd(SPI_IO_BUF *pspibuf)
{
    if (pspibuf->ulCmdSts && spiinfo.bIOInitFinish
            && (!pspibuf->bSetDoFlag))
    {
        if (pspibuf->ulCmdSts&IO_GET_FAULT_CMD)
        {
            IO_GetIoFaultInfo(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_GET_DI_ONDO_CMD)
        {
            IO_GetDiStsOnDo(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_GET_DI_CKDO_CMD)
        {
            IO_GetDiStsOnCKDo(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_COM_ADJ_CMD)
        {
            IO_SndAdjCmdOnCom(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_SET_DO_CMD)
        {
            IO_SetDoAttr(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_SET_CKDIO_CMD)
        {
            IO_SetCKDioAttr(pspibuf);
        }
        else if (pspibuf->ulCmdSts&IO_SET_DI_CMD)
        {
            IO_SetCKDiAttr(pspibuf);
        }

        pspibuf->bSetCmdFlag = TRUE;
    }
    else if (pspibuf->bSetCmdFlag)
    {
        if ((pspibuf->ucModType == DO_MODULE)
                || (pspibuf->ucModType == DIO_MODULE)
                || (pspibuf->ucModType == CKDIO_MODULE))
        {
            uint8_t aucBuf[6];

            aucBuf[0] = SPI_BYTES(4) | DO_OUTPUT_REG;
            aucBuf[1] = LL8(pspibuf->modinfo.domod.ulSts);
            aucBuf[2] = LH8(pspibuf->modinfo.domod.ulSts);
            aucBuf[3] = HL8(pspibuf->modinfo.domod.ulSts);
            aucBuf[4] = HH8(pspibuf->modinfo.domod.ulSts);

            SPI_Write(pspibuf->ucModAddr, aucBuf);
        }
        pspibuf->bSetCmdFlag = FALSE;
    }
}

/* send periodic command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
void IO_SndPeriodCmd(SPI_IO_BUF *pspibuf)
{
    if ((pspibuf->ucModType == COM_MODULE)
            && (!pspibuf->ulCmdSts) && spiinfo.bIOInitFinish   /* no common command. */
            && (!pspibuf->bSetDoFlag))
    {
        pspibuf->bCycleCommandSndFlag = FALSE;

        if (pspibuf->unDiChNum)
        {
            IO_GetDiStsOnCom(pspibuf);
        }

        if (pspibuf->unAiChNum)
        {
            IO_ChgAiChnOnCom(pspibuf);
        }
    }
}

/* adjusting, called by MMI.
 * Para:
 *     ucModAddr, address.
 *     ucAdjCtrl, adjusting control word.
 *     ucChnStrl, channel control word.
 * Return: EP_SUCCESS, EP_ERROR.
 */
EP_STATUS IO_adjust(uint8_t ucModAddr, uint8_t ucAdjCtrl, uint8_t ucChnStrl)
{
    SPI_IO_BUF *pspibuf;
    uint8_t ucCnt = 0;
    uint8_t ucMaxCnt = 0;

    IO_SetAdjCmdOnCom(ucModAddr, ucAdjCtrl, ucChnStrl);

    if (ucChnStrl == 0xFF)
    {
        ucMaxCnt = 40;
    }
    else
    {
        ucMaxCnt = 30;
    }

    pspibuf = aspibuf_g+ucModAddr;
    while ((!pspibuf->bCmdExecuteResult[3])
            && (ucCnt<ucMaxCnt))
    {
        taskDelay(10);
        ucCnt++;
    }

    LOG_Dbg_Msg("校准时间%dms.\n", 100*ucCnt, 0, 0, 0, 0, 0);

    if (IO_GetAdjCmdOnCom(ucModAddr))
    {
        if (ucChnStrl == 0xff)
        {
            LOG_Dbg_Msg("模件%d所有通道校准成功.\n", ucModAddr, 0, 0, 0, 0, 0);
        }
        else
        {
            LOG_Dbg_Msg("模件%d通道%d校准成功.\n", ucModAddr, ucChnStrl, 0, 0, 0, 0);
        }

        return EP_SUCCESS;
    }
    else
    {
        if (ucChnStrl == 0xff)
        {
            LOG_Dbg_Msg("模件%d所有通道校准失败.\n", ucModAddr, 0, 0, 0, 0, 0);
        }
        else
        {
            LOG_Dbg_Msg("模件%d通道%d校准失败.\n", ucModAddr, ucChnStrl, 0, 0, 0, 0);
        }

        return EP_ERROR;
    }
}

/***********************************************************************
* SetMBDoTest - 获取母板开出(电铁使用)
*
* RETURNS: EP_ERROR, EP_SUCCESS
*
*/
EP_STATUS SetMBDoTest(
    int iSetVal		/* 设定值 */
)
{
    int iReVal;

    // iReVal=IoPinOutputHigh(IO_OUT_CONNECTOR_DO, iSetVal);
    if(iReVal != iSetVal)
    {
        return EP_ERROR;
    }
    else
    {
        return EP_SUCCESS;
    }
}

/***********************************************************************
* SetOptCoupleEnable - 设置光耦是否导通(电铁使用)
*
* RETURNS: 无
*
*/
void SetOptCoupleEnable(
    BOOL bSts		/* 设置值 */
)
{
    // IoPinOutputHigh(IO_OUT_CONNECTOR_DO, bSts);
}

/* get the IO module type description information.
 * Para:
 *     ucModType, module type.
 * Return:
 *     pointer to description information.
 */
uint8_t *IO_GetModDesInfo(uint8_t ucModType)
{
    if (ENG_MODE == 0)
    {
        switch (ucModType)
        {
            case IDLE_MODULE:
                return "空模件";

            case DO_MODULE:
                return "开出模件";

            case DI_MODULE:
                return "开关量输入模件";

            case DIO_MODULE:
                return "开关量输入输出模件";

            case AI_MODULE:
                return "模拟量输入模件";

            case AO_MODULE:
                return "模拟量输出模件";

            case CKDIO_MODULE:
                return "测控开出模件";

            case COM_MODULE:
                return "通用模件";

            default:
                return "空模件";
        }
    }
    else if (ENG_MODE == 1)
    {
        switch (ucModType)
        {
            case IDLE_MODULE:
                return "Null module";

            case DO_MODULE:
                return "Digit output module";

            case DI_MODULE:
                return "Digit input module";

            case DIO_MODULE:
                return "Digit input and output module";

            case AI_MODULE:
                return "Analog input module";

            case AO_MODULE:
                return "Analog output module";

            case CKDIO_MODULE:
                return "Cekong used digit input and output module";

            case COM_MODULE:
                return "Common module.";

            default:
                return "Null module";
        }
    }

    return NULL;
}