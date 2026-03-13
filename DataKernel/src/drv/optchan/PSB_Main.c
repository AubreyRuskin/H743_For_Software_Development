/*  程序说明:
    Hdlc_Com_With_BU,首先将FPGA程序写入FPGA,接着初始化和FPGA互联的任务,最后挂接FPGA中断并使能该中断.
    FPGA_Data_Int,读出FPGA状态字节,得知哪个通道有数据,然后将数据读出,并是否信号量
    Set_Send_Back_Data,将收到的报文发送出去,目前只支持通道1的发送.
    FPGA和CPU交互的内存中的内容参见"发送端32bit_BUS.pdf"和"接收端32bit_BUS"
*/

/* PSB_Main.c - subroutine library for driving the FPGA */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 05nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for driving the FPGA.
INCLUDES:
    Hdlc_Com_With_BU, 首先将FPGA程序写入FPGA, 接着初始化和FPGA互联的任务, 最后挂接FPGA中断并使能该中断.
    FPGA_Data_Int, 读出FPGA状态字节, 得知哪个通道有数据, 然后将数据读出, 并是否信号量
    Set_Send_Back_Data, 将收到的报文发送出去, 目前只支持通道1的发送.
    FPGA和CPU交互的内存中的内容参见"发送端32bit_BUS.pdf"和"接收端32bit_BUS"
*/

/* defines */

#include <semLib.h>
#include <taskLib.h>
extern int Config_FPGA(char *fileName);


#define SEND_COMMAND_OFFSET (0x500 << 2)
#define RECV_STATUS_BYTE_COUNT   2
#define RECV_STATUS_LO_OFFSET SEND_COMMAND_OFFSET
#define RECV_STATUS_HI_OFFSET (RECV_STATUS_LO_OFFSET + 4)
#define RECV_STATUS_RECV_OK 0x3
#define RECV_STATUS_RECV_MASK 0x3

#define FPGA_RECV_BUFFER_BYTE_COUNT   512
#define FPGA_BLOCK_MEM_SIZE_BYTE_COUNT 128

#define HDLC_FPGA_BITSTREAM "/tffs/cpu_fpga_cpu.bin"

/* globals */

unsigned char fpgaRecvBuf[FPGA_RECV_BUFFER_BYTE_COUNT];
unsigned char fpgaSendBuf[FPGA_RECV_BUFFER_BYTE_COUNT];

unsigned int recvStatusOffset[RECV_STATUS_BYTE_COUNT] =
{
    RECV_STATUS_LO_OFFSET,
    RECV_STATUS_HI_OFFSET,
};

SEM_ID sem_send_back;

/* functions */

/* FPGA中断函数
 * 参数: NONE
 */
void FPGA_Data_Int(void)
{
    // int i;
    // int j;
    // int k;
    // unsigned int offset;
    // unsigned int *pBufInt;
    // unsigned char dataLength;
    // unsigned char readCount;

    // for(i=0; i<RECV_STATUS_BYTE_COUNT; i++)
    // {
    //     fpga_readVal = Get_FPGA_Mem_Val(recvStatusOffset[i]);
    //     if(fpga_readVal > 0)
    //     {
    //         for(j=0; j<16; j++)
    //         {
    //             if(((fpga_readVal>>j)&RECV_STATUS_RECV_MASK)
    //                     == RECV_STATUS_RECV_OK)
    //             {
    //                 offset = (i * RECV_STATUS_BYTE_COUNT + j)
    //                          * FPGA_BLOCK_MEM_SIZE_BYTE_COUNT;
    //                 pBufInt = (unsigned int*)fpgaRecvBuf;
    //                 *pBufInt = Get_FPGA_Mem_Val(offset);
    //                 dataLength = fpgaRecvBuf[1];
    //                 readCount = ( dataLength + 2 + 4 -1)/4 - 1;
    //                 pBufInt++;
    //                 for(k = 1; k<=readCount; k++)
    //                 {
    //                     *pBufInt = Get_FPGA_Mem_Val(offset+ 4*k);
    //                     pBufInt++;
    //                 }
    //             }
    //         }
    //     }
    // }
    // semGive(sem_send_back);
}

/* 通过FPGA发送数据
 * 参数: NONE
 */
void Set_Send_Back_Data(void)
{
    // unsigned char sendLen;
    // unsigned char writeLoopCount;
    // unsigned int *pBufInt;
    // int i;

    // while(1)
    // {
    //     semTake(sem_send_back, WAIT_FOREVER);
    //     fpgaSendBuf[0] = fpgaRecvBuf[1];
    //     sendLen = fpgaSendBuf[0];
    //     for(i=0; i<sendLen; i++)
    //     {
    //         fpgaSendBuf[i+1] = fpgaRecvBuf[i+2];
    //     }
    //     writeLoopCount = (sendLen + 4 - 1 + 1)/4;
    //     pBufInt = (unsigned int *)fpgaSendBuf;

    //     for(i=0; i< writeLoopCount; i++)
    //     {
    //         Set_FPGA_Mem_Val(i*4, *pBufInt);
    //         pBufInt++;
    //     }

    //     Set_FPGA_Mem_Val(SEND_COMMAND_OFFSET, 1);
    // }
}

/* 启动通过FPGA发送数据任务
 * 参数: NONE
 */
void Begin_Send_Back_Data(void)
{
    sem_send_back = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
    taskSpawn(
        "tSendBack",					/* Task Name */
        10,                			/* Task Priority */
        /* VX_FP_TASK, */0,								/* Task Option */
        0x1000,		                    /* Task Stack Size */
        (FUNCPTR)Set_Send_Back_Data,				/* Task Function */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0				/* Parameter */
    );
}

/* 初始化FPGA接收中断
 * 参数: NONE
 */
void Init_FPGA_Irq(void)
{
    // intConnect (INUM_TO_IVEC(INUM_IRQ6), FPGA_Data_Int, (int)NULL);

    // *M8260_SIMR_H(0xF0000000) |= 0x00000200;   /*IRQ6*/
}

/* 与子站通讯
 * 参数: NONE
 */
void Hdlc_Com_With_BU(void)
{
    // Config_FPGA(HDLC_FPGA_BITSTREAM);
    Begin_Send_Back_Data();
    Init_FPGA_Irq();
}