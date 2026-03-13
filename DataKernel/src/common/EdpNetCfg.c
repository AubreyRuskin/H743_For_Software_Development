/* EdpNegCfg.c - subroutine library for managing net configuration of CPU. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 11aug07, zy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for managing net configuration of CPU.
INCLUDES: EdpNetCfg.h
*/

/* includes */

#include <ioLib.h>
#include "EdpNetCfg.h"
#include "errtest.h"
#include "logmsg.h"
#include  "filetool.h"
#include "EdpVer.h"
#include "stdio_compat.h"
#include <logLib.h>

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "config05.h"
#endif

/* globals */

EDP_NET_CFG_INFO NT_NetCfgInf_g;
BOOL NT_bNetCfgIsInit_g=FALSE;
char *deviceName[] = {"motfcc1","motfcc0","motscc0"};   /* to indicate config string, motfcc1: Net1; motfcc0: Net2; motscc: Net3 */

/* global functions */

/*NOTE:  Set_EthIP and Set_EthMacAdrs must be called before Init_Net
*/
/*  Function:   Set ethernet mac address
    Parameter:  port: begin from 0
                addr: pointer to buffer holding last three bytes of mac address
    Return value:   ERROR;  no mac address set
                    OK;     set mac address ok
    Note:   if you want set the MAC address as
        xx.xx.xx.12.34.56
        you should set addr[0] = 12; addr[1] = 34; addr[2] = 56
*//*见goose_eth.h  BSP提供*/
extern  int Set_EthMacAdrs(unsigned char port, unsigned char *addr);

/*  Function:   Set ethernet IP
    Parameter:  port: begin from 0
                addr: pointer to buffer holding IP address
    Return value:   ERROR;  no IP set
                    OK;     set IP ok
    Note:   if you want set the IP address as
        172.30.20.56
        you should set addr[0] = 172; addr[1] = 30; addr[2] = 20; addr[3] = 56
*//*见goose_eth.h  BSP提供*/
extern  int Set_EthIP(unsigned char port, unsigned char *addr);


/*  Function:   Get ethernet MAC address
    Parameter:  portNum;ETHERNET_GOOSE_PORT
                addr: pointer to buffer to get mac address
    Return value: OK, or ERROR if the Ethernet address cannot be returned.
*/    /*见goose_eth.h BSP提供*/
extern  STATUS etherMacAddrGet(uint8_t portNum, uint8_t *addr);


/* 功能，读取解析文件中保存的网络IP配置
   参数，iNetSeqNo，网络号，从0开始
         pucRtIpAddr，返回IP地址的字符数组首址，字符串空间，由调用方分配
         iAddrLen,   传送的字符数组长度。
   返回：EP_SUCCESS:解析成功
         其他，解析失败*/
static  EP_STATUS  NT_RslvFileNetIpCfg(int  iNetSeqNo,uint8_t   *pucRtIpAddr,int  iAddrLen);

/* 功能，读取解析文件中保存的网络IP Mask配置
   参数，iNetSeqNo，网络号，从0开始
         pucRtIpMaskAddr，返回IP Mask的字符数组首址，字符串空间，由调用方分配
         iAddrLen,   传送的字符数组长度。
   返回：EP_SUCCESS:解析成功
         其他，解析失败*/
static  EP_STATUS  NT_RslvFileNetIpMaskCfg(
    int iNetSeqNo,
    uint8_t *pucRtIpMaskAddr,
    int iAddrLen
);

/* 功能，保存文件中的网络IP配置
   参数，iNetSeqNo，网络号，从0开始
         pucRtIpAddr，传送IP地址的字符数组首址，字符串空间，由调用方分配
         iAddrLen,   传送的字符数组长度。
   返回：EP_SUCCESS:保存成功
         其他，保存失败*/
static  EP_STATUS  NT_SaveFileNetIpCfg(int  iNetSeqNo,uint8_t   *pucIpAddr,int  iAddrLen);

/*功能,设置某网口的MAC地址
  参数,  iNetSeqNo,网络号,从0开始
         pMacAddrBase,Mac地址串基址*/
static  EP_STATUS   NT_SetOneNetMacAddr(int  iNetSeqNo,uint8_t *pMacAddrBase);

/***********************************************************************
* NT_SaveFileNetIpMaskCfg - 保存文件中的子网掩码配置
*
* RETURNS:
*        EP_SUCCESS: 保存成功
*        其他: 保存失败
*
*/
EP_STATUS NT_SaveFileNetIpMaskCfg(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pucIpMask,				/* 传送子网掩码的字符数组首址，字符串空间，由调用方分配 */
    int iAddrLen		/* 传送的字符数组长度 */
);

/* functions */

#ifdef EXCITE_BUILD

#ifndef VXWORKS_ROM		/* 非VXWORKS_ROM中没有提供，临时提供以防止编译出错 */
/*  Function:   Get ethernet MAC address
    Parameter:  portNum;ETHERNET_GOOSE_PORT
                addr: pointer to buffer to get mac address
    Return value: OK, or ERROR if the Ethernet address cannot be returned.
*/
STATUS etherMacAddrGet(uint8_t portNum, uint8_t *addr)
{
    return OK;
}

/*NOTE:  Set_EthIP and Set_EthMacAdrs must be called before Init_Net
*/
/*  Function:   Set ethernet mac address
    Parameter:  port: begin from 0
                addr: pointer to buffer holding last three bytes of mac address
    Return value:   ERROR;  no mac address set
                    OK;     set mac address ok
    Note:   if you want set the MAC address as
        xx.xx.xx.12.34.56
        you should set addr[0] = 12; addr[1] = 34; addr[2] = 56
*/
int Set_EthMacAdrs(unsigned char port, unsigned char *addr)
{
}

/*  Function:   Set ethernet IP
    Parameter:  port: begin from 0
                addr: pointer to buffer holding IP address
    Return value:   ERROR;  no IP set
                    OK;     set IP ok
    Note:   if you want set the IP address as
        172.30.20.56
        you should set addr[0] = 172; addr[1] = 30; addr[2] = 20; addr[3] = 56
*/
int Set_EthIP(unsigned char port, unsigned char *addr)
{
}

#endif

#endif

#ifndef EDP03_BUILD
/* 设定Mask
 * 参数:
 *     port, 端口号
 *     addrMask, 子网掩码
 */
STATUS Set_EthMask (unsigned char port, uint32_t addrMask)
{
    return OK;
}
#else
int Set_EthMask(unsigned char port, unsigned int dwval);
#endif

/*  Function:   Set ethernet IP Mask
    Parameter:  port: begin from 0
                addr: pointer to buffer holding IP Mask
    Return value:   ERROR;  no IP Mask set
                    OK;     set IP Mask ok
    Note:   if you want set the IP Mask address as
        255.255.255.0
        you should set addrMask[0] = 255; addrMask[1] = 255; addrMask[2] = 255; addrMask[3] = 0
*/
int Set_EthIPMask(
    unsigned char port,
    unsigned char *addrMask
)
{
    uint32_t ulMask;

    assert(port<3);		/* 最多3个网口 */
    assert(addrMask);
    ulMask=U8_TO_U32(addrMask[0],addrMask[1],
                     addrMask[2],addrMask[3]);

    return Set_EthMask(port, ulMask);
}

/* 初始化网络配置, 在文件系统初始化之后，Init_Net之前调用，才起作用
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS NT_NetCfgInit(void)
{
    int k;
    int m;
    uint8_t aucMacAddr[3];
    ONE_NET_CFG_INFO *pOneNetCfg;
    BOOL bInitIsSuccess;
    char TempInfo[256];

    LOG_Dbg_Msg("Init Net Cfg\n", 0, 0, 0, 0, 0, 0);

    bInitIsSuccess = TRUE;
    NT_NetCfgInf_g.iValidNetNum = 0;

    for (k=0; k<2; k++, NT_NetCfgInf_g.iValidNetNum++)
    {
        pOneNetCfg = NT_NetCfgInf_g.NetInfArr+NT_NetCfgInf_g.iValidNetNum;
        pOneNetCfg->iNetSeqNo = NT_NetCfgInf_g.iValidNetNum;

        if (NT_RslvFileNetIpCfg(k, pOneNetCfg->aucIpAddr, 4) != EP_SUCCESS)
        {
            if (k == 0)
            {
                pOneNetCfg->aucIpAddr[0] = 172;	/* 设置默认值 */
                pOneNetCfg->aucIpAddr[1] = 40;
                pOneNetCfg->aucIpAddr[2] = 20;
                pOneNetCfg->aucIpAddr[3] = 234;
            }
            else if (k == 1)
            {
                pOneNetCfg->aucIpAddr[0] = 192;	/* 设置默认值 */
                pOneNetCfg->aucIpAddr[1] = 168;
                pOneNetCfg->aucIpAddr[2] = 0;
                pOneNetCfg->aucIpAddr[3] = 123;
            }
            if (k == 0)
            {
                if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "system file Net1IP item format invalid or no set.\n", NULL);
                }
                else if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "系统文件 Net1IP 条目内容格式不对或没有设置.\n", NULL);
                }
            }
            else if (k == 1)
            {
                if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "system file Net2IP item format invalid or no set.\n", NULL);
                }
                else if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "系统文件 Net2IP 条目内容格式不对或没有设置.\n", NULL);
                }
            }
        } /* if结束 */

        if (NT_RslvFileNetIpMaskCfg(k, pOneNetCfg->aucIpMsk, 4) != EP_SUCCESS)
        {
            pOneNetCfg->aucIpMsk[0] = 255;
            pOneNetCfg->aucIpMsk[1] = 255;
            pOneNetCfg->aucIpMsk[2] = 255;
            pOneNetCfg->aucIpMsk[3] = 0;
            if (k == 0)
            {
                if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "system file file Net1IPMask item format invalid or no set.\n", NULL);
                }
                else if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "系统文件Net1IP条目内容异常\n", NULL);
                }
            }
            else if (k == 1)
            {
                if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "System file Net2IP item abnoamal\n",NULL);
                }
                else if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "系统文件Net2IP条目内容异常\n", NULL);
                }
            }
        }	/* if结束 */


#if defined(EDP03_BUILD)		/* 网口按照平台进行处理 */
        if (VER_GetHwBoardSN() == EDP03_CPU_A_A_200612_BORAD)
        {
            if ((ComVer_g == EDP03_COMA_A) || (ComVer_g == EDP03_COMD_A))
            {
                /* 两个485，一个232和一个485，没有网口 */
                continue;
            }
        }
        if (VER_GetHwBoardSN() == E03_CPU_B_A_200806_BORAD)
        {
            if ((ComVer_g == EDP03_COMJ_A))
            {
                /* 双485+232 */
                continue;
            }
        }

        if (VER_GetHwBoardSN() == EDP03_CPU_A_A_200612_BORAD)
        {
            if (ComVer_g == EDP03_COMC_A)
            {
                /* 一个232，一个以太网 */
                if (k == 0)
                {
                    /* 第一个网口不存在 */
                    continue;
                }
            }
        }

        if (VER_GetHwBoardSN() == E03_CPU_B_A_200806_BORAD)
        {
            if ((ComVer_g == EDP03_COMG_A) || (ComVer_g == EDP03_COMH_A))
            {
                /* 一以太网(电),一RS485(电)；一以太网(电),一RS485(光) */
                if (k == 0)
                {
                    /* 第一个网口不存在 */
                    continue;
                }
            }
        }
#endif

        if (Set_EthIP(k, pOneNetCfg->aucIpAddr) != OK)	/* 设置IP地址 */
        {
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"set ethernet %d IP addr failure\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"设置以太网%dIP地址失败\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            bInitIsSuccess = FALSE;
        }

        if (Set_EthIPMask(k, pOneNetCfg->aucIpMsk) != OK)  /* 设置子网掩码 */
        {
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"set ethernet %d IP Mask failure.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"设置以太网%d子网掩码失败.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            bInitIsSuccess = FALSE;
        }


        for (m=0; m<3; m++)
        {
            aucMacAddr[m] = pOneNetCfg->aucIpAddr[m+1];
        }

        if (Set_EthMacAdrs(k, aucMacAddr) != OK)  /* 设置MAC地址，由IP地址决定 */
        {
            /* 原为使用IP地址的最后三位设置 */
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"set ethernet %d MAC addr failure.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"设置以太网%dMAC地址失败\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

            }

            bInitIsSuccess=FALSE;
        }

        /* 获得实际的MAC地址，对实际的IP地址，目前BSP未提供该函数，下一版BSP提供该函数 */
        // if (etherMacAddrGet(k, pOneNetCfg->aucMacAddr) != OK)
        // {
        //     if (ENG_MODE == 1)
        //     {
        //         sprintf(TempInfo,"get ethernet %d MAC addr failure.\n", k);
        //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
        //     }
        //     else if (ENG_MODE == 0)
        //     {
        //         sprintf(TempInfo,"查询以太网%d MAC 地址失败.\n", k);
        //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
        //     }
        // }

        LOG_Dbg_Msg("Get Net Num %d Mac Addr, Last Three Mac Addr is %d: %d: %d.\n", k,
                    pOneNetCfg->aucMacAddr[3],
                    pOneNetCfg->aucMacAddr[4],
                    pOneNetCfg->aucMacAddr[5], 0, 0);
    } 	/* for (k=0; k<2; k++)结束 */

#if defined(EDP03_BUILD)
    if (ComVerExt_g == 0x02)
    {
        /* 有一个扩展网口 */
        pOneNetCfg = NT_NetCfgInf_g.NetInfArr+NT_NetCfgInf_g.iValidNetNum;
        pOneNetCfg->iNetSeqNo=NT_NetCfgInf_g.iValidNetNum;
        if (NT_RslvFileNetIpCfg(k, pOneNetCfg->aucIpAddr, 4) != EP_SUCCESS)
        {
            pOneNetCfg->aucIpAddr[0] = 172;	/* 设置默认值 */
            pOneNetCfg->aucIpAddr[1] = 30;
            pOneNetCfg->aucIpAddr[2] = 20;
            pOneNetCfg->aucIpAddr[3] = 234;

            if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Net3IP item abnormal.\n", NULL);
            }
            else if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "Net3IP 内容异常.\n", NULL);
            }
        }

        if (NT_RslvFileNetIpMaskCfg(k, pOneNetCfg->aucIpMsk, 4) != EP_SUCCESS)
        {
            pOneNetCfg->aucIpMsk[0] = 255;
            pOneNetCfg->aucIpMsk[1] = 255;
            pOneNetCfg->aucIpMsk[2] = 255;
            pOneNetCfg->aucIpMsk[3] = 0;
            if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "system file Net3IP Mask item format invalid or no set.\n", NULL);
            }
            else if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "系统文件 Net3IP Mask 条目内容格式不对或没有设置.\n", NULL);
            }
        }	/* if结束 */

        if (Set_EthIP(k, pOneNetCfg->aucIpAddr) != OK)	/* 设置IP地址 */
        {
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"set ethernet %d IP addr failure.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"设置以太网%d IP 地址失败.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            bInitIsSuccess = FALSE;
        }

        if (Set_EthIPMask(k, pOneNetCfg->aucIpMsk) != OK)				/* 设置子网掩码 */
        {
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"NET cfg failue: set ethernet %d IP Mask failure.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"网络配置失败: 设置以太网%d子网掩码失败.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            bInitIsSuccess = FALSE;
        }

        for (m=0; m<3; m++)
        {
            aucMacAddr[m] = pOneNetCfg->aucIpAddr[m+1];
        }

        if (Set_EthMacAdrs(k, aucMacAddr) != OK)  /* 设置MAC地址，由IP地址决定 */
        {
            /* 原为使用IP地址的最后三位设置 */
            if (ENG_MODE == 1)
            {
                sprintf(TempInfo,"set ethernet %d MAC addr failure.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }
            else if (ENG_MODE == 0)
            {
                sprintf(TempInfo,"设置以太网%d MAC地址失败.\n", k);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            bInitIsSuccess=FALSE;
        }

        /* 获得实际的MAC地址，对实际的IP地址，目前BSP未提供该函数，下一版BSP提供该函数 */
        // if (etherMacAddrGet(k, pOneNetCfg->aucMacAddr) != OK)
        // {
        //     if (ENG_MODE == 1)
        //     {
        //         sprintf(TempInfo,"get ethernet %d MAC addr failure.\n", k);
        //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
        //     }
        //     else if (ENG_MODE == 0)
        //     {
        //         sprintf(TempInfo,"查询以太网%d MAC 地址失败.\n", k);
        //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
        //     }
        // }

        LOG_Dbg_Msg("Get Net Num %d Mac Addr, Last Three Mac Addr is %d: %d: %d.\n", k,
                    pOneNetCfg->aucMacAddr[3],
                    pOneNetCfg->aucMacAddr[4],
                    pOneNetCfg->aucMacAddr[5], 0, 0);
    }
#endif
#if defined(EDP02_PSR_BUILD)
    pOneNetCfg=NT_NetCfgInf_g.NetInfArr+NT_NetCfgInf_g.iValidNetNum;
    pOneNetCfg->iNetSeqNo=NT_NetCfgInf_g.iValidNetNum;
    if(NT_RslvFileNetIpCfg(k, pOneNetCfg->aucIpAddr, 4) != EP_SUCCESS)
    {
        pOneNetCfg->aucIpAddr[0]=172;		/* 设置默认值 */
        pOneNetCfg->aucIpAddr[1]=50;
        pOneNetCfg->aucIpAddr[2]=20;
        pOneNetCfg->aucIpAddr[3]=234;

        LOG_Write(LOG_KERNEL,"网络配置错误: ini 文件 Net3IP 条目内容格式不对或没有设置!\n",NULL);
    }

    if(NT_RslvFileNetIpMaskCfg(k, pOneNetCfg->aucIpMsk, 4) != EP_SUCCESS)
    {
        pOneNetCfg->aucIpMsk[0]=255;
        pOneNetCfg->aucIpMsk[1]=255;
        pOneNetCfg->aucIpMsk[2]=255;
        pOneNetCfg->aucIpMsk[3]=0;
        LOG_Write(LOG_KERNEL,"网络配置错误: ini 文件 Net3IP MASK条目内容格式不对或没有设置!\n",NULL);
    }/* if结束 */

    if(Set_EthIP(k, pOneNetCfg->aucIpAddr) != OK)	/* 设置IP地址 */
    {
        LOG_Write(LOG_KERNEL,"网络配置错误: 设置 Net3IP 错误!\n",NULL);
        bInitIsSuccess=FALSE;
    }

    if(Set_EthIPMask(k, pOneNetCfg->aucIpMsk) != OK)				/* 设置子网掩码 */
    {
        LOG_Write(LOG_KERNEL,"网络配置错误: 设置 Net3IP MASK错误!\n",NULL);

        bInitIsSuccess=FALSE;
    }

    for(m=0; m<3; m++)
    {
        aucMacAddr[m]=pOneNetCfg->aucIpAddr[m+1];
    }

    if(Set_EthMacAdrs(k, aucMacAddr) != OK)			/* 设置MAC地址，由IP地址决定 */
    {
        /* 原为使用IP地址的最后三位设置 */
        LOG_Write(LOG_KERNEL,"网络配置失败:设置以太网3 MAC 地址失败!", NULL);

        bInitIsSuccess=FALSE;
    }

    // if(etherMacAddrGet(k, pOneNetCfg->aucMacAddr)!=OK)			/* 获得实际的MAC地址，对实际的IP地址，目前BSP未提供该函数，下一版BSP提供该函数 */
    // {
    //     LOG_Write(LOG_KERNEL,"网络配置失败:查询以太网3 MAC 地址失败!", NULL);
    // }

    logMsg("Get Net Num %d Mac Addr,Last Three Mac Addr  is %d:%d:%d \n",
           k,
           pOneNetCfg->aucMacAddr[3],
           pOneNetCfg->aucMacAddr[4],
           pOneNetCfg->aucMacAddr[5], 0, 0);

#endif

    NT_bNetCfgIsInit_g=TRUE;
    if(bInitIsSuccess)
    {
        return  EP_SUCCESS;
    }
    else
    {
        return EP_ERROR;
    }
}

/***********************************************************************
* NT_GetNetRunCfg - 获得网络实际运行配置
*
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
EP_STATUS NT_GetNetRunCfg(
    EDP_NET_CFG_INFO *pRtNetInfo		/* 供返回网络实际运行配置的变量地址，该变量本身由调用方来分配 */
)
{
    int  i;
    char TempInfo[256];

    if(!pRtNetInfo)
    {
        return  EP_ERROR;
    }

    if(NT_bNetCfgIsInit_g)
    {
        *pRtNetInfo=NT_NetCfgInf_g;
    }
    else
    {
        pRtNetInfo->iValidNetNum=2;
        for(i=0; i<pRtNetInfo->iValidNetNum; i++)
        {
            pRtNetInfo->NetInfArr[i].iNetSeqNo=i;
            pRtNetInfo->NetInfArr[i].aucIpAddr[0]=192;
            pRtNetInfo->NetInfArr[i].aucIpAddr[1]=168;
            pRtNetInfo->NetInfArr[i].aucIpAddr[2]=10;
            pRtNetInfo->NetInfArr[i].aucIpAddr[3]=200;
            /*
                  TODO, 获得实际IP地址
            */

            pRtNetInfo->NetInfArr[i].aucIpMsk[0]=255;
            pRtNetInfo->NetInfArr[i].aucIpMsk[1]=255;
            pRtNetInfo->NetInfArr[i].aucIpMsk[2]=255;
            pRtNetInfo->NetInfArr[i].aucIpMsk[3]=0;

            // if(etherMacAddrGet(i, pRtNetInfo->NetInfArr[i].aucMacAddr)!=OK)/*获得实际的MAC地址，对实际的IP地址，目前BSP未提供该函数，下一版BSP提供该函数  */
            // {
            //     if(ENG_MODE == 1)
            //     {
            //         sprintf(TempInfo,"get ethernet  %d MAC addr failure.\n", i);
            //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
            //     }
            //     else if(ENG_MODE == 0)
            //     {
            //         sprintf(TempInfo,"查询以太网%d MAC 地址失败.\n", i);
            //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
            //     }
            // }
        }
    }

    return  EP_SUCCESS;

}

/***********************************************************************
* NT_SetOneNetIpAddr - 设置某网口的IP地址，须重启后才起作用
*
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
EP_STATUS NT_SetOneNetIpAddr(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pIpAddrBase			/* IP地址串基址 */
)
{
    int  i=0;

    if(!pIpAddrBase)
    {
        return  EP_ERROR;
    }

    if(iNetSeqNo!=0&&iNetSeqNo!=1 && iNetSeqNo!=2)	/* 最多三个网口 */
    {

        if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Net Cfg Err:set ip net num over range.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "设置IP地址网口号超出范围.\n", NULL);
        }

        return  EP_ERROR;
    }

    for(i=0; i<4; i++)
    {
        NT_NetCfgInf_g.NetInfArr[iNetSeqNo].aucIpAddr[i]
            =pIpAddrBase[i];
    }

    if(NT_SaveFileNetIpCfg(iNetSeqNo,pIpAddrBase,4) != EP_SUCCESS)		/* 只需要保存1次 */
    {

        if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "NET IP set err.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "网口IP设置不成功.\n", NULL);
        }

        return  EP_ERROR;
    }

    return  EP_SUCCESS;

}

/*功能,设置某网口的MAC地址，目前由IP地址自动生成，不额外配置。
  参数,  iNetSeqNo,网络号,从0开始
         pMacAddrBase,Mac地址串基址*/
EP_STATUS   NT_SetOneNetMacAddr(int  iNetSeqNo,uint8_t *pMacAddrBase)
{
    /*目前空操作  */
    return  EP_SUCCESS;
}

/* 设置某网口的IP子网掩码
 * 参数:
 * iNetSeqNo, 网络号, 从0开始
 * pIpMskBase, 子网掩码串基址
 */
EP_STATUS NT_SetOneNetIpMsk(int iNetSeqNo, uint8_t *pIpMskBase)
{

    int  i=0;

    if(!pIpMskBase)
    {
        return  EP_ERROR;
    }

    if((iNetSeqNo != 0) && (iNetSeqNo != 1) && (iNetSeqNo != 2))	/* 最多三个网口 */
    {
        if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        {
            LOG_Write(LOG_KERNEL, "set NET IP Mask net num over range.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "设置子网掩码网口号超出范围.\n", NULL);
        }

        return  EP_ERROR;
    }

    for(i=0; i<4; i++)
    {
        NT_NetCfgInf_g.NetInfArr[iNetSeqNo].aucIpMsk[i]
            =pIpMskBase[i];
    }

    if(NT_SaveFileNetIpMaskCfg(iNetSeqNo,pIpMskBase,4) != EP_SUCCESS)		/* 只需要保存1次 */
    {
        if(ENG_MODE == 1)             /*2007-4-19日 张云修改，为了支持英文版  */
        {
            LOG_Write(LOG_KERNEL, "NET IP Mask save file failure.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "网口子网掩码保存文件不成功.\n", NULL);
        }

        return  EP_ERROR;
    }

    return  EP_SUCCESS;
}

/***********************************************************************
* NT_RslvFileNetIpCfg - 读取解析文件中保存的网络IP配置
*
* RETURNS: EP_SUCCESS: 解析成功
*                 其他, 解析失败
*
*/
EP_STATUS NT_RslvFileNetIpCfg(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pucRtIpAddr,		/* 返回IP地址的字符数组首址，字符串空间，由调用方分配 */
    int iAddrLen				/* 传送的字符数组长度 */
)
{

    uint8_t   aucBuf[128];
    uint8_t   aucTemp[128];
    int  i;
    int  j;
    int  iAddrPartSeq;
    BOOL   bRdIsSuccess;
    int  iAddrPart;

    assert(pucRtIpAddr);
    assert(iAddrLen>=4);

    bRdIsSuccess=TRUE;
    j=0;
    iAddrPartSeq=0;

    if(iNetSeqNo==0)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net1IP", aucBuf, 32)!=1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net1IP值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net1IP, create new ini file.\n", NULL);
            }

            FT_New_SYS_INI_File();
        }
    }
    else  if(iNetSeqNo==1)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net2IP", aucBuf, 32)!=1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net2IP值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net2IP, create new ini file.\n", NULL);
            }

            FT_New_SYS_INI_File();
        }
    }
    else  if(iNetSeqNo==2)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net3IP", aucBuf, 32)!=1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net3IP值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net3IP, create new ini file.\n", NULL);
            }

            FT_New_SYS_INI_File();
        }
    }
    else
    {
        logMsg("解析读取网口%d的IP配置，网口号超出范围!\n",iNetSeqNo,0,0,0,0,0);
        return   EP_ERROR;
    }

    for(i=0; i<32; i++)
    {
        aucTemp[j]=aucBuf[i];
        if(aucBuf[i]=='.'||aucBuf[i]=='\0')
        {
            /*获得地址的一部分 */
            aucTemp[j]='\0';
            iAddrPart=atoi(aucTemp);
            if(iAddrPart<0||iAddrPart>255)
            {
                /* 每部分值超出范围 */
                bRdIsSuccess=FALSE;
                break;
            }
            pucRtIpAddr[iAddrPartSeq]=(uint8_t)((uint32_t)iAddrPart);
            iAddrPartSeq++;
            if(iAddrPartSeq==4)
            {
                break;
            }
            if(aucBuf[i]=='\0')
            {
                break;
            }

            j=0;
        }
        else
        {
            j++;
        }
    }

    if(iAddrPartSeq!=4)
    {
        /* 若for循环结束时,字符串不合格, */
        logMsg("解析读取网口%d的IP配置，网口IP地址超出范围，读出配置字符串为%s!\n",iNetSeqNo,(int)aucBuf,0,0,0,0);
        bRdIsSuccess=FALSE;
    }

    if(!bRdIsSuccess)
    {
        return  EP_ERROR;
    }
    else
    {

        logMsg("Rslv Net Num %d IP Addr success,IP Addr  is %d:%d:%d:%d \n",iNetSeqNo
               ,pucRtIpAddr[0]
               ,pucRtIpAddr[1]
               ,pucRtIpAddr[2]
               ,pucRtIpAddr[3]
               ,0);
        return  EP_SUCCESS;
    }
}

/***********************************************************************
* NT_RslvFileNetIpMaskCfg - 读取解析文件中保存的网络IP Mask配置
*
* RETURNS:
*        EP_SUCCESS: 解析成功
*        其他: 解析失败
*
*/
EP_STATUS NT_RslvFileNetIpMaskCfg(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pucRtIpMaskAddr,		/* 返回IP Mask的字符数组首址，字符串空间，由调用方分配 */
    int iAddrLen				/* 传送的字符数组长度 */
)
{
    uint8_t aucBuf[128];
    uint8_t aucTemp[128];
    int i;
    int j;
    int iAddrPartSeq;
    BOOL bRdIsSuccess;
    int iAddrPart;

    assert(pucRtIpMaskAddr);
    assert(iAddrLen >= 4);

    bRdIsSuccess=TRUE;
    j=0;
    iAddrPartSeq=0;

    if(iNetSeqNo == 0)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net1IPMask", aucBuf, 32) != 1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net1IPMask值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net1IPMask, create new ini file.\n", NULL);
            }

            FT_New_SYS_INI_File();
        }
    }
    else if(iNetSeqNo == 1)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net2IPMask", aucBuf, 32) != 1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net2IPMask值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net2IPMask, create new ini file.\n", NULL);
            }
            FT_New_SYS_INI_File();
        }
    }
    else if(iNetSeqNo == 2)
    {
        if(FT_Rd_Sys_INI("[NET]", "Net3IPMask", aucBuf, 32) != 1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[NET] Net3IPMask值读取失败，创建新的系统INI文件.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of failing ro read [NET] Net3IPMask, create new ini file.\n", NULL);
            }

            FT_New_SYS_INI_File();
        }
    }
    else
    {
        LOG_Dbg_Msg("解析读取网口%d的IP Mask配置，网口号超出范围!\n", iNetSeqNo, 0, 0, 0, 0, 0);

        return EP_ERROR;
    }

    for(i=0; i<32; i++)
    {
        aucTemp[j]=aucBuf[i];
        if(aucBuf[i]=='.'||aucBuf[i]=='\0')
        {
            /* 获得掩码的一部分 */
            aucTemp[j]='\0';
            iAddrPart=atoi(aucTemp);
            if(iAddrPart<0||iAddrPart>255)
            {
                /* 每部分值超出范围  */
                bRdIsSuccess=FALSE;
                break;
            }
            pucRtIpMaskAddr[iAddrPartSeq]=(uint8_t)((uint32_t)iAddrPart);
            iAddrPartSeq++;
            if(iAddrPartSeq == 4)
            {
                break;
            }
            if(aucBuf[i] == '\0')
            {
                break;
            }

            j=0;
        }
        else
        {
            j++;
        }
    }

    if(iAddrPartSeq != 4)
    {
        /* 若for循环结束时,字符串不合格 */
        LOG_Dbg_Msg("解析读取网口%d的IP Mask配置，网口IP Mask地址超出范围，读出配置字符串为%s!\n", iNetSeqNo, (int)aucBuf, 0, 0, 0, 0);
        bRdIsSuccess=FALSE;
    }

    if(!bRdIsSuccess)
    {
        return  EP_ERROR;
    }
    else
    {
        LOG_Dbg_Msg("Rslv Net Num %d IP Mask success,IP Mask  is %d:%d:%d:%d \n", iNetSeqNo,
                    pucRtIpMaskAddr[0],
                    pucRtIpMaskAddr[1],
                    pucRtIpMaskAddr[2],
                    pucRtIpMaskAddr[3],
                    0);

        return  EP_SUCCESS;
    }
}

/***********************************************************************
* NT_SaveFileNetIpCfg - 保存文件中的网络IP配置
*
* RETURNS: EP_SUCCESS: 保存成功
*                 其他, 保存失败
*
*/
EP_STATUS NT_SaveFileNetIpCfg(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pucIpAddr,				/* 传送IP地址的字符数组首址，字符串空间，由调用方分配 */
    int iAddrLen		/* 传送的字符数组长度 */
)
{
    uint8_t   aucBuf[128];
    int  i;
    int  j;
    uint8_t  ucAddrPart;
    uint8_t  ucResult;
    uint8_t  ucFlag;


    assert(pucIpAddr);
    assert(iAddrLen>=4);

    j=0;
    for(i=0; i<4; i++)
    {
        ucFlag=0;
        ucAddrPart=*(pucIpAddr+i);
        ucResult=ucAddrPart/100;
        ucAddrPart=ucAddrPart%100;
        if(ucResult>0)
        {
            ucFlag=1;
            aucBuf[j]='0'+ucResult;
            j++;
        }
        ucResult=ucAddrPart/10;
        ucAddrPart=ucAddrPart%10;
        if(ucResult>0||ucFlag)
        {
            ucFlag=1;
            aucBuf[j]='0'+ucResult;
            j++;
        }
        aucBuf[j]='0'+ucAddrPart;
        j++;
        if(i==3)
        {
            aucBuf[j]='\0';
            j++;
        }
        else
        {
            aucBuf[j]='.';
            j++;
        }

    }

    if(iNetSeqNo==0)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net1IP", aucBuf)<0)
        {
            logMsg("保存网口%d的IP配置到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else  if(iNetSeqNo==1)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net2IP", aucBuf)<0)
        {
            logMsg("保存网口%d的IP配置到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else  if(iNetSeqNo==2)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net3IP", aucBuf)<0)
        {
            logMsg("保存网口%d的IP配置到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else
    {
        logMsg("保存网口%d的IP配置到文件中，网口号超出范围,失败!\n",iNetSeqNo,0,0,0,0,0);
        return   EP_ERROR;
    }

    return  EP_SUCCESS;
}

/***********************************************************************
* NT_SaveFileNetIpMaskCfg - 保存文件中的子网掩码配置
*
* RETURNS:
*        EP_SUCCESS: 保存成功
*        其他: 保存失败
*
*/
EP_STATUS NT_SaveFileNetIpMaskCfg(
    int iNetSeqNo,		/* 网络号，从0开始 */
    uint8_t *pucIpMask,				/* 传送子网掩码的字符数组首址，字符串空间，由调用方分配 */
    int iAddrLen		/* 传送的字符数组长度 */
)
{
    uint8_t aucBuf[128];
    int i;
    int j;
    uint8_t ucAddrPart;
    uint8_t ucResult;
    uint8_t ucFlag;

    assert(pucIpMask);
    assert(iAddrLen>=4);

    j=0;
    for(i=0; i<4; i++)
    {
        ucFlag=0;
        ucAddrPart=*(pucIpMask+i);
        ucResult=ucAddrPart/100;
        ucAddrPart=ucAddrPart%100;
        if(ucResult>0)
        {
            ucFlag=1;
            aucBuf[j]='0'+ucResult;
            j++;
        }
        ucResult=ucAddrPart/10;
        ucAddrPart=ucAddrPart%10;
        if(ucResult>0||ucFlag)
        {
            ucFlag=1;
            aucBuf[j]='0'+ucResult;
            j++;
        }
        aucBuf[j]='0'+ucAddrPart;
        j++;
        if(i==3)
        {
            aucBuf[j]='\0';
            j++;
        }
        else
        {
            aucBuf[j]='.';
            j++;
        }

    }

    if(iNetSeqNo==0)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net1IPMask", aucBuf)<0)
        {
            logMsg("保存网口%d的子网掩码到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else  if(iNetSeqNo==1)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net2IPMask", aucBuf)<0)
        {
            logMsg("保存网口%d的子网掩码到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else  if(iNetSeqNo==2)
    {
        if(FT_Wr_Sys_INI("[NET]", "Net3IPMask", aucBuf)<0)
        {
            logMsg("保存网口%d的子网掩码到文件中，失败!\n",iNetSeqNo,0,0,0,0,0);
        }
    }
    else
    {
        logMsg("保存网口%d的子网掩码到文件中，网口号超出范围,失败!\n",iNetSeqNo,0,0,0,0,0);
        return   EP_ERROR;
    }

    return  EP_SUCCESS;
}