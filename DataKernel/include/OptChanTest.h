/************************************************************************/
/*                                                                      */
/*      Copyright (c) 2006 SNAC(Guodian Nanjing Automation Co., Ltd.)   */
/*      All Rights Reserved.                                            */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* FILE NAME                                            VERSION         */
/*                                                                      */
/*      OptChanTest.h                             EDPx-04-0.1           */
/*                                                                      */
/* COMPONENT                                                            */
/*                                                                      */
/*      Optical Channel Simulating tester                               */
/*                                                                      */
/* DESCRIPTION                                                          */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* AUTHOR                                                               */
/*                                                                      */
/*      Chen, Xinzhi, SNAC                                              */
/*                                                                      */
/* DATA STRUCTURES                                                      */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* FUNCTIONS                                                            */
/*                                                                      */
/*                                                                      */
/* DEPENDENCIES                                                         */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* HISTORY                                                              */
/*                                                                      */
/*         NAME            DATE                    REMARKS              */
/*                                                                      */
/*      Chen, Xinzhi      2006/11/14                1.00                */
/*                                                                      */
/************************************************************************/

#ifndef OPT_CHAN_TEST_H
#define OPT_CHAN_TEST_H

/*  功能:   设置正常的延时
    参数:   delayTime;  以微秒计算的延时的时间,从0到25000
    返回值: OK, ERROR
*/
int    Opt_Test_SetNormalDelay(unsigned int delayTime);

/*  功能:   启动测试序列
*/
void Opt_Test_Sequence_Start();

/*  功能:   停止测试序列
*/
void Opt_Test_Sequence_Stop();

/*  功能:   设置要运行的序列的文件名
    参数:   fileName;   序列所在的文件名,要包含全路径
*/
int Opt_Test_Set_Seq_File_Name(char *fileName);

/*  功能:   获得要运行的序列的文件名
    返回值: 指向函数名的指针
*/
char *Opt_Test_Get_Seq_File_Name();

void    Opt_Test_Begin();

#endif  /*OPT_CHAN_TEST_H*/
