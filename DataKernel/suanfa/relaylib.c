#include "relay.h"
#include "publib.h"
#include "psllib.h"
#include "RE_RelayEngine.h"

/*Ӧ�������汾��,16λ�޷�������,��8λΪ����,��8λΪС��,��BCD��,ÿ4λ��ʾ0~9��һλ10������,
   ���4λ��ʾ������10λ��,�θ�4λ��ʾ�����ĸ�λ��,�ε�4λ��ʾС����10^-1λ,���4λ��ʾС����10^-2λ
   ����10.99,ʵ��Ӧ������λ=10,С��λ99,��ʾΪ16����:0x1099
  ��Ҫ����������Ա�޸�Ӧ������ʱ����,���ڽ���Ӧ�ó���汾����  */
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
* EP_Restart_Lgc - ���������߼�ͼ��ʼ��
*
* RETURNS: ��
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

/* ˢ��Ӧ�ò������Ϣ����̬����ʱʹ�ã�
   ��Ҫ�Ǹ���Ӧ�õ��㷨��
   ��������
   ���أ�EP_SUCCESS  �ɹ�
         ������FALSE */
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