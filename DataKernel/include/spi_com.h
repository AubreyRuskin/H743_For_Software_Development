/* spi_com.h - subroutine library for handling the driver program for SPI and IO Module */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add function taskDelay() to supply the delay between tow frames.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This file contains the driver program for SPI and IO Module.
*/

#ifndef SPI_COM_H
#define SPI_COM_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"

/*defines */

#define SPI_COM_LEN     72				/* 发送缓冲区长度 */
#define SPI_RE_LEN		 8			/* 接受缓冲区长度 */

#define SPI_COM_LEN_WATCHMEGA16     9				/* 与主板监视mega16通讯发送缓冲区长度 */
#define SPI_RE_LEN_WATCHMEGA16		 9			/* 与主板监视mega16通讯接受缓冲区长度 */

#define RISCTIMERNUMFORSPI 0		/* DO使用的Risc Timer */

/* typedef */

typedef struct SPICOMERROR_tag
{
    uint32_t ulMmiSpiComCount;		/* 面板SPI通讯计数 */
    uint32_t ulMmiAuxiSpiComErrorCount;			/* 面板SPI通讯错误计数 */
    uint32_t ulMmiAuxiSpiComSuccessCount;			/* 面板SPI通讯成功计数 */
    uint32_t ulMainSpiComCount;		/* 面板SPI通讯计数 */
    uint32_t ulMainAuxiSpiComErrorCount;					/* 主板SPI通讯错误计数 */
    uint32_t ulMainAuxiSpiComSuccessCount;					/* 主板SPI通讯成功计数 */
    BOOL bMmiSpiComSuccessFlag;			/* 面板通讯成功标志 */
    BOOL bMmiSpiProgramDelFlag;					/* 删除标志 */
    BOOL bMainSpiComSuccessFlag;			/* 主板通讯成功标志 */
    BOOL bMainSpiProgramDelFlag;					/* 删除标志 */
} SPICOMERROR, *pSPICOMERROR;

/* globals */

extern char SS[72];
extern char RS[8];
extern int iIMMR_g;
extern uint32_t SCount;
extern uint32_t ECount;
extern uint32_t RECount;
extern BOOL SHflag;
extern uint16_t spiMode_pm_g;  /* SPI模式 */

/* forward declarations */

/***********************************************************************
* SPI_Process -  SPI通讯任务
*
* RETURNS: 无
*
*/
void SPI_Process();

/***********************************************************************
* SPITest - SPI通讯发送测试
*
* RETURNS: 无
*
*/
void SPITest();

/***********************************************************************
* ClearData - 清屏
*
* RETURNS: 无
*
*/
void ClearData(void);

/***********************************************************************
* XCTest - 液晶闪烁测试
*
* RETURNS: 无
*
*/
void XCTest(void);

/***********************************************************************
* XTest - 液晶显示测试
*
* RETURNS: 无
*
*/
void XTest(void);

/***********************************************************************
* CTest - 液晶清除测试
*
* RETURNS: 无
*
*/
void CTest(void);

/***********************************************************************
* MM_Send_To_SPI -  SPI总线数据收发
*
* RETURNS: 接收数据校验结果，校验成功则返回TRUE，校验失败则返回FALSE
*
*/
BOOL MM_Send_To_SPI(
    char *pOut,			/* 发送数据保存地址*/
    char* pIn			/* 接收数据保存地址 */
);

/***********************************************************************
* Spi_Task_Delay_Init -  us定时初始化
*
* RETURNS: 无
*
*/
void Spi_Task_Delay_Init();

/***********************************************************************
* M16_Send_To_SPI - 与M16之间SPI总线数据收发
*
* RETURNS: 接收数据校验结果，校验成功则返回TRUE，校验失败则返回FALSE
*
*/
BOOL M16_Send_To_SPI(
    char *pOut, 	/* 发送数据保存地址 */
    char* pIn		/* 接收数据保存地址 */
);

/***********************************************************************
* SpiSetLampModeTest - 测试点装置灯
*
* RETURNS: 无
*
*/
EP_STATUS SpiSetLampModeTest(
    uint8_t ucIndex,		/* 灯序号 */
    uint8_t ucCode,				/* 0x01: 熄灭；0x02: 复归掉电保持；0x03: 点亮；0x04: 点亮并掉电保持 。对于用0x04操作点亮的指示灯需要0x01和0x02两个操作才能熄灭 */
    uint8_t color, 	/* 颜色 */
    uint8_t ucFlash		/* 0: 不闪烁；1~255: 闪烁周期，单位为0.1s */
);

/***********************************************************************
* DelAucCpuFile - 删除辅助CPU程序
*
* RETURNS: 无
*
*/
void DelAucCpuFile(void);

/***********************************************************************
* M16_Send_To_SPI_In_ISR - 与监视M16之间SPI总线数据收发(中断中执行)
*
* RETURNS: 接收数据校验结果，校验成功则返回TRUE，校验失败则返回FALSE
*
*/
BOOL M16_Send_To_SPI_In_ISR(
    char *pOut, 	/* 发送数据保存地址 */
    char* pIn		/* 接收数据保存地址 */
);

/***********************************************************************
* spistart - 启动SPI通讯
*
* RETURNS: 无
*
*/
void spistart(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* SPI_COM_H */
