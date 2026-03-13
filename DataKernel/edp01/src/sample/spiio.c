// /* spiio.c - This file contains the driver program for SPI and IO Module */

// /* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

// /*
// modification history
// --------------------------------------------
// 01c, 10may09, dy add the mutual operation between CPU and IO module.
// 01b, 24feb09, dy change the code style.
// 01a, 26jul02, yqf first created.
// */

// /*
// DESCRIPTION
// This file contains the driver program for SPI and IO Module.
// INCLUDES: spiio.h
// */

// /* includes */

// #include "spiio.h"
// #include "spi_mutual.h"
// #include "semLib.h"
// #include "realdata.h"
// #include "errtest.h"
// #include "logmsg.h"
// // #include "sbcm8260Sio.h"
// // #include "config04.h"   /* 底层提供头文件 */
// #include "EdpVer.h"
// #include "miscfunc.h"
// #include  "OPT_SamSyn.h"  /*2013-5-20  ZY  */
// #include "taskLib.h"

// // #include <drv/intrCtl/m8260IntrCtl.h>
// #include <tickLib.h>

// /* defines */

// #define TEST_INT_RESP   0

// /* BRG clock computer */


// /* SPI define */

// #define SPI_COM_LEN 96

// /* SPMODE register PM field calculate. */

// #define SPI_BAUD(b) (BRG_CLK(b)/(4*(spiMode_pm_g+1)))

// /* Ticks to wait for SPI communication. */
// #define WAIT_SPI_RX ((3*1000/RD_SIO_RATE)/SYS_TICK+2)

// /* #define SPI_PARAM_BASE 0x3800 */  /* EDP01_A-C, 8250 */
// #define SPI_PARAM_BASE 0x1800   /* 8247 */

// #define SPI_PARAM(ofst) (SPI_PARAM_BASE+(ofst))

// /* SPI receive DB address */
// #define SPI_RBASE (SPI_PARAM_BASE+0x40)
// /* SPI transmit DB address */
// #define SPI_TBASE (SPI_PARAM_BASE+0x48)

// /* SPI receive buffer address */
// #define SPI_RX_BUF (SPI_PARAM_BASE+0x80)
// /* SPI transmit buffer address */
// #define SPI_TX_BUF (SPI_PARAM_BASE+0x100)

// #define DIDO_EXEC_INTERVAL_TIME 600000000L		/* 两次DIDO异常的US数时间间隔,大约为10分钟 */
// #define CHECK_SUM_INTERVAL_Time 4000UL		/* 约为2秒 */
// #define ALLOW_CHECK_SUM_ERR_CNT 200  /* CHECK_SUM_INTERVAL_Time时间内的出错次数 */

// #define SPI_RXERR_CHK_FREQ (20*RD_SIO_RATE)  /* 通信检查周期：20秒 */
// #define SPI_RXERR_ALM_LEVEL	(SPI_RXERR_CHK_FREQ*9/10) /* 通信告警门槛, 同时切换为缺省值：90/100误码率 */
// #define SPI_RXERR_LOG_LEVEL	(SPI_RXERR_CHK_FREQ/100)	/* 通信日志门槛, 同时切换为缺省值: 1/100误码率 */
// #define SPI_RXERR_RET_LEVEL (SPI_RXERR_CHK_FREQ/1000)	/* 通信恢复门槛: 1/1000误码率 */

// #define SPI_RXERR_RET_FREQ	30							/* 连续30次通信检查(30*20秒)都正常后才再次开放日志记录功能 */
// /* 可以考虑实现指数级浮动门槛机制, 但没有实际意义 */

// #define SPI_OVER_THRESHOLD_TIME (1*60)  /* 连续越门槛持续时间 */
// #define SPI_OVER_THRESHOLD_LEVEL ((SPI_OVER_THRESHOLD_TIME*RD_SIO_RATE)/SPI_RXERR_CHK_FREQ)  /* 连续越门槛统计次数 */

// #define EXT_EN_DO 0x0001
// #define INT_EN_DO 0x0002

// #define RCV_OV_FLAG 0x8000
// #define RCV_ERR_FLAG 0x4000
// #define ECV_SIGN_FLAG 0x1000

// #define MAX_ERR_CNT (2*RD_SIO_RATE)	/* 异常接收消抖次数 */
// #define MAX_REBOOT_NUM 10    /* 最多重启次数 */

// #define SLAVE_CPU_CRC_CHECK_CNT 3		/*从CPU CRC校验确认帧数*/
// #define SLAVE_CPU_SUM_CHECK_CNT 100     /*从CPU SUM校验确认帧数*/
// /* #define SPIIOTEST */		/* test switch. */
// #define SPI_DI_NO_FILTER_CNT 10

// /* typedefs */

// #if TEST_INT_RESP
// struct tcn_struct
// {
//     uint16_t tcn3max;
//     uint16_t tcn3min;
//     uint32_t tcnv, tcns;
//     uint32_t tcncnt;
// } tcn= {0, 10000, 0, 0, 0};
// #endif

// /* gloabals */

// uint32_t ulSpiComDelay;	/* SPI上传延时，us */
// uint32_t ulPollDelay = 0;	/* 查询延时，us */
// SPI_IO_BUF aspibuf_g[MAX_MOD_NUM];		/* SPI<->IO 模件数据交换缓冲 */
// SPI_COM_INFO spiinfo;
// BOOL bNormalSndFlag = TRUE; /* 任务发送模式 */
// uint16_t spiMode_pm_g = 2;  /* SPI模式,EDP01/02平台缺省为总线频率100M模式 */

// extern BOOL g_bFinishFlag; /* 初次配置完成 */
// extern u_int uiDiNum_g; /* 开入通道配置数 */

// /* locals */

// static u_int uiEnDoFg_g;
// static unsigned char spiWriteBuf[SPI_COM_LEN];  /* writing buffer. */

// /* global functions */

// /* disable the QD signal on monitor controller.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// extern void WT_MegaCloseQD(void);

// /* enable the QD signal on monitor controller.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// extern void WT_MegaOpenQD(void);

// /* 设置所有保护任务处于复归状态，在复归命令中调用
//    参数：无
//    返回：无 */
// extern void  RE_SetAllTaskFgSts();

// /* 是否使用母板DI */
// extern BOOL RD_GetMbDIUsedFlag(void);

// /* static functions */

// /* processing the DO reset.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOReset(SPI_IO_BUF *pspibuf);

// /* processing the IO error, called in ISR.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOExec(SPI_IO_BUF *pspibuf);

// /* parse the status word.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealStatusWord(SPI_IO_BUF *pspibuf);

// /***********************************************************************
// * SetMBDo - 获取母板开出(电铁使用)
// *
// * RETURNS: EP_ERROR, EP_SUCCESS
// *
// */
// static EP_STATUS SetMBDo(
//     int *pBuf		/* 保存地址 */
// );

// /* ISR for Timer3.
//  * Para:
//  *     iVal, parameter for ISR.
//  * Return:
//  *     NONE.
//  */
// static void Timer3_ISR(int iVal);

// /* ISR for SPI.
//  * Para:
//  *     iVal, parameter for ISR.
//  * Return:
//  *     NONE.
//  */
// static void SPI_ISR(int iVal);

// /* processing the DI state.
//  * Para:
//  *     p_spi_io_buffer, buffer for io from SPI.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Recv_DI(SPI_IO_BUF *p_spi_io_buffer);

// /* AI and DI processing for common board, called in ISR.
//  * Para:
//  *     pspibuf, buffer of IO module data.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Recv_AI_DI(SPI_IO_BUF *pspibuf);

// /* read the state of IO module.
//  * Para:
//  *     iModAddr, address of IO module.
//  *     iReg, address of the register.
//  *     iNum, number of register need to be read.
//  *     pucBuf, buffer of data.
//  *     bAutoUp, 是否自动上送.
//  * Return:
//  *     NONE.
//  */
// static EP_STATUS SIO_Read_Reg(int iModAddr, int iReg, int iNum, uint8_t *pucBuf, BOOL bAutoUp);

// /* update the DI filter parameter.
//  * Para:
//  *     iScanRate, scan rate.
//  * Return:
//  *     NONE.
//  */
// static void UpdateDiFltCfg(int iScanRate);

// /* global functions */

// /* judge if it is the second board.
//  * Para:
//  *     NONE.
//  * Return:
//  *     TRUE, or FALSE.
//  */
// extern BOOL isNumber_2_04CPU();

// /* match the IO module between configuration and slot.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Match_IO(void);

// /* match the IO module between configuration and slot on slave CPU.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Match_IO_Slave(void);

// /* processing the IO status.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOSts(SPI_IO_BUF *pspibuf);

// /* 设定SPI通信相关参数.
//  * Para:
//  *     iScanRate, scan rate..
//  * Return:
//  *     NONE.
//  */
// static void SIO_SetSPIInfo(int iScanRate);

// /* SPI data frame sending.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Snd(void);

// /* functions */

// /* SPI Device Driver */

// /* initialize the SPI hardware.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Hw_Init(void)
// {
//     int iLockKey;
//     int iBRG_CLK;

//     iLockKey = intLock();   /* Prevent register R/W collision. */

//     // /* Setup SPIMODE to normal operation, master mode, enable, baud. */
//     // *SPMODE(iIMMR_g) = 0x26F0 | spiMode_pm_g;

//     // /* set the parallel ports */
//     // /* Enable SPIMISO(PD16),SPIMOSI(PD17),SPICLK(PD18) and SPISEL(PD19) */

//     // (*PDIRD(iIMMR_g)) |= 0x00007000; /* PD17, PD18 and PD19 as output. */
//     // (*PDIRD(iIMMR_g)) &= (~0x00008000); 		/* PD16 as input. */

//     // (*PSORD(iIMMR_g)) |= 0x0000F000;

//     // (*PODRD(iIMMR_g)) &= (~0x00007000);

//     // (*PPARD(iIMMR_g)) |= 0x0000E000;

//     // (*PDATD(iIMMR_g)) |= 0x00007000;

//     // /* Set /Slave on PD23，EDP02上是PD23，EDP01上是PC25 */

//     // (*PDIRD(iIMMR_g)) |= 0x00000100;

//     // (*PPARD(iIMMR_g)) &= (~0x0000100);

//     // (*PSORD(iIMMR_g)) &= (~0x00000100);

//     // (*PODRD(iIMMR_g)) &= (~0x00000100);

//     // /* SPI on 1st CPU in master mode. 主CPU处于SPI MASTER模式 */
//     // IoPinOutputHigh(IO_OUT_SPI_SLAVE_SEL, IO_PIN_LOW);

//     /* 由底层提供IO口初始化 */

//     // WT_MegaCloseQD();	/* 关闭启动信号 */

//     /* EDP01平台不开放启动信号 */
//     if (bdType_g == BOARD_TYPE_E01)
//     {
//         IoPinOutputHigh(IO_OUT_QD, IO_PIN_LOW);
//     }

//     IoPinOutputHigh(IO_OUT_GJ, IO_PIN_LOW); 	/* 禁止告警 */

//     // /* Assign a pointer to the SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+0x89fc) = SPI_PARAM_BASE;

//     // /* Write RABASE and TBASE to SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x00)) = SPI_RBASE;
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x02)) = SPI_TBASE;

//     // /* Write RFCR and RFCR to SPI parameter RAM */
//     // *(uint8_t*)(iIMMR_g+SPI_PARAM(0x04)) = 0x10;
//     // *(uint8_t*)(iIMMR_g+SPI_PARAM(0x05)) = 0x10;

//     // /* Write MRBLR  to SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x06)) = SPI_COM_LEN;

//     // /* initialize RxBD */
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET) = BD_RX_EMPTY_BIT |
//     //         BD_RX_WRAP_BIT | BD_RX_INTERRUPT_BIT;
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_DATA_LENGTH_OFFSET) = 0x0000;
//     // *(uint32_t*)(iIMMR_g+SPI_RBASE+BD_BUF_POINTER_OFFSET) = iIMMR_g+SPI_RX_BUF;

//     // /* initialize TxBD */
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET) = BD_TX_WRAP_BIT | 0x0800;
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_DATA_LENGTH_OFFSET) = SPI_COM_LEN;
//     // *(uint32_t*)(iIMMR_g+SPI_TBASE+BD_BUF_POINTER_OFFSET) = iIMMR_g+SPI_TX_BUF;

//     // /* initialize TX and RX */
//     // while (*CPCR(iIMMR_g) & CPM_CR_FLG)
//     //     continue;                       /* Wait for cp idle. */

//     // *CPCR(iIMMR_g) = 0x25410000;

//     // /* Clear SPIE */
//     // *SPIE(iIMMR_g) = 0xFF;

//     // /* Enable RXB interrupt */
//     // *SPIM(iIMMR_g) = 0x01;

//     /* test  */
//     // iBRG_CLK=BRG_CLK(iIMMR_g);
//     // LOG_Dbg_Msg("SPI BAUD is %d, BRG CLK is %d, PLLMF is %d, DFBRGis %d, VCO_OUT is %d\n",
//     //             SPI_BAUD(iIMMR_g), iBRG_CLK, PLLMF(iIMMR_g), DFBRG(iIMMR_g), VCO_OUT(iIMMR_g), 0);

//     // /* The actual baud rate must <= the maximum sustained data rate(CPMCLK/50). */
//     // assert(SPI_BAUD(iIMMR_g)<4000000L); /* Limitted by CPU freq. of IO boards. */
//     // /* Limitted by transmition timing. */
//     // assert(1000000L*SPI_COM_LEN*8/SPI_BAUD(iIMMR_g)+70<1000000L/RD_SIO_RATE);

//     // /* Setup SPIMODE to normal operation, master mode, enable, baud. */
//     // // *SPMODE(iIMMR_g) = 0x27F0 | spiMode_pm_g;

//     // ulSpiComDelay=(1000000L*SPI_COM_LEN*8)/SPI_BAUD(iIMMR_g)+400;		/* 加上IO板消抖时间 */

//     intUnlock(iLockKey);
// }

// /* initialize the Timer3 hardware.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void Timer3_Hw_Init (void)
// {
//     // /* Enable timer3 */
//     // *TGCR2(iIMMR_g) |= 0x01;
//     // *TGCR2(iIMMR_g) &= (~0x02);

//     // /* Set timer3 interrupt frequence to RD_SIO_RATE. */
//     // *TRR3(iIMMR_g) = 1000000L/RD_SIO_RATE-1;

//     // /* Set timer3 clock frequence 1MHz. 张云改成RESTART模式 */
//     // *TMR3(iIMMR_g) = ((sysInputFreq_g/1000000L-1)<<8) | 0x1A;

//     /* 查询延时 */
//     ulPollDelay = 1000000/(RD_SIO_RATE*2);
// }

// /* initialize the master SPI driver.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Driver_Init(void)
// {
//     int i;
//     int jj;
//     RD_PART_INFO *p_part;

//     /* initialize spi<->io buffer */
//     for	(i=0; i<MAX_MOD_NUM; i++)
//     {
//         aspibuf_g[i].ucModType=IDLE_MODULE;
//         aspibuf_g[i].ucModAddr=i;
//         aspibuf_g[i].pfDealRecv=NULL;
//         /* Initialize spi<->IO buffer semphore. */
//         aspibuf_g[i].semMod = semMCreate(SEM_Q_PRIORITY);
//         aspibuf_g[i].bIORebootFlag = FALSE;				/* 是否重启 */

//         aspibuf_g[i].ulCheckSumErrCnt = 0;
//         aspibuf_g[i].ulExcCnt = 0;
//         aspibuf_g[i].ulTotalExcCnt = 0;
//         aspibuf_g[i].ulRebootCnt = 0;
//         aspibuf_g[i].bRebootFlag = FALSE;
//         aspibuf_g[i].bErrOvFlag = FALSE;
//         aspibuf_g[i].ulErrOccurCnt = 0;
//         aspibuf_g[i].ulCheckSumErrStat = 0;  /* 统计用 */
//         aspibuf_g[i].bUsed=FALSE;
//         aspibuf_g[i].bDIDOExecFlag = FALSE;    /* exception flag. */
//         aspibuf_g[i].ulLastDIDOExecTime = 0;			/* the last time reporting error. */
//         aspibuf_g[i].bCycleCommandSndFlag = FALSE;
//         aspibuf_g[i].bInvalid = FALSE;		/* 失效 */
//         aspibuf_g[i].bBreakdown = FALSE;		/* 击穿 */
//         aspibuf_g[i].bReset = FALSE;	/* 重启 */
//         aspibuf_g[i].bSpiComError = FALSE;			/* SPI通讯错 */
//         aspibuf_g[i].bQDInvalid = FALSE;		/* 启动失效 */
//         aspibuf_g[i].ucDiGroupNum = 0;
//         aspibuf_g[i].ucAiChnCnt = 0;
//         aspibuf_g[i].bGetDiFlag = TRUE;
//         aspibuf_g[i].bGetAiFlag = FALSE;
//         aspibuf_g[i].bChgBaseReg = FALSE;
//         aspibuf_g[i].bSetDoFlag = FALSE;
//         aspibuf_g[i].ulSetDoCnt = 0;
//         aspibuf_g[i].bSetCmdFlag = FALSE;
//         aspibuf_g[i].bCRCCheckMod = TRUE;  /* 缺省为求和取反模式 *//*改为初始CRC校验*/
//         aspibuf_g[i].bCheckModAffirm = FALSE;  /* 初始进行校验法确认 */
//         aspibuf_g[i].ulSumCheckCnt = 0; /* 求和取反校验正确次数 */
//         aspibuf_g[i].ulCRCCheckCnt = 0;    /* CRC校验正确次数 */
//         aspibuf_g[i].ulOverThresholdCnt = 0;
//         aspibuf_g[i].bDefaultFlag = FALSE;
//         aspibuf_g[i].ulSwitchDefaultCnt = 0;
//         aspibuf_g[i].ulSwitchDefaultTm = 0;
//         aspibuf_g[i].bErrFrmLogFlag = FALSE;
//         for(jj=0; jj<MAXCMDNUM; jj++)
//         {
//             aspibuf_g[i].ulIOConfirmCnt[jj]=0;
//         }
//     }

//     for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
//     {
//         if(((p_part->ucType == DO_MODULE)
//                 || (p_part->ucType == DI_MODULE)
//                 || (p_part->ucType == DIO_MODULE)
//                 || (p_part->ucType == CKDIO_MODULE)
//                 /* || (p_part->ucType == AI_MODULE) */  /* 模拟量输入 */
//                 || (p_part->ucType == AO_MODULE)	/* 模拟量输出 */
//                 || (p_part->ucType == COM_MODULE))    /* 通用类型 */
//                 && (p_part->ucPosition == 0))
//         {
//             /* 仅针对本机箱，类型由配置而来 */
//             aspibuf_g[p_part->aucHwAddr[0]].bUsed=TRUE;	/* 设置该模件被使用 */
//         }
//     }

//     // /* initialize interrupt vect */
//     // intConnect(INUM_TO_IVEC(INUM_SPI), SPI_ISR, 0);

//     // intConnect(INUM_TO_IVEC(INUM_TIMER3), Timer3_ISR, 0);


//     /* Initialize SPI. */
//     SPI_Hw_Init();

//     /* Initialize timer3 hardware. */
//     Timer3_Hw_Init();

//     /* Enable SPI interrupt */
//     // intEnable(INUM_SPI);

//     /* Enable timer3 interrupt */
//     // intEnable(INUM_TIMER3);
// }

// /* initialize the slave SPI hardware.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Slave_Hw_Init(void)
// {
//     int iLockKey;

//     iLockKey = intLock();  /* Prevent register R/W collision. */

//     // /* Stop slave SPI. */
//     // *SPMODE(iIMMR_g) = 0x24F0 | spiMode_pm_g;

//     // /* set the parallel ports */

//     // /* Set /Slave on PD23，EDP02上是PD23，EDP01上是PC25 */

//     // (*PDIRD(iIMMR_g)) |= 0x00000100;

//     // (*PPARD(iIMMR_g)) &= (~0x0000100);

//     // (*PSORD(iIMMR_g)) &= (~0x00000100);

//     // (*PODRD(iIMMR_g)) &= (~0x00000100);

//     // /* SPI on 2nd CPU in  Slave mode. 从CPU处于SPI SLAVE模式 */
//     // IoPinOutputHigh(IO_OUT_SPI_SLAVE_SEL, IO_PIN_HIGH);


//     // /* Enable SPIMISO(PD16),SPIMOSI(PD17),SPICLK(PD18) and SPISEL(PD19). */

//     // (*PDIRD(iIMMR_g)) &= (~0x00007000); /* PD17, PD18 and PD19 as input. */
//     // (*PDIRD(iIMMR_g)) |= 0x00008000; /* PD16 as output. */

//     // (*PSORD(iIMMR_g)) |= 0x0000F000;

//     // (*PODRD(iIMMR_g)) &= (~0x0000F000);

//     // (*PPARD(iIMMR_g)) |= 0x00007000;

//     // (*PDATD(iIMMR_g)) |= 0x00008000;



//     /* 由底层初始化IO口 */

//     // WT_MegaCloseQD();

//     /* EDP01平台不开放启动信号 */
//     if (bdType_g == BOARD_TYPE_E01)
//     {
//         IoPinOutputHigh(IO_OUT_QD, IO_PIN_LOW);		/* 不开放启动信号 */
//     }

//     IoPinOutputHigh(IO_OUT_GJ, IO_PIN_LOW);	/* 禁止告警 */

//     // /* Assign a pointer to the SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+0x89fc) = SPI_PARAM_BASE;

//     // /* Write RABASE and TBASE to SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x00)) = SPI_RBASE;
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x02)) = SPI_TBASE;

//     // /* Write RFCR and TFCR to SPI parameter RAM */
//     // *(uint8_t*)(iIMMR_g+SPI_PARAM(0x04)) = 0x10;
//     // *(uint8_t*)(iIMMR_g+SPI_PARAM(0x05)) = 0x10;

//     // /* Write MRBLR  to SPI parameter RAM */
//     // *(uint16_t*)(iIMMR_g+SPI_PARAM(0x06)) = SPI_COM_LEN+1;

//     // /* initialize RxBD */
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET) = BD_RX_EMPTY_BIT |
//     //         BD_RX_WRAP_BIT | BD_RX_INTERRUPT_BIT;
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_DATA_LENGTH_OFFSET) = 0x0000;
//     // *(uint32_t*)(iIMMR_g+SPI_RBASE+BD_BUF_POINTER_OFFSET) = iIMMR_g+SPI_RX_BUF;

//     // /* initialize TxBD */
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET) = BD_TX_WRAP_BIT | 0x0800;
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_DATA_LENGTH_OFFSET) = SPI_COM_LEN;
//     // *(uint32_t*)(iIMMR_g+SPI_TBASE+BD_BUF_POINTER_OFFSET) = iIMMR_g+SPI_TX_BUF;

//     // /* initialize TX and RX */
//     // while (*CPCR(iIMMR_g) & CPM_CR_FLG)
//     //     continue;                       /* Wait for cp idle. */

//     // *CPCR(iIMMR_g) = 0x25410000;

//     // /* Clear SPIE */
//     // *SPIE(iIMMR_g) = 0xFF;

//     // /* Enable RXB interrupt */
//     // *SPIM(iIMMR_g) = 0x01;

//     // /* Setup SPIMODE to normal operation, slave mode, enable, baud. */
//     // *SPMODE(iIMMR_g) = 0x25F0 | spiMode_pm_g;

//     // ulSpiComDelay=(1000000L*SPI_COM_LEN*8)/SPI_BAUD(iIMMR_g)+400;		/* 加上IO板消抖时间 */

//     intUnlock(iLockKey);
// }

// /* SPI data receiving.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SPI_Recv(void)
// {
//     int m;
//     SPI_IO_BUF *pspibuf;	/* 模件信息数组 */
//     uint8_t *pucRx;			/*	SPI接收缓冲区首地址 */

//     pucRx =(uint8_t*)(iIMMR_g+SPI_RX_BUF);/* 数据帧发送之外的时间用于处理 */
//     pspibuf =aspibuf_g;
//     spiinfo.ulEnterISRCnt++;
//     /* Process RX buf data */
//     for (m =0; m < MAX_MOD_NUM; m++, pspibuf++)
//     {
//         int 	i;
//         uint8_t *pucUp, *pucSrc;
//         uint8_t ucSum, uc;
//         uint8_t *pucSrcTmp = NULL;
//         uint8_t aucBuf[SPI_FRAME_LEN_PER_MOD-1];

//         if((m == DIADDRONCPU) && (pspibuf->unDiChNum))
//         {
//             /* 主板开入不在这里处理 */
//             continue;
//         }

//         ucSum =0;
//         pucUp =pspibuf->aucUpFrame;

//         /* exchange the odd byte with the even byte. */
//         if (m&0x1)
//         {
//             pucSrc =pucRx+m-1;
//         }
//         else
//         {
//             pucSrc =pucRx+m+1;
//         }

//         /* SPI主从,允许设置 */
//         if ((((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())|| (ucCpuSpiRol_g == 2))
//                 && pspibuf->bUsed)
//         {
//             IO_Num_2_Get_OvVal(pspibuf, pucSrc);
//         }

//         if ((pspibuf->ucModType == IDLE_MODULE) || !pspibuf->bUsed)
//         {
//             /* 未使用模件不处理 */
//             continue;
//         }

//         /* 确认校验模式 */
//         if (!pspibuf->bCheckModAffirm)
//         {
//             pucSrcTmp = pucSrc;
//             for (i = 0; i<SPI_FRAME_LEN_PER_MOD-1; i++, pucSrcTmp += 16)
//             {
//                 aucBuf[i] = *pucSrcTmp;
//                 ucSum += aucBuf[i];
//             }

//             if (CDT_BCH_Check(aucBuf, SPI_FRAME_LEN_PER_MOD-1) == (*pucSrcTmp))
//             {
//                 pspibuf->ulCRCCheckCnt++;
//                 pspibuf->bCRCCheckMod = TRUE;

//                 if(isNumber_2_04CPU())	/*从CPU 需要3 帧确认校验方式*/
//                 {
//                     if(pspibuf->ulCRCCheckCnt >= SLAVE_CPU_CRC_CHECK_CNT)
//                     {
//                         pspibuf->bCheckModAffirm = TRUE;
//                         pspibuf->ulSumCheckCnt = 0;
//                         pspibuf->ulCRCCheckCnt = 0;
//                     }
//                 }
//                 else 	/*主CPU 直接确认校验方式*/
//                 {
//                     /*pspibuf->bCheckModAffirm = TRUE;*/	/*主CPU 通过版本号来确认校验方式*/
//                     pspibuf->ulSumCheckCnt = 0;
//                     pspibuf->ulCRCCheckCnt = 0;
//                 }
//             }
//             else if (ucSum == (uint8_t)~(*pucSrcTmp))
//             {
//                 pspibuf->ulSumCheckCnt++;
//                 pspibuf->bCRCCheckMod = FALSE;

//                 /*求和校验主CPU直接确认，无需三帧确认*/
//                 /*从CPU对于求和校验确认次数为100*/
//                 if(isNumber_2_04CPU())	/*从CPU 需要3 帧确认校验方式*/
//                 {
//                     if(pspibuf->ulSumCheckCnt >= SLAVE_CPU_SUM_CHECK_CNT)
//                     {
//                         pspibuf->bCheckModAffirm = TRUE;
//                         pspibuf->ulSumCheckCnt = 0;
//                         pspibuf->ulCRCCheckCnt = 0;
//                     }
//                 }
//                 else 	/*主CPU 直接确认校验方式*/
//                 {
//                     /*pspibuf->bCheckModAffirm = TRUE;*/	/*主CPU 通过版本号来确认校验方式*/
//                     pspibuf->ulSumCheckCnt = 0;
//                     pspibuf->ulCRCCheckCnt = 0;
//                 }
//             }
//             else	/*for 方案4.0,前面判断都错误则设定为CRC校验,但不确认*/
//             {
//                 pspibuf->bCRCCheckMod = TRUE;
//                 pspibuf->ulSumCheckCnt = 0;
//                 pspibuf->ulCRCCheckCnt = 0;
//             }

//             /* 如以上两种校验均不能通过, 则缺省作求和取反处理
//              * 以上判断重复进行
//              * 如I/O模件重启, 校验方法保持不变
//              */
//         }

//         /* 正式校验处理 */
//         ucSum = 0;
//         for (i =0; i < 5; i++, pucSrc +=16)
//         {
//             uc =*pucSrc;
//             *pucUp++ =uc;

//             /* 求和取反 */
//             if (!pspibuf->bCRCCheckMod)
//             {
//                 ucSum+=uc;
//             }
//         }
//         uc =*pucSrc;
//         *pucUp =uc;

//         /* 区分CRC校验和求和取反校验 */
//         if (pspibuf->bCRCCheckMod)
//         {
//             ucSum = CDT_BCH_Check(pspibuf->aucUpFrame, SPI_FRAME_LEN_PER_MOD-1);
//         }
//         else
//         {
//             uc = (uint8_t)~uc;
//         }

//         if (ucSum != uc)
//         {
//             /* frame check error */
//             pspibuf->bComErr =TRUE; /* 表明通信不正常 */
//             pspibuf->ulCheckSumErrStat++; /* 统计用 */
//             pspibuf->ulCheckSumErrCnt++;

//             /* 记录第一个出错帧 */
//             if (pspibuf->ulCheckSumErrCnt == 1)
//             {
//                 for (i = 0; i < 6; i++)
//                 {
//                     pspibuf->aucUpErrFrame[i] = pspibuf->aucUpFrame[i];
//                 }
//             }
//         }
//         else
//         {
//             pspibuf->bComErr = FALSE;

//             /* finish the DO sending. */
//             if (pspibuf->bSetDoFlag)
//             {
//                 pspibuf->ulSetDoCnt--;
//                 if (!pspibuf->ulSetDoCnt)
//                 {
//                     pspibuf->bSetDoFlag = FALSE;
//                 }
//             }

//             if (spiinfo.bIOInitFinish)
//             {
//                 /* Process module status word. */
//                 uint8_t status =pspibuf->aucUpFrame[0];

//                 if ((((status & 0x3F) == DO_SYS_EVENT) && ((pspibuf->ucModType == DO_MODULE) || (pspibuf->ucModType == DIO_MODULE)))
//                         || (((status & 0x3F) == CKDOFEEDBAKC1) && (pspibuf->ucModType == CKDIO_MODULE)))
//                 {
//                     /* 系统事件寄存器 */
//                     pspibuf->ucModSts=pspibuf->aucUpFrame[4];		/* 最后一个字节表示IO故障信息 */
//                     DealDOExec(pspibuf);		/* 处理开出异常 */
//                 }
//                 else if ((status & 0x3F) == IO_STATUS_REG)
//                 {
//                     pspibuf->ucModSts=pspibuf->aucUpFrame[1];		/* IO故障信息 */

//                     DealDOExec(pspibuf);		/* 处理开出异常 */
//                 }
//                 else if ((status & 0x3F) == DO_INVALID_STATUS_REG)
//                 {
//                     DealDOSts(pspibuf);		/* 处理开出异常 */
//                 }
//                 else if ((status == 0x40)
//                          || (status == 0x00))    /* 新规约复位后的情况 */
//                 {
//                     DealDOReset(pspibuf);
//                 }
//             }

//             if (pspibuf->ulCmdSts)
//             {
//                 /* command excuing over. */
//                 if (pspibuf->ulCmdSts&IO_GET_FAULT_CMD)
//                 {
//                     /* get fault information. */
//                     if ((pspibuf->ucModType == DO_MODULE) || (pspibuf->ucModType == DIO_MODULE))
//                     {
//                         /* get DI status. */
//                         pspibuf->ulCmdSts |= IO_GET_DI_ONDO_CMD;
//                         pspibuf->ulCmdSndCnt[1] = CMD_EXCUTE_ACK_NUM;
//                     }
//                     else if (pspibuf->ucModType == CKDIO_MODULE)
//                     {
//                         /* get DI status. */
//                         pspibuf->ulCmdSts |= IO_GET_DI_CKDO_CMD;
//                         pspibuf->ulCmdSndCnt[2] = CMD_EXCUTE_ACK_NUM;
//                     }
//                 }

//                 if (!pspibuf->bSetDoFlag)
//                 {
//                     IO_ClrComCmd(pspibuf);		/* clear command status. */
//                 }
//             }

//             /* Call receive process function. */
//             if (pspibuf->pfDealRecv)
//                 pspibuf->pfDealRecv(pspibuf);

//             IO_SndComCmd(pspibuf);		/* send common command. */
//         }
//         /* 检查通信错误 */
//         if (spiinfo.ulEnterISRCnt >= spiinfo.ulSPIRxErrCheckFreq)
//         {
//             unsigned  char   aucLogInfo[256];

//             if (pspibuf->ulCheckSumErrCnt >= spiinfo.ulSPIRxErrAlmLevel)
//             {
//                 /* 缺省值切换记录 */
//                 pspibuf->bDefaultFlag = TRUE;
//                 /*2013-5-25 ZY */
//                 pspibuf->ulSwitchDefaultTm = TM_High_Get_usCnt();
//                 TM_High_Get_Sys_Us_UTC_Time(&pspibuf->utSwitchDefaultChgTime, &pspibuf->ulSwitchDefaultTm);
//                 pspibuf->ulSwitchDefaultCnt++;

//                 pspibuf->ulOverThresholdCnt++;

//                 /* 增加通信出错累计判断, 目前为1分钟共3次 */
//                 if((!pspibuf->bErrOvFlag) && (pspibuf->ulOverThresholdCnt >= spiinfo.ulOverThreshold))
//                 {
//                     if(ENG_MODE == 0)
//                     {
//                         ER_Set_Err(EV_DIDO_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
//                                    "模件地址:%02d,错误码:%02d\n", pspibuf->ucModAddr+1, SPI_CHECK_ERR);
//                     }
//                     else
//                     {
//                         ER_Set_Err(EV_DIDO_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
//                                    "module address:%02d,reason code:%02d\n", pspibuf->ucModAddr+1, SPI_CHECK_ERR);
//                     }
//                     sprintf(aucLogInfo, "%d#IO模件通信异常,误码率: %d/%d\n",
//                             (int)(pspibuf->ucModAddr+1), (int)pspibuf->ulCheckSumErrCnt, (int)spiinfo.ulSPIRxErrCheckFreq);
//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

//                     sprintf(aucLogInfo, "接收校验出错报文: %x %x %x %x %x %x\n",
//                             pspibuf->aucUpErrFrame[0],
//                             pspibuf->aucUpErrFrame[1],
//                             pspibuf->aucUpErrFrame[2],
//                             pspibuf->aucUpErrFrame[3],
//                             pspibuf->aucUpErrFrame[4],
//                             pspibuf->aucUpErrFrame[5]);

//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

//                     pspibuf->bErrOvFlag =TRUE;
//                     pspibuf->bErrFrmLogFlag = TRUE;
//                 }
//                 else if (pspibuf->ulErrOccurCnt == 0)
//                 {
//                     sprintf(aucLogInfo, "%d#IO模件通信异常,误码率: %d/%d\n",
//                             (int)(pspibuf->ucModAddr+1), (int)pspibuf->ulCheckSumErrCnt, (int)spiinfo.ulSPIRxErrCheckFreq);
//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

//                     sprintf(aucLogInfo, "接收校验出错报文: %x %x %x %x %x %x\n",
//                             pspibuf->aucUpErrFrame[0],
//                             pspibuf->aucUpErrFrame[1],
//                             pspibuf->aucUpErrFrame[2],
//                             pspibuf->aucUpErrFrame[3],
//                             pspibuf->aucUpErrFrame[4],
//                             pspibuf->aucUpErrFrame[5]);

//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

//                     pspibuf->bErrFrmLogFlag = TRUE;
//                 }
//                 pspibuf->ulErrOccurCnt =SPI_RXERR_RET_FREQ;
//             }
//             else if (pspibuf->ulCheckSumErrCnt >= spiinfo.ulSPIRxErrLogLevel)
//             {
//                 /* 缺省值切换记录 */
//                 pspibuf->bDefaultFlag = TRUE;
//                 /*2013-5-25 ZY */
//                 pspibuf->ulSwitchDefaultTm = TM_High_Get_usCnt();
//                 TM_High_Get_Sys_Us_UTC_Time(&pspibuf->utSwitchDefaultChgTime, &pspibuf->ulSwitchDefaultTm);
//                 pspibuf->ulSwitchDefaultCnt++;

//                 pspibuf->ulOverThresholdCnt = 0;
//                 if (pspibuf->ulErrOccurCnt == 0)
//                 {
//                     sprintf(aucLogInfo, "%d#IO模件通信干扰,误码率: %d/%d\n",
//                             (int)(pspibuf->ucModAddr+1), (int)pspibuf->ulCheckSumErrCnt, (int)spiinfo.ulSPIRxErrCheckFreq);

//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

//                     sprintf(aucLogInfo, "接收校验出错报文: %x %x %x %x %x %x\n",
//                             pspibuf->aucUpErrFrame[0],
//                             pspibuf->aucUpErrFrame[1],
//                             pspibuf->aucUpErrFrame[2],
//                             pspibuf->aucUpErrFrame[3],
//                             pspibuf->aucUpErrFrame[4],
//                             pspibuf->aucUpErrFrame[5]);

//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
//                     pspibuf->bErrFrmLogFlag = TRUE;
//                 }
//                 pspibuf->ulErrOccurCnt =SPI_RXERR_RET_FREQ;
//             }
//             else
//             {
//                 /* 恢复使用实际值, 同时如果上次记录日志
//                  * 则记录对应恢复日志
//                  */
//                 pspibuf->bDefaultFlag = FALSE;
//                 /*2013-5-25 ZY */
//                 pspibuf->ulSwitchDefaultTm = TM_High_Get_usCnt();
//                 TM_High_Get_Sys_Us_UTC_Time(&pspibuf->utSwitchDefaultChgTime, &pspibuf->ulSwitchDefaultTm);

//                 if (pspibuf->bErrFrmLogFlag)
//                 {
//                     pspibuf->bErrFrmLogFlag = FALSE;
//                     sprintf(aucLogInfo, "%d#IO模件使用实际值,误码率: %d/%d\n",
//                             (int)(pspibuf->ucModAddr+1), (int)pspibuf->ulCheckSumErrCnt, (int)spiinfo.ulSPIRxErrCheckFreq);

//                     LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
//                 }

//                 pspibuf->ulOverThresholdCnt = 0;

//                 if (pspibuf->ulCheckSumErrCnt <= spiinfo.ulSPIRxErrRetLevel)
//                 {
//                     if (pspibuf->ulErrOccurCnt > 0)
//                     {
//                         pspibuf->ulErrOccurCnt--;
//                         if (pspibuf->ulErrOccurCnt == 0)
//                         {
//                             sprintf(aucLogInfo, "%d#IO模件通信正常,误码率: %d/%d\n",
//                                     (int)(pspibuf->ucModAddr+1), (int)pspibuf->ulCheckSumErrCnt, (int)spiinfo.ulSPIRxErrCheckFreq);

//                             LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
//                         }
//                     }
//                 }
//             }

//             pspibuf->ulCheckSumErrCnt=0;
//         }
//     }
//     if (spiinfo.ulEnterISRCnt >= spiinfo.ulSPIRxErrCheckFreq)
//     {
//         spiinfo.ulEnterISRCnt =0;
//     }
// }

// /* ISR for slave SPI.
//  * Para:
//  *     iVal, parameter for ISR.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Slave_ISR(int iVal)
// {
//     int i;
//     uint8_t uc;
//     int iLockKey;

//     iLockKey = intLock();  /* Prevent register R/W collision. */

//     spiinfo.ulIsrCnt++;

//     // uc=*SPIE(iIMMR_g);
//     // *SPIE(iIMMR_g)=0xFF;

//     // i=*(uint16_t*)(iIMMR_g+SPI_RBASE+BD_DATA_LENGTH_OFFSET);

//     // if ((uc & 0x01) && i==SPI_COM_LEN)
//     // {
//     //     spiinfo.ulFrmCnt++;

//     //     SPI_Recv();
//     // }

//     // /* initialize RxBD */
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET) = BD_RX_EMPTY_BIT |
//     //         BD_RX_WRAP_BIT | BD_RX_INTERRUPT_BIT;

// #if 0 /* 2009.10.22 合肥 */
//     /* initialize TxBD status. */
//     *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET) = BD_TX_READY_BIT |
//             BD_TX_WRAP_BIT | 0x0800;
// #endif

//     intUnlock(iLockKey);
// }

// /* ISR for Timer3.
//  * Para:
//  *     iVal, parameter for ISR.
//  * Return:
//  *     NONE.
//  */
// static void Timer3_ISR (int iVal)
// {
//     int iLockKey;

// //     /* Process timer3 interruput status word. */
// //     if(*TER3(iIMMR_g) & 0x02)
// //         *TER3(iIMMR_g) = 0xFFFF;        /* Clear timer3 interrupt status word. */
// //     else
// //     {
// //         *TER3(iIMMR_g) = 0xFFFF;        /* Clear timer3 interrupt status word. */

// //         return;
// //     }

// //     iLockKey = intLock();  /* Prevent register R/W collision. */
// // #if TEST_INT_RESP
// //     uint16_t t;

// //     t=*TCN3(iIMMR_g);

// //     tcn.tcncnt++;

// //     if(tcn.tcncnt>10)
// //     {
// //         if(t>tcn.tcn3max)
// //             tcn.tcn3max=t;
// //     }
// //     if(t<tcn.tcn3min)
// //         tcn.tcn3min=t;

// //     tcn.tcns+=t;
// //     tcn.tcnv=tcn.tcns/tcn.tcncnt;
// // #endif

// //     SPI_Snd();

// //     /* 为了保证消抖时间的准确性,每次中断时不再清除中断时间计数器 */
// //     //*TCN3(iIMMR_g) = 0;   /* Ensure the interval between two sending. */

//     intUnlock(iLockKey);
// }

// /* SPI data frame sending.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Snd(void)
// {
// #define BUFWR_ROUND (SPI_COM_LEN/16)
//     uint8_t *pOdd;
//     uint8_t *pEven;
//     uint8_t *p;
//     int8_t i;

//     // if (((*(uint16_t *)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET)) & BD_TX_READY_BIT)
//     //         || (!((*(uint16_t *)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET)) & BD_RX_EMPTY_BIT)))
//     // {
//     //     /* 包含假定:数据帧间隔时间大于100us; */
//     //     /* Process timer3 interruput status word. */
//     //     return;
//     // }

//     // *PDATD(iIMMR_g) &= (~0x00001000);   /* Assert /SPISEL for slave CPU. */

//     // /* 16位整型转化为两个字节，交换奇偶字节 */
//     // pOdd = spiWriteBuf;
//     // pEven = spiWriteBuf+1;
//     // p = (uint8_t *)(iIMMR_g+SPI_TX_BUF);

//     // for (i = 0; i<BUFWR_ROUND; i++)
//     // {
//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;

//     //     *p++ = *pEven;
//     //     *p++ = *pOdd;
//     //     pEven += 2;
//     //     pOdd += 2;
//     // }

//     // /* Initialize TxBD status. */
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET) = BD_TX_READY_BIT |
//     //         BD_TX_WRAP_BIT | 0x0800;

//     // /* Start sending data. */
//     // *SPCOM(iIMMR_g) = 0x80;
// }

// /* ISR for master SPI.
//  * Para:
//  *     iVal, parameter for ISR.
//  * Return:
//  *     NONE.
//  */
// static void SPI_ISR(int iVal)
// {
//     uint8_t uc;
//     int iLockKey;
//     int i;

//     spiinfo.ulIsrCnt++;

//     iLockKey = intLock();  /* Prevent register R/W collision. */
//     // uc=*SPIE(iIMMR_g);
//     // *SPIE(iIMMR_g)=0xFF;

//     // *PDATD(iIMMR_g) |= 0x00001000;      /* Cancel /SPISEL for slave CPU. */

//     // i = *(uint16_t *)(iIMMR_g+SPI_RBASE+BD_DATA_LENGTH_OFFSET);
//     // if ((uc & 0x01) && (i == SPI_COM_LEN))
//     //     SPI_Recv();

//     // /* Initialize RxBD. */
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET) = BD_RX_EMPTY_BIT |
//     //         BD_RX_WRAP_BIT | BD_RX_INTERRUPT_BIT;

//     intUnlock(iLockKey);
// }

// /* initialize the slave SPI driver.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SPI_Slave_Init(void)
// {
//     int i;
//     int iLockKey;
//     u_int uiFindMod;
//     u_int uiLstFind;
//     int iModNum;
//     int iModNoChg;
//     int iRetry;
//     ULONG ulLastRpt;
//     RD_PART_INFO *p_part;

//     /* initialize spi<->io buffer */
//     for (i = 0; i<MAX_MOD_NUM; i++)
//     {
//         /* Temporary used to enable SPI receive in SPI_Slave_ISR. */
//         aspibuf_g[i].ucModType = DO_MODULE;
//         aspibuf_g[i].ucModAddr = i;
//         aspibuf_g[i].bComErr = TRUE;
//         aspibuf_g[i].pfDealRecv = NULL;
//         /* Initialize spi<->IO buffer semphore. */
//         aspibuf_g[i].semMod = semMCreate(SEM_Q_PRIORITY);
//         aspibuf_g[i].ulCheckSumErrCnt = 0;
//         aspibuf_g[i].ulExcCnt = 0;
//         aspibuf_g[i].ulTotalExcCnt = 0;
//         aspibuf_g[i].ulRebootCnt = 0;
//         aspibuf_g[i].bRebootFlag = FALSE;
//         aspibuf_g[i].bErrOvFlag = FALSE;
//         aspibuf_g[i].ulErrOccurCnt = 0;
//         aspibuf_g[i].ulCheckSumErrStat = 0;
//         aspibuf_g[i].bUsed = FALSE;
//         aspibuf_g[i].bDIDOExecFlag = FALSE;
//         aspibuf_g[i].ulLastDIDOExecTime = 0;
//         aspibuf_g[i].bCycleCommandSndFlag = FALSE;  /* 循环发送命令 */
//         aspibuf_g[i].bInvalid = FALSE;		/* 失效 */
//         aspibuf_g[i].bBreakdown = FALSE;		/* 击穿 */
//         aspibuf_g[i].bReset = FALSE;	/* 重启 */
//         aspibuf_g[i].bSpiComError = FALSE;			/* SPI通讯错 */
//         aspibuf_g[i].bQDInvalid = FALSE;		/* 启动失效 */
//         aspibuf_g[i].ucDiGroupNum = 0;
//         aspibuf_g[i].ucAiChnCnt = 0;
//         aspibuf_g[i].bGetDiFlag = TRUE;
//         aspibuf_g[i].bGetAiFlag = FALSE;
//         aspibuf_g[i].bChgBaseReg = FALSE;
//         aspibuf_g[i].bSetDoFlag = FALSE;
//         aspibuf_g[i].ulSetDoCnt = 0;
//         aspibuf_g[i].bSetCmdFlag = FALSE;
//         aspibuf_g[i].bCRCCheckMod = TRUE;  /* 缺省为CRC模式 */
//         aspibuf_g[i].bCheckModAffirm = FALSE;  /* 初始进行校验法确认 */
//         aspibuf_g[i].ulSumCheckCnt = 0;   /* 求和取反校验正确次数 */
//         aspibuf_g[i].ulCRCCheckCnt = 0;      /* CRC校验正确次数 */
//         aspibuf_g[i].ulOverThresholdCnt = 0;
//         aspibuf_g[i].bDefaultFlag = FALSE;
//         aspibuf_g[i].ulSwitchDefaultCnt = 0;
//         aspibuf_g[i].ulSwitchDefaultTm = 0;
//         aspibuf_g[i].bErrFrmLogFlag = FALSE;
//         aspibuf_g[i].ulSPIDIRcvTimes=0;
//     }

//     for (p_part = apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
//     {
//         if(((p_part->ucType == DO_MODULE)
//                 || (p_part->ucType == DI_MODULE)
//                 || (p_part->ucType == DIO_MODULE)
//                 || (p_part->ucType == CKDIO_MODULE)
//                 /* || (p_part->ucType == AI_MODULE) */  /* 模拟量输入 */
//                 || (p_part->ucType == AO_MODULE)	/* 模拟量输出 */
//                 || (p_part->ucType == COM_MODULE))    /* 通用类型 */
//                 && (p_part->ucPosition == 0))
//         {
//             /* 类型由配置而来 */
//             aspibuf_g[p_part->aucHwAddr[0]].bUsed = TRUE;	/* 设置该模件被使用 */
//         }
//     }

//     /* initialize interrupt vect */
//     // intConnect(INUM_TO_IVEC(INUM_SPI), SPI_Slave_ISR, 0);

//     /* Initialize SPI. */
//     SPI_Slave_Hw_Init();

//     /* Enable SPI interrupt */
//     // intEnable(INUM_SPI);

//     ulLastRpt = 0;
//     iModNoChg = 0;
//     uiLstFind = 0;
//     for (iRetry = 0; iRetry<600*SYS_SEC/WAIT_SPI_RX; iRetry++)
//     {
//         taskDelay(WAIT_SPI_RX);         /* Wait for SPI communication. */

//         //assert (MAX_MOD_NUM <= 16);
//         uiFindMod = 0;
//         iModNum = 0;
//         for (i = 0; i<MAX_MOD_NUM; i++)
//         {
//             if (!aspibuf_g[i].bComErr && aspibuf_g[i].aucUpFrame[0])
//             {
//                 uiFindMod |= BV16(i);
//                 iModNum++;
//                 aspibuf_g[i].unVer = 0xFFFF;
//             }
//             else
//                 aspibuf_g[i].unVer = 0;
//         }

//         if (uiFindMod == uiLstFind)
//         {
//             iModNoChg++;
//             if (uiFindMod && iModNoChg>5)
//                 break;
//         }
//         else
//         {
//             uiLstFind = uiFindMod;
//             iModNoChg = 0;
//         }

//         if ((long)(tickGet()-ulLastRpt)>5*SYS_SEC)
//         {
//             LOG_Dbg_Msg("Try to synchronize slave SPI.\n", 0, 0, 0, 0, 0, 0);
//             ulLastRpt = tickGet();
//         }

//     }

//     if (!spiinfo.ulFrmCnt)
//     {
//         assert(!iModNum);

//         if (ENG_MODE == 0)
//             ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
//                        "IO模件通信异常\n", 0, 0);
//         else if (ENG_MODE == 1)
//             ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
//                        "IO com. error\n", 0, 0);
//         EP_Set_Sts_Bit(REBOOT_DLY);

//         if (spiinfo.ulIsrCnt)
//         {
//             LOG_Write(LOG_KERNEL, "CPU无SPI数据帧接收!!\n", NULL);
//         }
//         else
//         {
//             LOG_Write(LOG_KERNEL, "CPU无SPI中断!!\n", NULL);
//         }

//     }
//     else
//     {
//         LOG_Dbg_Msg("Synchronize slave SPI: found %d sub-IO-boards.\n",
//                     iModNum, 0, 0, 0, 0, 0);
//     }

//     iLockKey = intLock();

//     for (i = 0; i<MAX_MOD_NUM; i++)
//     {
//         /* Initialize them as IDLE first.  They will be changed by SIO_Init_DI
//          * or SIO_Init_DO.
//          */
//         aspibuf_g[i].ucModType = IDLE_MODULE;
//     }

//     // /* 2009-9-9 zy */
//     // if (IoPinInputHigh(IO_IN_PNL_DI) != IO_FUN_NULL)
//     // {
//     //     aspibuf_g[DIADDRONCPU].ucModType = DIO_MODULE;		/* 模件初始化 */
//     //     aspibuf_g[DIADDRONCPU].ucModAddr = DIADDRONCPU;
//     //     aspibuf_g[DIADDRONCPU].unDiChNum = DIADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unDoChNum = DOADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unAiChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].pfDealRecv = SIO_Recv_DI; 		/* 处理程序入口 */
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = IO_IN_PNL_DI;
//     // }
//     // else if (IoPinInputHigh(IO_IN_CONNECTOR_DI) != IO_FUN_NULL)
//     // {
//     //     aspibuf_g[DIADDRONCPU].ucModType = DI_MODULE;		/* 模件初始化 */
//     //     aspibuf_g[DIADDRONCPU].ucModAddr = DIADDRONCPU;
//     //     aspibuf_g[DIADDRONCPU].unDiChNum = DIADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unDoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAiChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].pfDealRecv = SIO_Recv_DI; 		/* 处理程序入口 */
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = IO_IN_CONNECTOR_DI;
//     // }
//     // else
//     // {
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = -1;
//     // }


//     SIO_Match_IO_Slave();

//     intUnlock(iLockKey);

//     /* 防止超前主CPU运行而在初次扫描时获取不到开入状态
//      */
//     taskDelay(300);

//     spiinfo.bIOInitFinish = TRUE;
// }

// /* write the down frame.
//  * Para:
//  *     iModAddr, address of the module.
//  *     pucData, address of data.
//  * Return:
//  *     NONE.
//  */
// void SPI_Write(int iModAddr, uint8_t *pucData)
// {
//     int iLockKey;
//     uint8_t ucSum;
//     uint8_t *pucDown;
//     uint8_t *pucTx;

//     assert (iModAddr<MAX_MOD_NUM);

//     /* 区分CRC校验和求和取反 */
//     if (aspibuf_g[iModAddr].bCRCCheckMod)
//     {
//         ucSum = CDT_BCH_Check(pucData, SPI_FRAME_LEN_PER_MOD-1);
//     }
//     else
//     {
//         /* Computer ucSum code. */
//         ucSum = pucData[0];
//         ucSum += pucData[1];
//         ucSum += pucData[2];
//         ucSum += pucData[3];
//         ucSum += pucData[4];
//         ucSum = ~ucSum;
//     }

//     /* Copy TX data. */

//     pucDown = aspibuf_g[iModAddr].aucDownFrame;
//     pucTx = (uint8_t *)(spiWriteBuf+iModAddr);    /* writing buffer. */

//     iLockKey = intLock();

//     *pucDown++ = *pucData;
//     *pucTx = *pucData++;

//     *pucDown++ = *pucData;
//     *(pucTx+0x10) = *pucData++;

//     *pucDown++ = *pucData;
//     *(pucTx+0x20) = *pucData++;

//     *pucDown++ = *pucData;
//     *(pucTx+0x30) = *pucData++;

//     *pucDown++ = *pucData;
//     *(pucTx+0x40) = *pucData++;

//     *pucDown = ucSum;
//     *(pucTx+0x50) = ucSum;

//     intUnlock(iLockKey);
// }

// /* read the up frame.
//  * Para:
//  *     iModAddr, address of the module.
//  *     pucData, address of data.
//  * Return:
//  *     EP_SUCCESS, EP_COM_ERR.
//  */
// static EP_STATUS SPI_Read(int iModAddr, uint8_t *pucData)
// {
//     int iLockKey;
//     uint8_t *pucRx;
//     EP_STATUS stsCom;

//     assert (iModAddr<MAX_MOD_NUM);

//     /* Copy RX data. */

//     pucRx = aspibuf_g[iModAddr].aucUpFrame;

//     iLockKey = intLock();

//     *pucData++ = *pucRx++;
//     *pucData++ = *pucRx++;
//     *pucData++ = *pucRx++;
//     *pucData++ = *pucRx++;
//     *pucData++ = *pucRx++;

//     if (aspibuf_g[iModAddr].bComErr)
//         stsCom = EP_COM_ERR;
//     else
//         stsCom = EP_SUCCESS;

//     intUnlock(iLockKey);

//     return stsCom;
// }

// /* intialize the whole SPI-IO driver module.
//  * Para:
//  *     NONE.
//  * Return:
//  *     EP_SUCCESS, EP_BUF_ERR, or EP_COM_ERR.
//  */
// EP_STATUS SIO_Initialize(void)
// {
//     int i;
//     int j;
//     int iRetry;
//     uint8_t aucBuf[6];
//     char TempInfo[256];
//     uint8_t aucVer[32]="";/*2009-11-18 ZY   */
//     uint8_t aucItemKey[128]="";

//     spiinfo.bIOInitFinish = FALSE;		/* IO模件初始化完成标志 */
//     spiinfo.ulIsrCnt = 0;
//     spiinfo.ulFrmCnt = 0;
//     spiinfo.ulEnterISRCnt = 0;		/* SPI中断次数 */
//     spiinfo.ulSPIRxErrCheckFreq = SPI_RXERR_CHK_FREQ;
//     spiinfo.ulSPIRxErrAlmLevel = SPI_RXERR_ALM_LEVEL;
//     spiinfo.ulSPIRxErrLogLevel = SPI_RXERR_LOG_LEVEL;
//     spiinfo.ulSPIRxErrRetLevel = SPI_RXERR_RET_LEVEL;
//     spiinfo.ulMaxErrCnt = MAX_ERR_CNT;
//     spiinfo.ulCommandDelayCnt = COMMANDSNDMAXDEALYCNT;
//     spiinfo.ulSPIDelayCnt = WAIT_SPI_RX;
//     spiinfo.ulOverThreshold = SPI_OVER_THRESHOLD_LEVEL;

//     /* SPI主从,允许设置
//      */
//     if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//             || (ucCpuSpiRol_g == 2))
//     {
//         SPI_Slave_Init();

//         return EP_SUCCESS;
//     }

//     /* Initialize SPI. */
//     SPI_Driver_Init();

//     /* Set all module up frame ctrl and attribute word to read edition info. */
//     aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
//     aucBuf[1] = 0;
//     aucBuf[2] = SPI_BYTES(4) | IO_TYPE_REG;
//     aucBuf[3] = 0xA5;
//     aucBuf[4] = 0x5A;
//     for (i=0; i<MAX_MOD_NUM; i++)
//     {
//         SPI_Write(i, aucBuf);
//         /* 暂时使用 */
//         aspibuf_g[i].ucModType = DO_MODULE;
//     }

//     taskDelay(WAIT_SPI_RX);             /* Wait for SPI communication. */

//     /* 重复发两次, 满足2.2版本要求 */
//     for (i=0; i<MAX_MOD_NUM; i++)
//     {
//         SPI_Write(i, aucBuf);
//         /* 暂时使用 */
//         aspibuf_g[i].ucModType = DO_MODULE;
//     }

//     taskDelay(WAIT_SPI_RX);             /* Wait for SPI communication. */

//     for (i = 0; i<MAX_MOD_NUM; i++)
//     {
//         /* Initialize module address. */
//         aspibuf_g[i].ucModAddr = (uint8_t)i;
//         aspibuf_g[i].ulSPIDIRcvTimes=0;

//         for (iRetry=IO_INIT_RETRY; iRetry>0; taskDelay(WAIT_SPI_RX), iRetry--)
//         {
//             /* Judge module communication state. */
//             if (!aspibuf_g[i].bComErr)
//             {
//                 SPI_Read(i, aucBuf);    /* 读取上行帧数据 */

//                 /* Judge module reply data */
//                 if (aucBuf[0] == (SPI_BYTES(4) | IO_TYPE_REG))
//                 {

//                     /* Set module type and edition info */
//                     if (aucBuf[1] == 0x11)
//                         aspibuf_g[i].ucModType = DO_MODULE;
//                     else if (aucBuf[1] == 0x12)
//                         aspibuf_g[i].ucModType = DI_MODULE;
//                     else if(aucBuf[1] == 0x13)
//                         aspibuf_g[i].ucModType = DIO_MODULE;		/* DIO，类型由IO子模件来 */
//                     else if (aucBuf[1] == 0x14)
//                         aspibuf_g[i].ucModType = AI_MODULE;   /* AI module. */
//                     else if (aucBuf[1] == 0x15)
//                         aspibuf_g[i].ucModType = AO_MODULE;	   /* AO module. */
//                     else if(aucBuf[1] == 0x16)
//                         aspibuf_g[i].ucModType = CKDIO_MODULE;		/* 测控DIO */
//                     else if (aucBuf[1] == 0x17)
//                         aspibuf_g[i].ucModType = COM_MODULE; /* COM module. */
//                     else
//                         continue;

//                     aspibuf_g[i].ucDesignSN = aucBuf[2];
//                     aspibuf_g[i].unVer = aucBuf[3]+(aucBuf[4]<<8);

//                     /* 写入IO板的版本信息到系统日志　2009-11-18 ZY */
//                     sprintf(aucItemKey,"%d号IO板版本",i+1);
//                     sprintf(aucVer,"%x.%02x",HI8(aspibuf_g[i].unVer),LO8(aspibuf_g[i].unVer));
//                     LOG_ExtraItemWrite(aucItemKey,aucVer);

//                     aspibuf_g[i].unAiChNum = 0;
//                     aspibuf_g[i].unAoChNum = 0;
//                     aspibuf_g[i].unDiChNum = 0;
//                     aspibuf_g[i].unDoChNum = 0;

//                     /* I/O模件检验模式判断
//                      * 2.20以上, 排除5.*版本
//                      */
//                     if (((aucBuf[4] == 0x02) && (aucBuf[3] >= 0x20))
//                             || ((aucBuf[4] > 0x02) && (aucBuf[4] != 0x05)))
//                     {
//                         aspibuf_g[i].bIOCheckMod = TRUE; /* CRC */
//                         aspibuf_g[i].bCRCCheckMod = TRUE;
//                         aspibuf_g[i].bCheckModAffirm = TRUE;
//                     }
//                     else
//                     {
//                         aspibuf_g[i].bIOCheckMod = FALSE;  /* SUM */
//                         aspibuf_g[i].bCRCCheckMod = FALSE;
//                         aspibuf_g[i].bCheckModAffirm = TRUE;
//                     }

//                     /* Set all module up frame ctrl and attribute word to read edition info.
//                      * 根据版本号重新设定
//                      */
//                     aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
//                     aucBuf[1] = 0; /* 按寄存器0x09方式上传 */
//                     aucBuf[2] = SPI_BYTES(4) | IO_TYPE_REG;
//                     aucBuf[3] = 0xA5;
//                     aucBuf[4] = 0x5A;

//                     SPI_Write(i, aucBuf);
//                     break;
//                 }
//                 else  if((aucBuf[0] == (SPI_BYTES(1) | IO_STATUS_REG))
//                          || (aucBuf[0] == IO_STATUS_REG))  /* check OK. */
//                 {
//                     /* 若检测到IO板故障，则出错，添加 */

//                     LOG_Dbg_Msg("Error, detect IO module at address %d is error.\n", i+1, 0, 0, 0, 0, 0);

//                     if (aspibuf_g[i].bUsed)
//                     {
//                         if (ENG_MODE == 0)
//                             ER_Set_Err(EV_DIDO_ERR,
//                                        ER_REPORT|ER_ALARM|ER_LOCK,
//                                        "模件地址:%d\n",
//                                        i+1, 0);
//                         else if (ENG_MODE == 1)
//                             ER_Set_Err(EV_DIDO_ERR,
//                                        ER_REPORT|ER_ALARM | ER_LOCK,
//                                        "module address:%d\n",
//                                        i+1, 0);
//                     }

//                     iRetry = 0;

//                     break;

//                 }
//             }
//         }

//         if (!iRetry)
//         {
//             aspibuf_g[i].ucModType = IDLE_MODULE;
//             continue;
//         }
//     }

//     /* 等待IO模件确认校验法 */
//     taskDelay(10*WAIT_SPI_RX);             /* Wait for SPI communication. */

//     /* 版本确认和属性读取/设置分开 */
//     for (i = 0; i<MAX_MOD_NUM; i++)
//     {
//         /* 空模件不处理 */
//         if (aspibuf_g[i].ucModType == IDLE_MODULE)
//         {
//             continue;
//         }

//         for (iRetry = IO_INIT_RETRY; iRetry>0; iRetry--)
//         {
//             switch (aspibuf_g[i].ucModType)
//             {
//                 case DO_MODULE:
//                     if (SIO_Read_Reg(i, DO_CH_NUM_REG, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_DO_PER_MOD)
//                         {
//                             iRetry = -1;      /* Initialize OK flag. */
//                             aspibuf_g[i].unDoChNum = aucBuf[0];

//                             LOG_Dbg_Msg("Find DO module at address %d. Channel=%d, Software ver=%x.\n",
//                                         i+1, aspibuf_g[i].unDoChNum, aspibuf_g[i].unVer, 0, 0, 0);

//                             /* 通知允许DO板上送系统事件，开出模件的输入监视，通道数，IO故障信息 */
//                             aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
//                             aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
//                             aucBuf[2] = SPI_BYTES(4) | DO_SYS_EVENT;				/* 开出板系统事件寄存器开始 */
//                             aucBuf[3] = 0x5A;
//                             aucBuf[4] = 0xA5;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                             /* 初始化开出板 */
//                             aucBuf[0] = SPI_BYTES(4) | DO_OUTPUT_REG;
//                             aucBuf[1] = 0;   /* 定义允许自定义方式上送 */
//                             aucBuf[2] = 0;
//                             aucBuf[3] = 0;
//                             aucBuf[4] = 0;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                         }
//                     }
//                     break;

//                 case CKDIO_MODULE:
//                     if (SIO_Read_Reg(i, DO_CH_NUM_REG, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_DO_PER_MOD)
//                         {
//                             iRetry = -1;      /* Initialize OK flag. */
//                             aspibuf_g[i].unDoChNum = aucBuf[0];

//                             LOG_Dbg_Msg("Find CKDIO module at address %d. DO channel=%d. DI chanel=%d, Software ver=%x.\n",
//                                         i+1, aspibuf_g[i].unDoChNum, 17, aspibuf_g[i].unVer, 0, 0);		/* 目前最多17个DI */


//                             /* 通知允许CKDO板上送开出返回，及IO故障信息 */
//                             aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;			/* 读写两个字节，基址为IO_UF_CTRL_REG */
//                             aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
//                             aucBuf[2] = SPI_BYTES(4) | CKDOFEEDBAKC1;	/* 测控开出板返回字节1开始 */
//                             aucBuf[3] = 0x5A;
//                             aucBuf[4] = 0xA5;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                             /* 初始化开出板 */
//                             aucBuf[0] = SPI_BYTES(4) | DO_OUTPUT_REG;
//                             aucBuf[1] = 0;   /* 定义允许自定义方式上送 */
//                             aucBuf[2] = 0;
//                             aucBuf[3] = 0;
//                             aucBuf[4] = 0;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                             /* Install receive process subrotine. */
//                             aspibuf_g[i].pfDealRecv = SIO_Recv_DI;

//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                         }
//                     }
//                     break;

//                 case DI_MODULE:
//                     if (SIO_Read_Reg(i, DI_CH_NUM_REG, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_DI_PER_MOD)
//                         {
//                             iRetry = -1;      /* Initialize OK flag. */
//                             aspibuf_g[i].unDiChNum = aucBuf[0];

//                             /* 通知允许IO板上送出错信息 */
//                             aucBuf[0] = SPI_BYTES(1) | IO_UF_CTRL_REG;
//                             aucBuf[1] = 1;   		/* 定义允许自定义方式上送 */
//                             aucBuf[2] = 0X5A;
//                             aucBuf[3] = 0xA5;
//                             aucBuf[4] = 0x5A;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                             /* Start read digital input */
//                             SIO_Read_Reg(i, DI_INPUT_REG, 4, aucBuf, TRUE);

//                             /* Install receive process subrotine. */
//                             aspibuf_g[i].pfDealRecv = SIO_Recv_DI;

//                             LOG_Dbg_Msg("Find DI module at address %d. Channel=%d, Software ver=%x.\n",
//                                         i+1, aspibuf_g[i].unDiChNum, aspibuf_g[i].unVer, 0, 0, 0);
//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                         }
//                     }
//                     break;

//                 case DIO_MODULE:
//                     LOG_Dbg_Msg("开入开出模件.\n", 0, 0, 0, 0, 0, 0);
//                     break;

//                 case AI_MODULE:
//                     LOG_Dbg_Msg("模拟输入模件.\n", 0, 0, 0, 0, 0, 0);
//                     break;

//                 case AO_MODULE:
//                     LOG_Dbg_Msg("模拟输出模件.\n", 0, 0, 0, 0, 0, 0);
//                     break;

//                 case COM_MODULE:
//                     LOG_Dbg_Msg("通用IO模件.\n", 0, 0, 0, 0, 0, 0);

//                     /* DI. */
//                     aucBuf[0] = SPI_BYTES(1) | IO_CPU_TYPE_SET;
//                     aucBuf[1] = 1;
//                     aucBuf[2] = 0X5A;
//                     aucBuf[3] = 0xA5;
//                     aucBuf[4] = 0x5A;
//                     SPI_Write(i, aucBuf);
//                     taskDelay(WAIT_SPI_RX);

//                     if (SIO_Read_Reg(i, IO_CHN_NUM, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_DI_PER_MOD)
//                         {
//                             aspibuf_g[i].unDiChNum = aucBuf[0];

//                             /* Install receive process subrotine. */
//                             aspibuf_g[i].pfDealRecv = SIO_Recv_AI_DI;
//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                             break;
//                         }
//                     }

//                     if (aspibuf_g[i].unDiChNum)
//                     {
//                         aspibuf_g[i].bGetDiFlag = TRUE;
//                         LOG_Dbg_Msg("Find DI channel at address %d. Channel=%d, Software ver=%x.\n",
//                                     i+1, aspibuf_g[i].unDiChNum, aspibuf_g[i].unVer, 0, 0, 0);
//                     }
//                     else
//                     {
//                         aspibuf_g[i].bGetDiFlag = FALSE;
//                     }

//                     /* DO. */
//                     aucBuf[0] = SPI_BYTES(1) | IO_CPU_TYPE_SET;
//                     aucBuf[1] = 2;
//                     aucBuf[2] = 0X5A;
//                     aucBuf[3] = 0xA5;
//                     aucBuf[4] = 0x5A;
//                     SPI_Write(i, aucBuf);
//                     taskDelay(WAIT_SPI_RX);

//                     if (SIO_Read_Reg(i, IO_CHN_NUM, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_DO_PER_MOD)
//                         {
//                             aspibuf_g[i].unDoChNum = aucBuf[0];
//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                             break;
//                         }
//                     }

//                     /* AI. */
//                     aucBuf[0] = SPI_BYTES(1) | IO_CPU_TYPE_SET;
//                     aucBuf[1] = 3;
//                     aucBuf[2] = 0X5A;
//                     aucBuf[3] = 0xA5;
//                     aucBuf[4] = 0x5A;
//                     SPI_Write(i, aucBuf);
//                     taskDelay(WAIT_SPI_RX);

//                     if (SIO_Read_Reg(i, IO_CHN_NUM, 1, aucBuf, FALSE) == EP_SUCCESS)
//                     {
//                         if (aucBuf[0] <= MAX_VTBOX_AI_NUM)
//                         {
//                             aspibuf_g[i].unAiChNum = aucBuf[0];
//                         }
//                         else
//                         {
//                             /* assert(FALSE); */
//                             break;
//                         }
//                     }

//                     if (aspibuf_g[i].unDoChNum)
//                     {
//                         LOG_Dbg_Msg("Find DO channel at address %d. Channel=%d, Software ver=%x.\n",
//                                     i+1, aspibuf_g[i].unDoChNum, aspibuf_g[i].unVer, 0, 0, 0);

//                         aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
//                         aucBuf[1] = 0;   		/* 按寄存器0x09方式上传 */
//                         aucBuf[2] = SPI_BYTES(4) | DO_COM_STATUS_REG;
//                         aucBuf[3] = 0x5A;
//                         aucBuf[4] = 0xA5;
//                         SPI_Write(i, aucBuf);
//                         taskDelay(WAIT_SPI_RX);

//                         /* 初始化开出板 */
//                         aucBuf[0] = SPI_BYTES(4) | DO_COM_OUTPUT_REG;
//                         aucBuf[1] = 0;   /* 定义允许自定义方式上送 */
//                         aucBuf[2] = 0;
//                         aucBuf[3] = 0;
//                         aucBuf[4] = 0;
//                         SPI_Write(i, aucBuf);
//                         taskDelay(WAIT_SPI_RX);
//                     }

//                     if (aspibuf_g[i].unAiChNum)
//                     {
//                         aspibuf_g[i].bGetAiFlag = TRUE;
//                         LOG_Dbg_Msg("Find AI channel at address %d. Channel=%d, Software ver=%x.\n",
//                                     i+1, aspibuf_g[i].unAiChNum, aspibuf_g[i].unVer, 0, 0, 0);

//                         for (j=0; j<aspibuf_g[i].unAiChNum; j++)
//                         {
//                             aucBuf[0] = SPI_BYTES(2) | IO_CHN_RD_CTRL;
//                             aucBuf[1] = 1;
//                             aucBuf[2] = j+1;
//                             aucBuf[3] = 0xA5;
//                             aucBuf[4] = 0x5A;
//                             SPI_Write(i, aucBuf);
//                             taskDelay(WAIT_SPI_RX);

//                             if (SIO_Read_Reg(i, IO_CHN_RD_CTRL, 4, aucBuf, FALSE) == EP_SUCCESS)
//                             {
//                                 if ((aucBuf[0] == 0x01) && (aucBuf[1] == (j+1)))
//                                 {
//                                     aspibuf_g[i].aOvValBuf[j]=U8_TO_U16(aucBuf[3], aucBuf[2]);
//                                 }
//                                 else
//                                 {
//                                     /* assert(FALSE); */
//                                     break;
//                                 }
//                             }
//                         }

//                         aucBuf[0] = SPI_BYTES(2) | IO_CHN_RD_CTRL;
//                         aucBuf[1] = 2;
//                         aucBuf[2] = 1;		/* read the first channel. */
//                         aucBuf[3] = 0xA5;
//                         aucBuf[4] = 0x5A;
//                         SPI_Write(i, aucBuf);
//                         taskDelay(WAIT_SPI_RX);
//                     }
//                     else
//                     {
//                         aspibuf_g[i].bGetAiFlag = FALSE;
//                     }

//                     /* Install receive process subrotine. */
//                     aspibuf_g[i].pfDealRecv=SIO_Recv_AI_DI;

//                     iRetry=-1;      /* Initialize OK flag. */

//                     break;

//                 default:
//                     /* assert(FALSE); */
//                     break;
//             }
//         }

//         if (!iRetry)
//         {
//             if (aspibuf_g[i].bUsed)
//             {
//                 if (ENG_MODE == 0)
//                 {
//                     ER_Set_Err(EV_DIDO_ERR, ER_REPORT|ER_ALARM|ER_NOLOGWRITE|ER_LOCK,
//                                "模件地址:%d,错误码:%02d\n", i+1, SUB_MOD_INIT_ERR);
//                 }
//                 else if (ENG_MODE == 1)
//                 {
//                     ER_Set_Err(EV_DIDO_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
//                                "Module address:%d, error code:%02d\n", i+1, SUB_MOD_INIT_ERR);
//                 }
//                 sprintf(TempInfo, "%d#IO模件初始化失败!!\n", i+1);
//                 LOG_Write(LOG_KERNEL, TempInfo, NULL);
//             }
//         }
//     }

//     // if (IoPinInputHigh(IO_IN_PNL_DI) != IO_FUN_NULL)
//     // {
//     //     aspibuf_g[DIADDRONCPU].ucModType = DIO_MODULE;		/* 模件初始化 */
//     //     aspibuf_g[DIADDRONCPU].ucModAddr = DIADDRONCPU;
//     //     aspibuf_g[DIADDRONCPU].unDiChNum = DIADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unDoChNum = DOADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unAiChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].pfDealRecv = SIO_Recv_DI; 		/* 处理程序入口 */
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = IO_IN_PNL_DI;
//     // }
//     // else if (IoPinInputHigh(IO_IN_CONNECTOR_DI) != IO_FUN_NULL)
//     // {
//     //     aspibuf_g[DIADDRONCPU].ucModType = DI_MODULE;		/* 模件初始化 */
//     //     aspibuf_g[DIADDRONCPU].ucModAddr = DIADDRONCPU;
//     //     aspibuf_g[DIADDRONCPU].unDiChNum = DIADDRONCPUNUM;
//     //     aspibuf_g[DIADDRONCPU].unDoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAiChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].unAoChNum = 0;
//     //     aspibuf_g[DIADDRONCPU].pfDealRecv = SIO_Recv_DI; 		/* 处理程序入口 */
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = IO_IN_CONNECTOR_DI;
//     // }
//     // else
//     // {
//     //     aspibuf_g[DIADDRONCPU].iMbDiSrcType = -1;
//     // }

//     SIO_Match_IO();

//     taskDelay(100);

//     /* 设置IO初始化完成标志 */
//     spiinfo.bIOInitFinish = TRUE;

//     return EP_SUCCESS;
// }

// /* match the IO module between configuration and slot.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Match_IO(void)
// {
//     RD_PART_INFO *p_part;
//     char auctemp[256];

//     for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
//     {
//         if(((p_part->ucType == DO_MODULE)
//                 || (p_part->ucType == DI_MODULE)
//                 || (p_part->ucType == DIO_MODULE)
//                 || (p_part->ucType == CKDIO_MODULE)
//                 || (p_part->ucType == COM_MODULE))
//                 && (p_part->ucPosition == 0) && aspibuf_g[p_part->aucHwAddr[0]].bUsed)
//         {
//             /* 开出模件，开入模件或开入开出模件，只是主机箱这么处理 */

//             aspibuf_g[p_part->aucHwAddr[0]].bMatched = TRUE;	/* matched. */
//             if((p_part->ucType == DO_MODULE) && (aspibuf_g[p_part->aucHwAddr[0]].ucModType == DO_MODULE))
//             {
//                 continue;
//             }
//             else if((p_part->ucType == DI_MODULE) && (aspibuf_g[p_part->aucHwAddr[0]].ucModType == DI_MODULE))
//             {
//                 continue;
//             }
//             else if((p_part->ucType == DIO_MODULE)
//                     && ((aspibuf_g[p_part->aucHwAddr[0]].ucModType == DIO_MODULE) || (aspibuf_g[p_part->aucHwAddr[0]].ucModType == DO_MODULE)))
//             {
//                 continue;
//             }
//             else if((p_part->ucType == CKDIO_MODULE) && (aspibuf_g[p_part->aucHwAddr[0]].ucModType == CKDIO_MODULE))
//             {
//                 continue;
//             }
//             else if ((p_part->ucType == COM_MODULE)
//                      && (aspibuf_g[p_part->aucHwAddr[0]].ucModType == COM_MODULE))
//             {
//                 continue;
//             }

//             if(ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR, ER_ALARM|ER_LOCK|ER_REPORT | ER_NOLOGWRITE,
//                            "模件地址:%d,错误码:%02d\n", p_part->aucHwAddr[0]+1, SUB_MOD_INIT_ERR);	/* 地址从1开始，与配置一致 */
//             }
//             else if(ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR, ER_ALARM|ER_LOCK|ER_REPORT | ER_NOLOGWRITE,
//                            "Module address:%d, error code:%02d\n", p_part->aucHwAddr[0]+1, SUB_MOD_INIT_ERR);
//             }
//             sprintf(auctemp, "%d#IO模件类型校验错误,实际类型:%s,配置类型:%s!!\n",
//                     p_part->aucHwAddr[0]+1,
//                     IO_GetModDesInfo(aspibuf_g[p_part->aucHwAddr[0]].ucModType),
//                     IO_GetModDesInfo(p_part->ucType));
//             LOG_Write(LOG_KERNEL, auctemp, NULL);

//             aspibuf_g[p_part->aucHwAddr[0]].bMatched = FALSE;	/* not matched. */
//         }
//     }
// }

// /* match the IO module between configuration and slot on slave CPU.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Match_IO_Slave(void)
// {
//     RD_PART_INFO *p_part;

//     for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
//     {
//         if(((p_part->ucType == DO_MODULE)
//                 || (p_part->ucType == DI_MODULE)
//                 || (p_part->ucType == DIO_MODULE)
//                 || (p_part->ucType == CKDIO_MODULE)
//                 || (p_part->ucType == COM_MODULE))
//                 && (p_part->ucPosition == 0) && aspibuf_g[p_part->aucHwAddr[0]].bUsed)
//         {
//             /* 开出模件，开入模件或开入开出模件，只是主机箱这么处理 */

//             aspibuf_g[p_part->aucHwAddr[0]].bMatched = TRUE;	/* matched. */
//             if (aspibuf_g[p_part->aucHwAddr[0]].unVer)
//             {
//                 continue;
//             }

//             if (ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR, ER_ALARM|ER_LOCK|ER_REPORT,
//                            "模件地址:%d,错误码:%02d\n", p_part->aucHwAddr[0]+1, SUB_MOD_INIT_ERR);	/* 地址从1开始，与配置一致 */
//             }
//             else if (ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR, ER_ALARM|ER_LOCK|ER_REPORT,
//                            "Module address:%d, error code:%02d\n", p_part->aucHwAddr[0]+1, SUB_MOD_INIT_ERR);
//             }

//             aspibuf_g[p_part->aucHwAddr[0]].bMatched = FALSE;	/* not matched. */
//         }
//     }
// }

// /* Judge if the IO module exist.
//  * Para:
//  *     NONE.
//  * Return:
//  *     TRUE, or FALSE.
//  */
// BOOL SIO_GetIOExsitSts(int iModAddr)
// {
//     if (!aspibuf_g[iModAddr].bUsed)
//     {
//         return FALSE;
//     }
//     else if (!aspibuf_g[iModAddr].bMatched)
//     {
//         return FALSE;
//     }
//     else
//     {
//         return TRUE;
//     }
// }

// /* set the up frame of IO module.
//  * Para:
//  *     iModAddr, address of IO module.
//  *     ucCtrl, control word of the up frame.
//  *     ucAttr, attribution word of the up frame.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Set_Up_Frm(int iModAddr, uint8_t ucCtrl, uint8_t ucAttr)
// {
//     uint8_t aucBuf[6];

//     assert(iModAddr<MAX_MOD_NUM);

//     aucBuf[0] = SPI_BYTES(2) | IO_UF_CTRL_REG;
//     aucBuf[1] = ucCtrl;
//     aucBuf[2] = ucAttr;
//     SPI_Write(iModAddr, aucBuf);
// }

// /* read the register of IO module.
//  * Para:
//  *     iModAddr, address of IO module.
//  *     ucCtrl, address of the register, 0~0x3F.
//  *     iNum, number of the register.
//  *     pucBuf, buffer address.
//  *     bAutoUp, 是否自动上送.
//  * Return:
//  *     EP_SUCCESS, EP_COM_ERR.
//  */
// static EP_STATUS SIO_Read_Reg(int iModAddr, int iReg, int iNum, uint8_t *pucBuf, BOOL bAutoUp)
// {
//     EP_STATUS sts;
//     uint8_t aucRcv[6];

//     assert(iModAddr < MAX_MOD_NUM);
//     assert(iReg < 0x40);
//     assert(iNum <= 4);

//     /* Take SPI semaphore. */
//     semTake(aspibuf_g[iModAddr].semMod, WAIT_FOREVER);

//     /* 是否主动上送 */
//     if (bAutoUp)
//     {
//         SIO_Set_Up_Frm(iModAddr, 0x01, SPI_BYTES(iNum) | iReg);
//     }
//     else
//     {
//         SIO_Set_Up_Frm(iModAddr, 0x00, SPI_BYTES(iNum) | iReg);
//     }

//     taskDelay(WAIT_SPI_RX);

//     sts=SPI_Read(iModAddr, aucRcv);

//     /* Give SPI semaphore. */
//     semGive(aspibuf_g[iModAddr].semMod);

//     if (sts==EP_SUCCESS
//             && ((aucRcv[0] == (SPI_BYTES(iNum) | iReg))
//                 || (aucRcv[0] == iReg)))		/* new protocol. */
//     {
//         /* Copy register data */
//         memcpy(pucBuf, aucRcv+1, iNum);

//         return EP_SUCCESS;
//     }
//     else return EP_COM_ERR;
// }

// /* DI processing, called in ISR.
//  * Para:
//  *     pspibuf, buffer of IO module data.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Recv_DI(SPI_IO_BUF *pspibuf)
// {
//     uint8_t *pucUp;
//     DI_MOD_PARM *pdimod;
//     DI_CHANNEL *pdich;
//     int i;
//     uint32_t ulChgBit;
//     uint32_t ulCurrVal;
//     uint32_t ulChkBit;

//     pucUp=pspibuf->aucUpFrame;
//     pdimod=&pspibuf->modinfo.dimod;

//     /* 判断是否是开入量数据 */
//     if((pucUp[0] != (SPI_BYTES(4) | DI_INPUT_REG)) && (pucUp[0] != (SPI_BYTES(4) | DO_SYS_EVENT)) && (pucUp[0] != (SPI_BYTES(4) | CKDOFEEDBAKC1)))
//         return;

//     if(pspibuf->ulSPIDIRcvTimes==SPI_DI_NO_FILTER_CNT)
//     {
//         /* 读取所有配置消抖时间 */
//         for (i = 0; i<MAX_DI_PER_MOD; i++)
//         {
//             pspibuf->chinfo.dich[i].ulFltTmp=pspibuf->chinfo.dich[i].ulFltCfg;
//         }
//         pspibuf->ulSPIDIRcvTimes++;
//     }
//     else if(pspibuf->ulSPIDIRcvTimes<SPI_DI_NO_FILTER_CNT)
//     {
//         /* 第一次读DI时无消抖 */
//         for (i = 0; i<MAX_DI_PER_MOD; i++)
//         {
//             pspibuf->chinfo.dich[i].ulFltTmp = 0;
//         }

//         pspibuf->ulSPIDIRcvTimes++;
//     }

//     ulCurrVal=U8_TO_U32(pucUp[4], pucUp[3], pucUp[2], pucUp[1]);
//     if(pspibuf->ucModType == DIO_MODULE)
//     {
//         /* 如果是开入开出板，仅仅第3个字节表示实际开入状态 */
//         ulCurrVal = (ulCurrVal & 0x0000FF00)>>8;
//     }

//     ulChgBit=ulCurrVal ^ pdimod->ulLastVal;
//     pdimod->ulLastVal=ulCurrVal;

//     ulCurrVal=pdimod->ulFltFg;
//     ulCurrVal |= ulChgBit;

//     if (ulCurrVal)                      /* 开入量当前点有变化或者在滤波 */
//     {
//         pdimod->ulFltFg=ulCurrVal;

//         ulChkBit=1;
//         for (i=0; ; i++)
//         {
//             if (ulCurrVal & ulChkBit)
//             {
//                 pdich=&pspibuf->chinfo.dich[i];
//                 if (ulChgBit & ulChkBit)
//                 {
//                     /* 重置变化通道计数器 */
//                     pdich->ulFltCnt=pdich->ulFltTmp;
//                     TM_High_Get_Sys_Us_UTC_Time(&pdich->utTmpTime,&pdich->ulTmpTime);
//                     pdich->ulTmpTime-=(ulSpiComDelay+ulPollDelay);		/* 减去消抖及通讯延时 */
//                     pdich->utTmpTime.ullusCntFrom1970-=(ulSpiComDelay+ulPollDelay);
//                 }

//                 if (!pdich->ulFltCnt)
//                 {
//                     pdimod->ulFltFg &= ~ulChkBit;
//                     if (((pdimod->ulLastVal & ulChkBit) && !pdich->bSts) ||
//                             (!(pdimod->ulLastVal & ulChkBit) && pdich->bSts))
//                     {
//                         /* 原来这里有BUG,进行了修改 */
//                         pdich->bSts=(pdimod->ulLastVal & ulChkBit) ? TRUE:FALSE;

//                         /* 保护装置的状态变位类信息的时标应为消抖后时标 */
//                         if (uiAppType_g == APP_PROT_MEA_MERGE)
//                         {
//                             pdich->ulChgTime=pdich->ulTmpTime;
//                             pdich->utChgTime=pdich->utTmpTime;
//                         }
//                         else
//                         {
//                             TM_High_Get_Sys_Us_UTC_Time(&pdich->utChgTime,&pdich->ulChgTime);
//                         }
//                     }
//                 }
//                 else
//                 {
//                     pdich->ulFltCnt--;
//                 }

//                 ulCurrVal &= ~ulChkBit;
//                 if (!ulCurrVal)
//                     break;
//             }

//             ulChkBit<<=1;
//         }
//     }

// }

// /* AI processing, called in ISR.
//  * Para:
//  *     pspibuf, buffer of IO module data.
//  * Return:
//  *     NONE.
//  */
// static void SIO_Recv_AI_DI(SPI_IO_BUF *pspibuf)
// {
//     static uint32_t ulCnt = 0;

// #ifdef SPIIOTEST
//     static uint8_t ucChnNum[MAX_MOD_NUM] = {0};
//     static uint8_t cycle[MAX_MOD_NUM] = {0};
// #endif

//     uint16_t usData = 50;
//     uint8_t *pucUp;
//     DI_MOD_PARM *pdimod;
//     DI_CHANNEL *pdich;
//     int i;
//     uint64_t ulChgBit;
//     static uint32_t ulLow32Val = 0;
//     uint64_t ulCurrVal;
//     uint64_t ulChkBit;


//     pucUp=pspibuf->aucUpFrame;
//     pdimod=&pspibuf->modinfo.dimod;

//     ulCnt++;

//     if (pucUp[0] == DI_INPUT_GROUP1_BASE_REG)
//     {
//         /* the first group DI. */
//         ulLow32Val=(uint32_t)U8_TO_U32(pucUp[4], pucUp[3], pucUp[2], pucUp[1]);

//         return;
//     }
//     else if (pucUp[0] == DI_INPUT_GROUP2_BASE_REG)
//     {
//         if(pspibuf->ulSPIDIRcvTimes==1)
//         {
//             /* 读取所有配置消抖时间 */
//             for (i = 0; i<MAX_DI_PER_MOD; i++)
//             {
//                 pspibuf->chinfo.dich[i].ulFltTmp=pspibuf->chinfo.dich[i].ulFltCfg;
//             }
//             pspibuf->ulSPIDIRcvTimes++;
//         }
//         else if(pspibuf->ulSPIDIRcvTimes==0)
//         {
//             /* 第一次读DI无消抖 */
//             for (i = 0; i<MAX_DI_PER_MOD; i++)
//             {
//                 pspibuf->chinfo.dich[i].ulFltTmp = 0;
//             }
//             pspibuf->ulSPIDIRcvTimes++;
//         }

//         ulCurrVal = U8_TO_U32(pucUp[4], pucUp[3], pucUp[2], pucUp[1]);
//         ulCurrVal=(ulCurrVal<<32)+ulLow32Val;

//         ulChgBit=ulCurrVal ^ pdimod->ulLastVal;
//         pdimod->ulLastVal=ulCurrVal;

//         ulCurrVal=pdimod->ulFltFg;
//         ulCurrVal |= ulChgBit;

//         if (ulCurrVal)                      /* 开入量当前点有变化或者在滤波 */
//         {
//             pdimod->ulFltFg=ulCurrVal;

//             ulChkBit=1;
//             for (i=0; ; i++)
//             {
//                 if (ulCurrVal & ulChkBit)
//                 {
//                     pdich=&pspibuf->chinfo.dich[i];
//                     if (ulChgBit & ulChkBit)
//                     {
//                         /* 重置变化通道计数器 */
//                         pdich->ulFltCnt=pdich->ulFltTmp;
//                         TM_High_Get_Sys_Us_UTC_Time(&pdich->utTmpTime,&pdich->ulTmpTime);
//                         pdich->ulTmpTime-=(ulSpiComDelay+ulPollDelay);		/* 减去消抖及通讯延时 */
//                         pdich->utTmpTime.ullusCntFrom1970-=(ulSpiComDelay+ulPollDelay);
//                     }

//                     if (!pdich->ulFltCnt)
//                     {
//                         pdimod->ulFltFg &= ~ulChkBit;
//                         if (((pdimod->ulLastVal & ulChkBit) && !pdich->bSts) ||
//                                 (!(pdimod->ulLastVal & ulChkBit) && pdich->bSts))
//                         {
//                             /* 原来这里有BUG,进行了修改 */
//                             pdich->bSts=(pdimod->ulLastVal & ulChkBit) ? TRUE:FALSE;

//                             /* 保护装置的状态变位类信息的时标应为消抖后时标 */
//                             if (uiAppType_g == APP_PROT_MEA_MERGE)
//                             {
//                                 pdich->ulChgTime=pdich->ulTmpTime;
//                                 pdich->utChgTime=pdich->utTmpTime;
//                             }
//                             else
//                             {
//                                 TM_High_Get_Sys_Us_UTC_Time(&pdich->utChgTime,&pdich->ulChgTime);
//                             }
//                         }
//                     }
//                     else
//                     {
//                         pdich->ulFltCnt--;

//                         if (pdich->ulFltCnt)
//                         {
//                             pdich->ulFltCnt--;
//                         }
//                     }
//                     ulCurrVal &= ~ulChkBit;
//                     if (!ulCurrVal)
//                         break;
//                 }

//                 ulChkBit<<=1;
//             }
//         }
//     }
//     else if (pucUp[0] == IO_CHN_RD_CTRL)
//     {
//         uint16_t rcvdata;
// #ifdef SPIIOTEST
//         cycle[pspibuf->ucModAddr]++;
//         usData += (pspibuf->ucModAddr+cycle[pspibuf->ucModAddr]);
//         usData = usData << 3;

//         pspibuf->aucUpFrame[1] = 0x02;
//         pspibuf->aucUpFrame[2] = ucChnNum[pspibuf->ucModAddr];
//         pspibuf->aucUpFrame[3] = (uint8_t)usData&0x00FF;
//         pspibuf->aucUpFrame[4] = (uint8_t)((usData&0xFF00) >> 8);

//         ucChnNum[pspibuf->ucModAddr]++;

//         if (ucChnNum[pspibuf->ucModAddr] >= 10)
//         {
//             ucChnNum[pspibuf->ucModAddr] = 0;
//         }
// #endif
//         rcvdata = U8_TO_U16(pspibuf->aucUpFrame[4], pspibuf->aucUpFrame[3]);

//         pspibuf->aAiBuf[pspibuf->aucUpFrame[2]-1] = ((int16_t)((rcvdata & 0x1FFF) << 3))/8;
//         pspibuf->chinfo.aich[pspibuf->aucUpFrame[2]-1].overflag = (rcvdata & RCV_OV_FLAG) ? TRUE : FALSE;
//         pspibuf->chinfo.aich[pspibuf->aucUpFrame[2]-1].errflag = (rcvdata & RCV_ERR_FLAG) ? TRUE : FALSE;

//         if (((ulCnt%10000 == 1) || (ulCnt%10000 == 2) || (ulCnt%10000 == 3)) && 0)
//         {
//             LOG_Dbg_Msg("通用模型接收, 接收通道%d, 接收值%d, %x, %x, %x, 满偏值%d\n",
//                         pspibuf->aucUpFrame[2],
//                         pspibuf->aAiBuf[pspibuf->aucUpFrame[2]-1],
//                         pspibuf->aucUpFrame[3],
//                         pspibuf->aucUpFrame[4], usData, pspibuf->aOvValBuf[pspibuf->aucUpFrame[2]-1]);
//         }
//     }

//     IO_SndPeriodCmd(pspibuf);		/* send cycle command. */
// }

// /* initialize the AI channel.
//  * Para:
//  *     iModAddr, address of module.
//  *     uiCh, channel number in this module, begin from 0.
//  *     pfRate, rate coefficient.
//  * Return:
//  *     pointer to this AI channel, NULL if error occur.
//  */
// void *SIO_Init_AI(int iModAddr, u_int uiCh, float *pfRate)
// {
//     AI_CHANNEL *paich;

//     LOG_Dbg_Msg("直流通道输入，地址%d, 通道号%d.\n", iModAddr, uiCh, 0, 0, 0, 0);

//     assert (iModAddr<MAX_MOD_NUM);
//     assert (uiCh<MAX_DI_PER_MOD);

//     if (uiCh >= aspibuf_g[iModAddr].unAiChNum)
//     {
//         /* AI number. */

//         /* SPI主从,允许设置
//          */
//         if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//                 || (ucCpuSpiRol_g == 2))
//         {
//             aspibuf_g[iModAddr].unAiChNum=uiCh+1;

//             if (aspibuf_g[iModAddr].ucModType == IDLE_MODULE)
//             {
//                 aspibuf_g[iModAddr].ucModType=COM_MODULE;

//                 /* Install receive process subrotine. */
//                 aspibuf_g[iModAddr].pfDealRecv=SIO_Recv_AI_DI;
//             }
//             else
//                 assert (aspibuf_g[iModAddr].ucModType == COM_MODULE);

//             if (!aspibuf_g[iModAddr].unVer)
//                 return NULL;
//         }
//         else if (aspibuf_g[iModAddr].ucModType == COM_MODULE)
//         {
//             aspibuf_g[iModAddr].unAiChNum=uiCh+1;

//             if (!aspibuf_g[iModAddr].unVer)
//                 return NULL;
//         }
//         else
//             return NULL;
//     }

//     paich=&aspibuf_g[iModAddr].chinfo.aich[uiCh];
//     paich->pPos = aspibuf_g[iModAddr].aAiBuf+uiCh;
//     *pfRate=aspibuf_g[iModAddr].aOvValBuf[uiCh]/4095.0;

//     return paich;
// }

// /* initialize the AO channel.
//  * Para:
//  *     iModAddr, address of module.
//  *     uiCh, channel number in this module, begin from 0.
//  * Return:
//  *     pointer to this AI channel, NULL if error occur.
//  */
// void *SIO_Init_AO(int iModAddr, u_int uiCh)
// {
//     LOG_Dbg_Msg("直流通道输出，地址%d, 通道号%d.\n", iModAddr, uiCh, 0, 0, 0, 0);

//     return NULL;
// }

// /* initialize the DO channel.
//  * Para:
//  *     iModAddr, address of module.
//  *     uiCh, channel number in this module, begin from 0.
//  * Return:
//  *     pointer to this DO channel, NULL if error occur.
//  */
// void *SIO_Init_DO(int iModAddr, u_int uiCh)
// {
//     DO_CHANNEL *pdoch;

//     assert(iModAddr<MAX_MOD_NUM);
//     assert(uiCh<MAX_DO_PER_MOD);

//     if (uiCh>=aspibuf_g[iModAddr].unDoChNum)
//     {
//         /* SPI主从,允许设置
//          */
//         if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//                 || (ucCpuSpiRol_g == 2))
//         {
//             aspibuf_g[iModAddr].unDoChNum=uiCh+1;

//             if (aspibuf_g[iModAddr].ucModType==IDLE_MODULE)
//             {
//                 aspibuf_g[iModAddr].ucModType=DO_MODULE;
//             }
//             else if(aspibuf_g[iModAddr].ucModType==DI_MODULE)
//             {
//                 /* 如果已为开入模件，则设为开入开出，不能区分DIO和CKDIO */
//                 aspibuf_g[iModAddr].ucModType=DIO_MODULE;
//             }
//             else
//                 assert((aspibuf_g[iModAddr].ucModType==DO_MODULE) || (aspibuf_g[iModAddr].ucModType==DIO_MODULE));

//             if (!aspibuf_g[iModAddr].unVer)
//                 return NULL;
//         }
//         else
//             return NULL;
//     }

//     pdoch=&aspibuf_g[iModAddr].chinfo.doch[DO_CLR_CH];
//     pdoch->iModAddr=iModAddr;
//     pdoch->iChIdx=DO_CLR_CH;
//     pdoch->ulPassWd=DO_PASSWORD;

//     pdoch=&aspibuf_g[iModAddr].chinfo.doch[uiCh];

//     pdoch->iModAddr=iModAddr;
//     pdoch->iChIdx=uiCh;
//     pdoch->ulPassWd=DO_PASSWORD;

//     return pdoch;
// }

// /* set the realtime DO output.
//  * Para:
//  *     pvDoCh, pointer to this DO channel.
//  *     bClose, state, TRUE=close; FALSE=open.
//  * Return:
//  *     NONE.
//  */
// void SIO_Set_DO(void *pvDoCh, BOOL bClose)
// {
//     DO_CHANNEL *pdoch;
//     SPI_IO_BUF *pspibuf;
//     uint8_t aucBuf[6];
//     uint32_t ulSts;
//     int iLockKey;

//     assert(pvDoCh);

//     pdoch=(DO_CHANNEL*)pvDoCh;

//     if ((pdoch->ulPassWd!=DO_PASSWORD) || (pdoch->iModAddr>=MAX_MOD_NUM))
//     {
//         logMsg("Error,detect  Invalid  Enter DO,  Reboot Machine!\n",
//                0, 0, 0, 0, 0, 0);
//         if (ENG_MODE == 0)
//         {
//             LOG_Write(LOG_KERNEL, "准备非法进行开出, 复位CPU!\n", NULL);
//         }
//         else if (ENG_MODE == 1)
//         {
//             LOG_Write(LOG_KERNEL, "ready to open DO illegally, reset CPU!\n", NULL);
//         }
//         EP_Set_Sts_Bit(REBOOT_DLY|SYS_LOCK_DO);

//         return;
//     }


//     pspibuf=aspibuf_g+pdoch->iModAddr;

//     if(!(pdoch->iChIdx<pspibuf->unDoChNum || pdoch->iChIdx==DO_CLR_CH))
//     {
//         logMsg("Error,detect  Invalid  Enter DO,  Reboot Machine!\n",
//                0, 0, 0, 0, 0, 0);
//         if (ENG_MODE == 0)
//         {
//             LOG_Write(LOG_KERNEL, "准备非法进行开出, 复位CPU!\n", NULL);
//         }
//         else if (ENG_MODE == 1)
//         {
//             LOG_Write(LOG_KERNEL, "ready to open DO illegally, reset CPU!\n", NULL);
//         }
//         EP_Set_Sts_Bit(REBOOT_DLY|SYS_LOCK_DO);

//         return;
//     }
//     iLockKey = intLock();   /* 闭锁中断 */
//     pspibuf->bSetDoFlag = TRUE;
//     pspibuf->ulSetDoCnt = CMD_EXCUTE_ACK_NUM;
//     intUnlock(iLockKey);

//     /* semTake(pspibuf->semMod, WAIT_FOREVER); */      /* 去掉,为了在逻辑图中用TASKLOCK来实现 */
//     aucBuf[0]=SPI_BYTES(4) | DO_OUTPUT_REG;

//     ulSts=pspibuf->modinfo.domod.ulSts;
//     if (bClose)
//         ulSts |= BV32(pdoch->iChIdx);
//     else
//         ulSts &= ~BV32(pdoch->iChIdx);
//     pspibuf->modinfo.domod.ulSts=ulSts;

//     aucBuf[1]=LL8(ulSts);
//     aucBuf[2]=LH8(ulSts);
//     aucBuf[3]=HL8(ulSts);
//     aucBuf[4]=HH8(ulSts);

//     /* test */
//     /* iTestValue=((aucBuf[1])&0x01); */
//     /* LOG_Dbg_Msg("Test DO Value is %d,Mod Addr  is %d ,ch is %d \n",iTestValue,pdoch->iModAddr,pdoch->iChIdx,0,0,0); */

//     SPI_Write(pdoch->iModAddr, aucBuf);
//     if (pdoch->iModAddr == DIADDRONCPU)
//     {
//         if (aspibuf_g[DIADDRONCPU].unDoChNum)
//         {
//             /* 配置此模件 */
//             SetMBDo((int *)&aspibuf_g[DIADDRONCPU].aucDownFrame[1]);    /* 设定母板上开出 */
//         }
//     }

//     /* semGive(pspibuf->semMod); */
// }

// /* set the realtime DO output directly.
//  * Para:
//  *     iModAddr, 模件地址.
//  *     ulChnNo, 通道号.
//  *     bClose, 状态.
//  * Return:
//  *     NONE.
//  */
// void SIO_Set_DO_Drc(int iModAddr, uint32_t ulChnNo, BOOL bClose)
// {
//     uint32_t ulSts;
//     uint8_t aucBuf[6];
//     int iLockKey;

//     ulSts = aspibuf_g[iModAddr].modinfo.domod.ulSts;
//     if (bClose)
//         ulSts |= BV32(ulChnNo);
//     else
//         ulSts &= ~BV32(ulChnNo);

//     aspibuf_g[iModAddr].modinfo.domod.ulSts = ulSts;

//     iLockKey = intLock();   /* 闭锁中断 */
//     aspibuf_g[iModAddr].bSetDoFlag = TRUE;
//     aspibuf_g[iModAddr].ulSetDoCnt = CMD_EXCUTE_ACK_NUM;
//     intUnlock(iLockKey);

//     /* semTake(pspibuf->semMod, WAIT_FOREVER); */      /* 去掉,为了在逻辑图中用TASKLOCK来实现 */
//     aucBuf[0]=SPI_BYTES(4) | DO_OUTPUT_REG;

//     aucBuf[1] = LL8(ulSts);
//     aucBuf[2] = LH8(ulSts);
//     aucBuf[3] = HL8(ulSts);
//     aucBuf[4] = HH8(ulSts);

//     LOG_Dbg_Msg("SIO_Set_DO_Drc 地址%d 状态%x\n", iModAddr, ulSts, 0, 0, 0, 0);

//     SPI_Write(iModAddr, aucBuf);
// }

// /* initialize the DI channel.
//  * Para:
//  *     iModAddr, address of module.
//  *     uiCh, channel number in this module, begin from 0.
//  *     ulFilt, filting time, unit is us.
//  *     bInvalidDftVal，default value type.
//  * Return:
//  *     pointer to this DI channel, NULL if error occur.
//  */
// void *SIO_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt, BOOL bInvalidDftVal)
// {
//     DI_CHANNEL *pdich;

//     assert(iModAddr<MAX_MOD_NUM);
//     assert(uiCh<MAX_DI_PER_MOD);

//     if (uiCh>=aspibuf_g[iModAddr].unDiChNum)
//     {
//         /* unDiChNum表示目前可用的Di数 */

//         /* SPI主从,允许设置
//          */
//         if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//                 || (ucCpuSpiRol_g == 2))
//         {
//             aspibuf_g[iModAddr].unDiChNum=uiCh+1;

//             if (aspibuf_g[iModAddr].ucModType==IDLE_MODULE)
//             {
//                 /* Install receive process subrotine. */
//                 aspibuf_g[iModAddr].pfDealRecv=SIO_Recv_DI;

//                 aspibuf_g[iModAddr].ucModType=DI_MODULE;
//             }
//             else if(aspibuf_g[iModAddr].ucModType==DO_MODULE)
//             {
//                 /* 如果已为开出模件，则配为开入开出模件，不能区分DIO和CKDIO */
//                 /* Install receive process subrotine. */
//                 aspibuf_g[iModAddr].pfDealRecv=SIO_Recv_DI;
//                 aspibuf_g[iModAddr].ucModType=DIO_MODULE;

//             }
//             else
//                 assert((aspibuf_g[iModAddr].ucModType==DI_MODULE) || (aspibuf_g[iModAddr].ucModType==DIO_MODULE));

//             if (!aspibuf_g[iModAddr].unVer)
//                 return NULL;
//         }
//         else if((aspibuf_g[iModAddr].ucModType == DO_MODULE) || (aspibuf_g[iModAddr].ucModType == DIO_MODULE)
//                 || (aspibuf_g[iModAddr].ucModType == CKDIO_MODULE))
//         {
//             /* 处理开出模件上的开入 */
//             aspibuf_g[iModAddr].unDiChNum=uiCh+1;		/* 总开入*/

//             if(aspibuf_g[iModAddr].ucModType == DO_MODULE)
//             {
//                 /* 最开始是开出模件，第一次处理 */
//                 /* Install receive process subrotine. */
//                 aspibuf_g[iModAddr].pfDealRecv=SIO_Recv_DI;

//                 aspibuf_g[iModAddr].ucModType=DIO_MODULE;
//             }

//             if (!aspibuf_g[iModAddr].unVer)
//                 return NULL;
//         }
//         else
//             return NULL;
//     }

//     pdich=&aspibuf_g[iModAddr].chinfo.dich[uiCh];

//     if(iModAddr == DIADDRONCPU)
//     {
//         /* 主板IO子模件 */
//         pdich->ulFltCfg=ulFilt/(1000000L/uiAiRate_g);
//         if(ulFilt%(1000000L/uiAiRate_g)>(1000000L/uiAiRate_g/2))
//         {
//             pdich->ulFltCfg++;
//         }
//     }
//     else
//     {
//         pdich->ulFltCfg=ulFilt/(1000000L/RD_SIO_RATE);
//         if(ulFilt%(1000000L/RD_SIO_RATE)>(1000000L/RD_SIO_RATE/2))
//         {
//             pdich->ulFltCfg++;
//         }
//     }

//     pdich->ulFiltTime=ulFilt;		/* 消抖时间，us */
//     pdich->ulFltTmp = pdich->ulFltCfg;
//     pdich->pIoBuf = (void *)&aspibuf_g[iModAddr];
//     pdich->ulInvalidDftVal = (uint32_t)bInvalidDftVal; /* 使用时转换为整数 */

//     return pdich;
// }

// /* read the realtime AI status.
//  * Para:
//  *     pvAiCh, pointer to this AI channel.
//  * Return:
//  *     result.
//  */
// int16_t SIO_Get_AI(void *pvAiCh)
// {
//     AI_CHANNEL *paich;
//     int iLockKey;
//     int16_t sampdata;

//     /* if (!EP_IS_BOOT_SEL()) */
//     {
//         if (!pvAiCh)
//         {
//             /* if NULL. */
//             return 0;
//         }
//     }

//     paich=(AI_CHANNEL *)pvAiCh;

//     iLockKey=intLock();

//     sampdata=*(paich->pPos);

//     intUnlock(iLockKey);

//     return sampdata;
// }

// /* read the realtime DI status.
//  * Para:
//  *     pvDoCh, pointer to this DI channel.
//  *     pulChgTime, the time chage to this status(us).
//  *     putChgtm, the time chage to this status(US_CNT_UTC_TIME).
//  * Return:
//  *     TRUE=close; FALSE=open.
//  */
// BOOL SIO_Get_DI(void *pvDiCh, uint32_t *pulChgTime, US_CNT_UTC_TIME *putChgtm)
// {
//     DI_CHANNEL *pdich;
//     BOOL bSts = FALSE;
//     int iLockKey;
//     SPI_IO_BUF *pTmpIoBuf = NULL;

//     /* if (!EP_IS_BOOT_SEL()) */	/* 没有DIO模件，方便EDP01和EDP02平台调试 */
//     {
//         /* assert(pvDiCh); */
//         if (!pvDiCh)
//         {
//             /* NULL */
//             return FALSE;
//         }
//     }

//     pdich=(DI_CHANNEL *)pvDiCh;

//     iLockKey=intLock();
//     pTmpIoBuf = (SPI_IO_BUF *)pdich->pIoBuf;

//     /* 缺省值处理 */
//     if (pTmpIoBuf->bDefaultFlag)
//     {
//         *pulChgTime = pTmpIoBuf->ulSwitchDefaultTm;

//         if (pdich->ulInvalidDftVal == 1)
//         {
//             bSts = TRUE;
//         }
//         else if (pdich->ulInvalidDftVal == 2)
//         {
//             bSts = pdich->bSts;
//         }
//         else if (pdich->ulInvalidDftVal == 3)
//         {
//             bSts = pdich->bSts;
//         }
//         else
//         {
//             bSts = FALSE;
//         }
//     }
//     else
//     {
//         *pulChgTime=pdich->ulChgTime;
//         *putChgtm=pdich->utChgTime;
//         bSts=pdich->bSts;
//     }

//     intUnlock(iLockKey);

//     return bSts;
// }

// /* get the status of IO module.
//  * Para:
//  *     iAddr, address of module.
//  *     pmodinfo, status infomation.
//  * Return:
//  *     NONE.
//  */
// void SIO_Mod_Info(int iAddr, SUB_MOD_INFO *pmodinfo)
// {
//     SPI_IO_BUF *pspibuf;

//     assert(iAddr < MAX_MOD_NUM);

//     pspibuf = &aspibuf_g[iAddr];

//     if(pspibuf->ucModType == IDLE_MODULE)
//         pmodinfo->stsMod = EP_COM_ERR;
//     else if (pspibuf->bComErr)
//         pmodinfo->stsMod = EP_COM_ERR;
//     else if (pspibuf->ucModSts != 0)
//         pmodinfo->stsMod = EP_IO_ERR;
//     else
//         pmodinfo->stsMod = EP_SUCCESS;

//     pmodinfo->type = pspibuf->ucModType;
//     pmodinfo->ucDesignSN = pspibuf->ucDesignSN;
//     pmodinfo->ucHardAddr = pspibuf->ucModAddr;
//     pmodinfo->unAiChNum = pspibuf->unAiChNum;
//     pmodinfo->unAoChNum = pspibuf->unAoChNum;
//     pmodinfo->unDiChNum = pspibuf->unDiChNum;
//     pmodinfo->unDoChNum = pspibuf->unDoChNum;
//     pmodinfo->unVer = pspibuf->unVer;
//     LOG_Dbg_Msg("IO VER IS %x.\n", pspibuf->unVer, 0, 0, 0, 0, 0);
// }

// /* enable the start-up DO.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_Enable_DO(void)
// {
//     int iLockKey;

//     iLockKey = intLock(); /* Prevent register R/W collision. */

//     uiEnDoFg_g |= EXT_EN_DO;

//     /* EDP01平台直接端口操作 */
//     if (bdType_g == BOARD_TYPE_E01)
//     {
//         IoPinOutputHigh(IO_OUT_QD, IO_PIN_HIGH);
//     }

//     // WT_MegaOpenQD();

//     intUnlock(iLockKey);
// }

// /* disable the start-up DO.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_Disable_DO(void)
// {
//     int iLockKey;

//     iLockKey = intLock();   /* Prevent register R/W collision. */

//     uiEnDoFg_g &= ~EXT_EN_DO;

//     if (!uiEnDoFg_g)
//     {
//         /* EDP01平台直接端口操作 */
//         if (bdType_g == BOARD_TYPE_E01)
//         {
//             IoPinOutputHigh(IO_OUT_QD, IO_PIN_LOW);
//         }
//         // WT_MegaCloseQD();
//     }

//     intUnlock(iLockKey);
// }

// /* alarm out.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_Enable_Alm(void)
// {
//     int iLockKey;

//     iLockKey = intLock();    /* Prevent register R/W collision. */

//     IoPinOutputHigh(IO_OUT_GJ, IO_PIN_HIGH);

//     intUnlock(iLockKey);
// }

// /* stop alarm out.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_Disable_Alm(void)
// {
//     int iLockKey;

//     iLockKey = intLock();   /* Prevent register R/W collision. */

//     IoPinOutputHigh(IO_OUT_GJ, IO_PIN_LOW);

//     intUnlock(iLockKey);
// }

// /* revert the self-keep DO(signal board).
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_Clr_DO_Keep(void)
// {
//     int i;
//     DO_CHANNEL *pdoch;
//     int iLockKey;
//     STATUS vxsts;

//     iLockKey = intLock();    /* Prevent register R/W collision. */
//     /* 不能开放启动信号,否则可能导致跳闸信号乱跳,然后复归,只能复归自保持信号 */
//     /*
//     uiEnDoFg_g |= INT_EN_DO;
//     (*PDATA(iIMMR_g)) &= ~PA2;
//     */
// #ifdef EDP_TEST	 /* 测试用 */
//     SIO_Enable_DO();  /* 启动信号开放，以便在复归时解除开出保持 */
// #endif

//     intUnlock(iLockKey);

//     /* 为了防止原来的DO设置还起作用，需要先将其清零 */
//     RD_Clear_All_Phy_DO();

//     taskDelay(spiinfo.ulSPIDelayCnt);             /* Wait for SPI communication. */

//     for (i=0; i<MAX_MOD_NUM; i++)
//     {
//         if ((aspibuf_g[i].ucModType == DO_MODULE) || (aspibuf_g[i].ucModType == DIO_MODULE)
//                 || (aspibuf_g[i].ucModType == CKDIO_MODULE))
//         {
//             pdoch=&aspibuf_g[i].chinfo.doch[DO_CLR_CH];

//             if (pdoch->ulPassWd==DO_PASSWORD)
//             {
//                 vxsts = taskLock();
//                 SIO_Set_DO(pdoch, TRUE);
//                 vxsts = taskUnlock();
//             }
//         }
//     }

//     taskDelay(spiinfo.ulSPIDelayCnt);             /* Wait for SPI communication. */

//     for (i=0; i<MAX_MOD_NUM; i++)
//     {
//         if ((aspibuf_g[i].ucModType == DO_MODULE) || (aspibuf_g[i].ucModType == DIO_MODULE)
//                 || (aspibuf_g[i].ucModType == CKDIO_MODULE))
//         {
//             pdoch=&aspibuf_g[i].chinfo.doch[DO_CLR_CH];

//             if (pdoch->ulPassWd==DO_PASSWORD)
//             {
//                 vxsts = taskLock();
//                 SIO_Set_DO(pdoch, FALSE);
//                 vxsts = taskUnlock();
//             }
//         }
//     }


//     iLockKey = intLock();    /* Prevent register R/W collision. */

//     /* 收回启动信号 */

//     /*
//     uiEnDoFg_g &= ~INT_EN_DO;
//     if (!uiEnDoFg_g)
//         (*PDATA(iIMMR_g)) |= PA2;
//     */

// #ifdef EDP_TEST		/* 测试用 */
//     SIO_Disable_DO();				/* 启动信号禁止 */
// #endif

//     intUnlock(iLockKey);

//     /* 告警复归 */
//     if (!bEnableAlarm_g)
//     {
//         /* 若没有被置上告警，则允许复归告警，否则不复归告警，防止告警继电器被复归之后，很快又被置上告警 */
//         SIO_Disable_Alm();
//     }

//     /* 设置所有保护任务接收到复归命令 */
//     RE_SetAllTaskFgSts();
// }

// /* processing the DO reset.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOReset(SPI_IO_BUF *pspibuf)
// {
//     unsigned char aucLogInfo[512];
//     int i;
//     for(i=0; i<MAXCMDNUM; i++)
//     {
//         pspibuf->ulIOConfirmCnt[i]=0;		/*有复位时计数器清零*/
//         if(i > 3 && i < 7)
//         {
//             pspibuf->ulCmdSndCnt[i] = CMD_EXCUTE_ACK_NUM;	/*仅针对IO_SET_DO_CMD、IO_SET_CKDIO_CMD、IO_SET_DI_CMD的情况*/
//         }
//     }

//     if (!pspibuf->bIORebootFlag)
//     {
//         pspibuf->ulRebootCnt++;		/* 重启计数 */
//         pspibuf->bIORebootFlag = TRUE;

//         if (pspibuf->ulRebootCnt<MAX_REBOOT_NUM)
//         {
//             sprintf(aucLogInfo, "%d#IO模件异常复位\n", pspibuf->ucModAddr+1);

//             LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
//         }

//         if (pspibuf->ucModType == DO_MODULE)
//         {
//             LOG_Dbg_Msg("开出模件%d重启.\n", pspibuf->ucModAddr, 0, 0, 0, 0, 0);
//             pspibuf->ulCmdSts |= IO_SET_DO_CMD;
//             pspibuf->ulCmdSndCnt[4] = CMD_EXCUTE_ACK_NUM;
//         }
//         else if (pspibuf->ucModType == DIO_MODULE)
//         {
//             pspibuf->ulCmdSts |= IO_SET_DO_CMD;
//             pspibuf->ulCmdSndCnt[4] = CMD_EXCUTE_ACK_NUM;
//         }
//         else if (pspibuf->ucModType == CKDIO_MODULE)
//         {
//             LOG_Dbg_Msg("测控开入开出模件%d重启.\n", pspibuf->ucModAddr, 0, 0, 0, 0, 0);
//             pspibuf->ulCmdSts |= IO_SET_CKDIO_CMD;
//             pspibuf->ulCmdSndCnt[5] = CMD_EXCUTE_ACK_NUM;
//         }
//         else if (pspibuf->ucModType == DI_MODULE)
//         {
//             LOG_Dbg_Msg("开入模件%d重启.\n", pspibuf->ucModAddr, 0, 0, 0, 0, 0);
//             pspibuf->ulCmdSts |= IO_SET_DI_CMD;
//             pspibuf->ulCmdSndCnt[6] = CMD_EXCUTE_ACK_NUM;
//         }
//         else if (pspibuf->ucModType == COM_MODULE)
//         {
//             LOG_Dbg_Msg("通用模件%d重启.\n", pspibuf->ucModAddr, 0, 0, 0, 0, 0);
//             pspibuf->bChgBaseReg = TRUE;
//         }
//     }
// }

// /* processing the IO error, called in ISR.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOExec(SPI_IO_BUF *pspibuf)
// {
//     char TempInfo[256];

//     if (pspibuf->ucModSts
//             & (DO_INVALID | DO_BREAK | DO_QD_INVALID))
//     {
//         /* 状态字置位，说明有IO板异常报告 */
//         pspibuf->ulTotalExcCnt++;  /* 总的异常次数 */

//         if (!pspibuf->bDIDOExecFlag)
//         {
//             /* 若头一回报DIDO故障 */
//             pspibuf->ulExcCnt++;	/* 异常计数 */

//             if (pspibuf->ulExcCnt >= spiinfo.ulMaxErrCnt)
//             {
//                 DealStatusWord(pspibuf);
//                 pspibuf->bDIDOExecFlag = TRUE;
//                 pspibuf->ulExcCnt = 0;
//             }
//         }
//     }
//     else
//     {
//         pspibuf->ulExcCnt = 0;
//     }

//     /* 下行帧校验出错记录 */
//     if (pspibuf->ucModSts & DO_SPI_COM_ERROR)
//     {
//         pspibuf->ulUpChkErr++;
//         if (pspibuf->ulUpChkErr == 1)
//         {
//             sprintf(TempInfo, "%d#开出模件下行帧接收异常或校验错误!!\n",
//                     pspibuf->ucModAddr+1);
//             LOG_Write(LOG_KERNEL, TempInfo, NULL);
//         }
//     }

//     if (pspibuf->ucModSts & DO_RESET)
//     {
//         if ((pspibuf->ucModType == DO_MODULE)
//                 || (pspibuf->ucModType == DIO_MODULE)
//                 || (pspibuf->ucModType == CKDIO_MODULE))
//         {
//             if (!pspibuf->bRebootFlag)
//             {
//                 pspibuf->bRebootFlag = TRUE;
//                 sprintf(TempInfo, "%d#开出模件异常复位!\n", pspibuf->ucModAddr+1);
//                 LOG_Write(LOG_RUN, TempInfo, NULL);
//             }
//         }
//         else if (pspibuf->ucModType == DI_MODULE)
//         {
//             DealDOReset(pspibuf);
//             if (!pspibuf->bRebootFlag)
//             {
//                 pspibuf->bRebootFlag = TRUE;
//                 sprintf(TempInfo, "%d#开入模件异常复位!\n", pspibuf->ucModAddr+1);
//                 LOG_Write(LOG_RUN, TempInfo, NULL);
//             }
//         }
//     }
// }

// /* parse the status word.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealStatusWord(SPI_IO_BUF *pspibuf)
// {
//     char TempInfo[256];

//     if ((pspibuf->ucModType == DO_MODULE)
//             || (pspibuf->ucModType == DIO_MODULE)
//             || (pspibuf->ucModType == CKDIO_MODULE))
//     {
//         if (pspibuf->ucModSts & (DO_INVALID | DO_BREAK))
//         {
//             pspibuf->ulCmdSts |= IO_GET_FAULT_CMD;
//             pspibuf->ulCmdSndCnt[0] = CMD_EXCUTE_ACK_NUM;
//         }

// #if 0
//         if (pspibuf->ucModSts&DO_SPI_COM_ERROR)
//         {
//             if (ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                            "模件地址:%d,错误码:%02d\n",
//                            pspibuf->ucModAddr+1, DO_PROBLOM);
//             }
//             else if (ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                            "Module address:%d, error code:%02d\n",
//                            pspibuf->ucModAddr+1, DO_PROBLOM);

//             }
//             sprintf(TempInfo,"%d#开出模件下行帧接收异常或校验错误!!\n",
//                     pspibuf->ucModAddr+1);
//             LOG_Write(LOG_KERNEL, TempInfo, NULL);
//         }
// #endif
//         if (pspibuf->ucModSts&DO_QD_INVALID)
//         {
//             if (ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                            "模件地址:%d,错误码:%02d\n",
//                            pspibuf->ucModAddr+1, DO_QIDONG_ERR);
//             }
//             else if (ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                            "Module address:%d, error code:%02d\n",
//                            pspibuf->ucModAddr+1, DO_QIDONG_ERR);
//             }
//             sprintf(TempInfo,"%d#开出模件启动信号检测异常!!\n",
//                     pspibuf->ucModAddr+1);
//             LOG_Write(LOG_KERNEL, TempInfo, NULL);
//         }
//     }
// }

// /* processing the IO status.
//  * Para:
//  *     IO buffer.
//  * Return:
//  *     NONE.
//  */
// static void DealDOSts(SPI_IO_BUF *pspibuf)
// {
//     if ((pspibuf->ucModType == DO_MODULE)
//             || (pspibuf->ucModType == DIO_MODULE)
//             || (pspibuf->ucModType == CKDIO_MODULE))
//     {
//         if ((pspibuf->ucModSts&DO_INVALID) && (!pspibuf->bInvalid))
//         {

//             if (ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK,
//                            "%d#模件开出失效(0x%x)\n",
//                            pspibuf->ucModAddr+1,
//                            U8_TO_U32(pspibuf->aucUpFrame[4], pspibuf->aucUpFrame[3], pspibuf->aucUpFrame[2], pspibuf->aucUpFrame[1]));
//             }
//             else if (ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK,
//                            "%d# module DO error(0x%x)\n",
//                            pspibuf->ucModAddr+1,
//                            U8_TO_U32(pspibuf->aucUpFrame[4], pspibuf->aucUpFrame[3], pspibuf->aucUpFrame[2], pspibuf->aucUpFrame[1]));
//             }

//             pspibuf->bInvalid = TRUE;
//         }

//         if ((pspibuf->ucModSts&DO_BREAK) && (!pspibuf->bBreakdown))
//         {
//             if (ENG_MODE == 0)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK,
//                            "%d#模件开出击穿(0x%x)\n",
//                            pspibuf->ucModAddr+1,
//                            U8_TO_U32(pspibuf->aucUpFrame[4], pspibuf->aucUpFrame[3], pspibuf->aucUpFrame[2], pspibuf->aucUpFrame[1]));
//             }
//             else if (ENG_MODE == 1)
//             {
//                 ER_Set_Err(EV_DIDO_ERR,
//                            ER_REPORT | ER_ALARM | ER_LOCK,
//                            "%d# module DO broken(0x%x)\n",
//                            pspibuf->ucModAddr+1,
//                            U8_TO_U32(pspibuf->aucUpFrame[4], pspibuf->aucUpFrame[3], pspibuf->aucUpFrame[2], pspibuf->aucUpFrame[1]));
//             }
//             pspibuf->bBreakdown = TRUE;
//         }
//     }
// }

// /* record the DO buffer.
//  * Para:
//  *     pucRecBuf, buffer address.
//  *     BufSize, size of buffer.
//  * Return:
//  *     EP_SUCCESS, or EP_ERROR.
//  */
// EP_STATUS SIO_Rec_Do_Buf(uint8_t *pucRecBuf, int BufSize)
// {
// #define MAX_VALID_DO_BOARD_NUM 10

//     int i;
//     uint8_t *pucTx;
//     int iLockKey;
//     uint8_t *pucBufAddr;

//     assert((MAX_VALID_DO_BOARD_NUM*4)<=BufSize);

//     pucBufAddr=(uint8_t*)(iIMMR_g+SPI_TX_BUF);

//     pucTx=pucBufAddr;
//     iLockKey=intLock();
//     for (i=0; i<MAX_VALID_DO_BOARD_NUM; i++)
//     {
//         *pucRecBuf++=*(pucTx+0x10);
//         *pucRecBuf++=*(pucTx+0x20);
//         *pucRecBuf++=*(pucTx+0x30);
//         *pucRecBuf++=*(pucTx+0x40);

//         pucTx++;
//     }
//     intUnlock(iLockKey);

//     return  EP_SUCCESS;
// }

// /* SPI send data actively, called by the fask scanning task.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SPI_NEW_COM(int nScanInterval)
// {
//     /* 为了提高DI的瞬时刷新频率，重新用定时器来驱动，这里改为空操作 */
//     static OPT_TIME_BASE ulLastComUsCnt_s= {0,0};
//     OPT_TIME_BASE ulCurUsCnt;
//     int iLockKey;
//     static BOOL bStopSPITimer_s=FALSE;
//     static BOOL bSndFlag = TRUE;

//     if (!bSndFlag)
//     {
//         return;
//     }

//     /* 若开始通过快速保护任务通信，则停止TIMER3中断，一开始是通过TIMER中断来驱动 */
//     if (!bStopSPITimer_s)
//     {
//         /* 若开始通过快速保护任务驱动SPI通信，则停止TIMER方式的通信 但此次不通信，防止和TIMER方式冲突 */
//         // if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//         //         || (ucCpuSpiRol_g == 2))
//         // {
//         //     bSndFlag = FALSE;
//         // }
//         // else
//         // {
//         //     intDisable(INUM_TIMER3);
//         // }

//         bStopSPITimer_s=TRUE;
//         iLockKey = intLock();    /* 需要进行中断保护 */
//         SIO_SetSPIInfo(uiAiRate_g/nScanInterval);
//         intUnlock(iLockKey);

//         return;
//     }
//     /*2013-5-20  ZY */
//     ulCurUsCnt=OptGetBaseTimerCnt();
//     if (OptGetUsIntvlByBase(&ulCurUsCnt,&ulLastComUsCnt_s)>(1000000L/RD_SIO_RATE))
//     {
//         /* 若本次和上次SPI通信时间>设置的允许通信时间间隔，则开始一次新的通信 */

//         iLockKey = intLock();    /* 需要进行中断保护 */

//         SPI_Snd();  /* 发送 */

//         intUnlock(iLockKey);

//         ulLastComUsCnt_s=ulCurUsCnt;
//     }

//     return;
// }

// /***********************************************************************
// * SPIComInISR - Send SPI data in ISR
// *
// * RETURNS: NONE
// *
// */
// void SPIComInISR (void)
// {
//     int iLockKey;
//     static BOOL bStopSPITimer_s=FALSE;

//     if(!bStopSPITimer_s)
//     {
//         /* If the ADC ISR begin to drive the SPI, then stop the TIMER; but this time stop to prevent the confliction */
//         // intDisable(INUM_TIMER3);
//         bStopSPITimer_s=TRUE;

//         return;
//     }

//     iLockKey=intLock();               /* Need to protect the data. */

//     // *PDATD(iIMMR_g) &= (~0x00001000);   /* Assert /SPISEL for slave CPU. */

//     // /* Initialize RxBD. */
//     // *(uint16_t*)(iIMMR_g+SPI_RBASE+BD_STATUS_OFFSET) = BD_RX_EMPTY_BIT |
//     //         BD_RX_WRAP_BIT | BD_RX_INTERRUPT_BIT;

//     // /* Initialize TxBD status. */
//     // *(uint16_t*)(iIMMR_g+SPI_TBASE+BD_STATUS_OFFSET) = BD_TX_READY_BIT |
//     //         BD_TX_WRAP_BIT | 0x0800;

//     // /* Start sending data. */
//     // *SPCOM(iIMMR_g) = 0x80;

//     intUnlock(iLockKey);
// }

// /***********************************************************************
// * UpdateAllDiFltCfg - 更新全部开入消抖计数，包括母板开入，全部在采样中断中处理
// *
// * RETURNS: 无
// *
// */
// void UpdateAllDiFltCfg(
//     int  nScanInterval
// )
// {
//     int i, j;
//     STATUS vxsts;

//     vxsts=taskLock();

//     for(i=0; i<MAX_MOD_NUM; i++)
//     {
//         /* MAX_MOD_NUM-1模件是快速开入开出 */
//         for(j=0; j<MAX_DI_PER_MOD; j++)
//         {
//             aspibuf_g[i].chinfo.dich[j].ulFltCfg=aspibuf_g[i].chinfo.dich[j].ulFiltTime/((1000000L)/(uiAiRate_g/nScanInterval));
//             if(aspibuf_g[i].chinfo.dich[j].ulFiltTime%((1000000L)/(uiAiRate_g/nScanInterval))>((1000000L)/(uiAiRate_g/nScanInterval)/2))
//             {
//                 aspibuf_g[i].chinfo.dich[j].ulFltCfg++;
//             }
//             aspibuf_g[i].chinfo.dich[j].ulFltTmp = aspibuf_g[i].chinfo.dich[j].ulFltCfg;
//         }
//     }

//     vxsts=taskUnlock();
// }

// /* update the DI filter parameter.
//  * Para:
//  *     iScanRate, scan rate.
//  * Return:
//  *     NONE.
//  */
// static void UpdateDiFltCfg(int iScanRate)
// {
//     int i, j;

//     for(i=0; i<MAX_MOD_NUM-1; i++)
//     {
//         /* MAX_MOD_NUM-1模件是快速开入开出 */
//         for(j=0; j<MAX_DI_PER_MOD; j++)
//         {
//             aspibuf_g[i].chinfo.dich[j].ulFltCfg=aspibuf_g[i].chinfo.dich[j].ulFiltTime/((1000000L)/iScanRate);
//             if(aspibuf_g[i].chinfo.dich[j].ulFiltTime%((1000000L)/iScanRate)>(((1000000L)/iScanRate)/2))
//             {
//                 aspibuf_g[i].chinfo.dich[j].ulFltCfg++;
//             }
//             aspibuf_g[i].chinfo.dich[j].ulFltTmp = aspibuf_g[i].chinfo.dich[j].ulFltCfg;
//         }
//     }
// }

// /* get the start-up signal on mother board.
//  * Para:
//  *     NONE.
//  * Return:
//  *     TRUE, start-up; FALSE, no start-up.
//  */
// BOOL SIO_Is_Open_QD()
// {
//     BOOL bQDIsOpen=FALSE;
//     int iQDOpenSts;

//     if (bdType_g == BOARD_TYPE_E02)
//     {
//         /* EDP02平台 */
//         /* 获得启动反馈状态，目前对EDP02起作用，EDP01，EDP03不知是否支持 */
//         // iQDOpenSts=IoPinInputHigh(IO_IN_QD_RET);
//         // if (iQDOpenSts==IO_PIN_HIGH)
//         // {
//         //     bQDIsOpen=TRUE;
//         // }
//         // else if(iQDOpenSts==IO_PIN_LOW)
//         // {
//         //     /* 拉低代表闭锁 */
//         //     bQDIsOpen=FALSE;
//         // }
//         // else if(iQDOpenSts == IO_FUN_NULL)
//         // {
//         //     bQDIsOpen=TRUE;		/* 如果没有该引脚，默认是已经启动 */
//         // }

//         return  bQDIsOpen;
//     }
//     else if (bdType_g == BOARD_TYPE_E01)
//     {
//         /* EDP01平台没有反馈 */
//         if(uiEnDoFg_g&EXT_EN_DO)
//         {
//             return TRUE;
//         }
//         else
//         {
//             return FALSE;
//         }
//     }

//     return bQDIsOpen;
// }

// /***********************************************************************
// * SetMBDo - 获取母板开出(电铁使用)
// *
// * RETURNS: EP_ERROR, EP_SUCCESS
// *
// */
// static EP_STATUS SetMBDo(
//     int *pBuf		/* 保存地址 */
// )
// {
//     int iReVal;
//     uint32_t iSetVal;
//     char *p;

//     p=(char *)pBuf;
//     iSetVal=U8_TO_U32(p[3], p[2], p[1], p[0]);
//     iSetVal=(~iSetVal)&0x00000001;		/* 目前只支持一个通道 */

//     // iReVal=IoPinOutputHigh(IO_OUT_CONNECTOR_DO, (int)iSetVal);
//     if(iReVal != (int)iSetVal)
//     {
//         return EP_ERROR;
//     }
//     else
//     {
//         return EP_SUCCESS;
//     }
// }

// /***********************************************************************
// * UpdateMbDiSts - 更新模板开入状态
// *
// * RETURNS: 无
// *
// */
// void UpdateMbDiSts(void)
// {
//     SPI_IO_BUF *pspibuf;
//     int *pBuf;

//     pspibuf=aspibuf_g+DIADDRONCPU;

//     if (pspibuf->iMbDiSrcType == -1)
//     {
//         return;
//     }

//     if (!RD_GetMbDIUsedFlag())
//     {
//         return;
//     }

//     pspibuf->aucUpFrame[0]=(SPI_BYTES(4) | DI_INPUT_REG);		/* 为了统一处理 */
//     pBuf = (int *)&pspibuf->aucUpFrame[1];

//     if(pspibuf->iMbDiSrcType == IO_IN_CONNECTOR_DI)
//     {
//         *pBuf=~((IoPinInputHigh(IO_IN_CONNECTOR_DI)&DIADDROMCPUMASK)<<24);
//     }
//     else if(pspibuf->iMbDiSrcType == IO_IN_PNL_DI)
//     {
//         int iTmpRst;

//         iTmpRst=IoPinInputHigh(IO_IN_PNL_DI)&DIADDROMCPUMASK;
//         *pBuf=(iTmpRst<<24)|(iTmpRst<<8);
//     }
//     else
//     {
//         assert(FALSE);
//     }

//     if (pspibuf->pfDealRecv)		/* Call receive process function. */
//         pspibuf->pfDealRecv(pspibuf);
// }

// /* 获取开入变位时间.
//  * Para:
//  *     pSrc, 句柄.
//  * Return:
//  *     变位时间，单位为us.
//  */
// uint32_t SIO_GetDiChgTime(void *pSrc)
// {
//     DI_CHANNEL *pdich;

//     pdich = (DI_CHANNEL *)pSrc;

//     return pdich->ulChgTime;
// }

// /* 获取开入变位UTC时间.
//  * Para:
//  *     pSrc, 句柄.
//  * Return:
//  *     uint64_t, 从1970记录的微妙计数.
//  */
// uint64_t SIO_GetDiChgUTCTime(void *pSrc)
// {
//     DI_CHANNEL *pdich;

//     pdich = (DI_CHANNEL *)pSrc;

//     if (uiAppType_g == APP_PROT_MEA_MERGE)
//     {
//         return pdich->utTmpTime.ullusCntFrom1970;
//     }
//     else
//     {
//         return pdich->utChgTime.ullusCntFrom1970;
//     }
// }

// /* 设定SPI通信相关参数.
//  * Para:
//  *     iScanRate, scan rate..
//  * Return:
//  *     NONE.
//  */
// static void SIO_SetSPIInfo(int iScanRate)
// {
//     uiDioRate_g = iScanRate;
//     UpdateDiFltCfg(uiDioRate_g);
//     spiinfo.ulSPIRxErrCheckFreq = 20*uiDioRate_g;
//     spiinfo.ulSPIRxErrAlmLevel = spiinfo.ulSPIRxErrCheckFreq*9/10;
//     spiinfo.ulSPIRxErrLogLevel = spiinfo.ulSPIRxErrCheckFreq/100;
//     spiinfo.ulSPIRxErrRetLevel = spiinfo.ulSPIRxErrCheckFreq/1000;
//     spiinfo.ulMaxErrCnt = 2*uiDioRate_g;
//     spiinfo.ulCommandDelayCnt = 2.8*uiDioRate_g;
//     spiinfo.ulSPIDelayCnt = ((3*1000/uiDioRate_g)/SYS_TICK+2);

//     spiinfo.ulOverThreshold = ((SPI_OVER_THRESHOLD_TIME*uiDioRate_g)/spiinfo.ulSPIRxErrCheckFreq);

//     /* 查询延时 */
//     ulPollDelay = 1000000/(iScanRate*2);
// }

// /* 设定在定时器定时中断函数中发送SPI数据帧（提高SOE分辨率）.
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void SIO_SetIsrSndSPIFlag(void)
// {
//     int iLockKey;
//     static BOOL bFstFlag = TRUE;

//     if (!bFstFlag)
//     {
//         return;
//     }
//     bFstFlag = FALSE;

//     iLockKey = intLock();    /* 需要进行中断保护 */
//     SIO_SetSPIInfo(RD_SIO_RATE);
//     bNormalSndFlag = FALSE;
//     if (((ucCpuSpiRol_g == 0) && isNumber_2_04CPU())
//             || (ucCpuSpiRol_g == 2))
//     {
//         /* 从CPU直接返回 */
//         intUnlock(iLockKey);
//         return;
//     }
//     // *TER3(iIMMR_g) = 0xFFFF;  /* clear interupt flag. */
//     // *TCN3(iIMMR_g) = 0;   /* set 0 to ensure the interval. */
//     // intEnable(INUM_TIMER3);
//     intUnlock(iLockKey);
// }
