#include "relay.h"
#include "publib.h"
#include "psllib.h"

/*应用软件版本号,16位无符号整数,高8位为整数,低8位为小数,用BCD码,每4位表示0~9的一位10进制数,
   最高4位表示整数的10位数,次高4位表示整数的个位数,次低4位表示小数的10^-1位,最低4位表示小数的10^-2位
   比如10.99,实际应是整数位=10,小数位99,表示为16进制:0x1099
  需要保护开发人员修改应用软件时更新,便于进行应用程序版本控制  */
#define   USR_SW_VER   0X0100    

void   EP_Set_04CPU_Init_End_Flag(BOOL   bInitEndFlag);
/* Init function of the example logic part. */

//extern EP_STATUS YK_Get_Init(EP_ELEMENT *pelm);

EP_EXT_ELEM_MAP aextmap[]=
{
    {"PUB_GetAmpt", PUB_GetAmpt},
    
    {"PSL_CALC",	PSL_CALC},
	{"PSL_PTDX",	PSL_PTDX},
	{"PSL_TOC",		PSL_TOC},
	{"PSL_NOCURRENT",		PSL_NOCURRENT},
	{"PSL_OVERLOAD",		PSL_OVERLOAD},
	{"PSL_NOC",		PSL_NOC},
	{"PSL_MXWY",	PSL_MXWY},
	{"PSL_MXYY",	PSL_MXYY},
	{"PSL_XLWY",	PSL_XLWY},
	{"PSL_XLYY",	PSL_XLYY},
	{"PSL_TQ",		PSL_TQ},
	{"PSL_IJS",		PSL_IJS},
	{"PSL_I0JS",	PSL_I0JS}
};

int EP_Ext_Elem_Num()
{
    return sizeof(aextmap)/sizeof(aextmap[0]);
}

void EP_Debug_Part(void)
{
volatile int i;

    i++;
}

uint16_t  GetUsrSwVer()
{
    assert(((USR_SW_VER)&(0x000f))<0x000a
            &&(((USR_SW_VER)&(0x00f0))>>4)<0x000a
            &&(((USR_SW_VER)&(0x0f00))>>8)<0x000a
            &&(((USR_SW_VER)&(0xf000))>>12)<0x000a);
            
    return  	USR_SW_VER;
}

/***********************************************************************
* EP_Restart_Lgc - 重新启动逻辑图初始化
*
* RETURNS: 无
*
*/
EP_STATUS EP_Restart_Lgc(void)
{
#if 0
    RE_Refresh_SuanfaTable_Info(aextmap,EP_Ext_Elem_Num(),EP_Debug_Part);

	if (Relay_Engine_Activate(EP_LGC_CFG_FILE)==EP_SUCCESS)
	{
	    logMsg("LogicGraph  Restart  OK.\n", 0, 0, 0, 0, 0, 0);	    
            EP_Set_04CPU_Init_End_Flag(TRUE);
	    return EP_SUCCESS;
    }
    else
    {
        logMsg("LogicGraph  Restart  ERROR.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }
#endif

    return EP_SUCCESS;
}

/* 刷新应用层相关信息，动态加载时使用，
   主要是更新应用的算法库
   参数，无
   返回：EP_SUCCESS  成功
         其他，FALSE */
EP_STATUS   EP_App_Refresh()
{

RE_Refresh_SuanfaTable_Info(aextmap,EP_Ext_Elem_Num(),EP_Debug_Part);	

return EP_SUCCESS;
}

/* Start the program. */
void StartMain(void)
{
	EP_App_Refresh();
	//externMain();
}