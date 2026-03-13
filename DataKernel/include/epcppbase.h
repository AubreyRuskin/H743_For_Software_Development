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
/*      edcppbase.h                                  EDP01-04-0.1               */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      eb - cpp base define for EDP01 .                     					*/
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains base define for cpp file of EDP01.					*/
/*                                                                              */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      Qiufan	Yin, SNAC                                                       */
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
/*      Qiufan Yin      2003.01.03      Created first version 0.1.              */
/*                                                                              */
/********************************************************************************/

#ifndef EPCPPBASE_H

#include "edpbase.h"
#include "logmsg.h"

#define HIGHEST_PRIORITY	10			//任务最高优先级
#define HIGHER_PRIORITY		50			//任务较高优先级
#define NORMAL_PRIORITY		100			//任务正常优先级
#define LOWER_PRIORITY		150			//任务较低优先级
#define LOWEST_PRIORITY		200			//任务最低优先级

#define NORMAL_STACK_SIZE	2048		//正常堆栈大小


//定义成员函数的入口函数
#define DEFINE_ENTRY_FUNCTION(class_name,member_name,entry_name)	\
	int entry_name(int arg1,int arg2,int arg3,int arg4,int arg5,	\
				   int arg6,int arg7,int arg8,int arg9,int arg10)	\
	{																\
		class_name *p=(class_name *)arg1;							\
		return p->member_name();										\
	}

//声明成员函数的入口函数
#define DECLAR_ENTRY_FUNCTION(entry_name)							\
	int entry_name(int arg1,int arg2,int arg3,int arg4,int arg5,	\
				   int arg6,int arg7,int arg8,int arg9,int arg10);


//回调函数指针变量定义
typedef int (*RECALL_FUNCTION_PTR)(int arg1,int arg2,int arg3,int arg4,int arg5);

//回调函数声明
#define RECALL_FUNCTION_DECLAR(fun) \
	int fun(int arg1,int arg2,int arg3,int arg4,int arg5);

#endif //EPCPPBASE_H

