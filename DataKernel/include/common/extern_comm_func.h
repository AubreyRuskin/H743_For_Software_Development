#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

/**************************log相关处理函数*****************************/
#include "vxworks_type.h"

/*切换定值区记日志ok*/
extern void ChangeSetAreaModifiesToLog(uint8_t preArea,uint8_t newArea, uint16_t usOpSrc);

/* 软压板操作记日志, 增加数据源标识 */
extern void SybChangeToLog(SC_LINK_ITEM *RybItem, BOOL bOldStas, BOOL bNewStats, uint16_t usOpSrc);

/*删除定值区记日志ok*/
extern void  DeleteSetAreaToLog(uint8_t AreaCode, uint16_t usOpSrc);

/*切换yb总选择记日志ok*/
extern void YBTotalToLog(uint16_t oldTotalYbStas,uint8_t newYbTotalStas, uint16_t usOpSrc);

/*保护功能投退记日志ok*/
extern void ProtStatsModifiesToLog(const SC_SUB_LGC_ITEM *ProItem,BOOL bOldProtStats,
                                   BOOL bNewProtStats, uint16_t usOpSrc);

/*网络设置记日志ok*/
extern void IPAdressModifiesToLog(EDP_NET_CFG_INFO OldIpStats,EDP_NET_CFG_INFO NewIpStats);


/*装置电能清零记日志ok*/
extern void PoClearAdjustToLog(int iChannel);

/*装置通道校准记日志ok*/
extern void ChannelAdjustToLog();

/*装置复归记日志ok*/
extern void DeviceResetToLog();


/*****************************事件相关******************************************/

/*获取事件配置的个数*/
extern int EdpCan_Get_Event_Cfg_Num();

/*获取事件配置的头指针*/
extern VI_EVT_CFG* EdpCan_Get_Event_Cfg_Point();

/*获取事件的相关数据长度*/
extern uint16_t EdpCan_Get_Event_Para_Num(const VI_RUN_INFO *prunInfo);

/*获取当前事件的类型*/
extern uint16_t EdpCan_Get_Event_Type(const VI_RUN_INFO *prunInfo);

/*获取当前事件的属性*/
extern uint16_t EdpCan_Get_Event_ParaAttrib(const VI_RUN_INFO *prunInfo,uint16_t serial);

/*获取事件的相关数据*/
extern VI_EVT_PARM* 	EdpCan_Get_Event_ParaData(const VI_RUN_INFO *prunInfo,uint16_t serial);

/*获取SOE事件的相关数据*/
extern VI_SOE_MSG* 	EdpCan_Get_Soe_Data(const VI_RUN_INFO *prunInfo);

/*获取面板灯的相关数据*/
extern VI_LED_MSG* 	EdpCan_Get_Led_Status(const VI_RUN_INFO *prunInfo);



/*获取连接的相关数据*/
extern VI_LINK_MSG* 	EdpCan_Get_Link_Status(const VI_RUN_INFO *prunInfo);

/*获取ERR的相关数据*/
extern VI_ERR_MSG* 	EdpCan_Get_Err_Status(const VI_RUN_INFO *prunInfo);

/*获取ERR_STAT的相关数据*/
extern VI_ERR_STAT_MSG* 	EdpCan_Get_Err_Stat_Status(const VI_RUN_INFO *prunInfo);













/*****************************开出相关******************************************/



/*遥控配置结构体的头指针*/
extern VI_MEA_DO_CFG* EdpCan_Get_Remote_Do_Cfg();

/*遥控数据结构体的头指针*/
extern VI_MEA_DO_DB* EdpCan_Get_Remote_Do_DB();

/*遥控点的个数*/
extern uint16_t EdpCan_Get_Remote_Do_Num();



/*获取物理开出量的总个数*/
extern uint16_t EdpCan_Get_Hw_Do_Num();

/*获取物理开出量的结构指针*/
extern RD_LGC_DO_CH* EdpCan_Get_Hw_Do_Stru();

/*
	用于遥控操作，遥控选择和遥控执行都选择该函数
	ptnum 										点号
	OptNum 										遥控类型
	ucCmdTypeucCmdType				有预发和执行两种类型
	OptPara										脉宽长度
	usTqPara									同期参数
*/
extern BOOL VI_New_MeaDo(	uint8_t PtNum,uint8_t OptNum,uint8_t ucCmdType,uint32_t OptPara,uint32_t usTqPara	);


/* 强制DO输出（/开出传动等）
 * 参数：   iIdx, 开出量索引，从0开始
 *          iSts, 预设置的状态: TRUE, FALSE or -1 解除强制.
 * 返回值： 无 */
extern void RD_Force_DO(int iIdx, int iSts);

/* Read hardware DO status.
 * Parameters:
 *      iIdx, index of DO(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DO. */
extern int RD_Mea_Hw_DO(int iIdx);

/***********************************************************************
* VI_Clear_Evt - Clear self-keep event signals.
*清除自保持信号
* 本次设计中nType始终赋值为0即可
* RETURNS: 无
*
*/
extern void VI_Clear_Evt(int nType		);













/*****************************定值相关******************************************/


/*该函数实际上是特制的，就是只有一个0号定值区，其余都报错*/
extern uint8_t EdpCan_If_Set_Area_Efficient(int iArea);

/*获取运行定值区定值页的结构指针*/
extern SC_SET_PAGE* EdpCan_Get_Run_Set_Page_Stru();

/*获取定值页的总个数*/
extern uint16_t EdpCan_Get_Set_Page_Num();

/*获取当前运行区具体的定值的结构体
  page  具体的定值页
  serial 具体定值页的具体定值的偏移*/
extern SC_SET_ITEM* EdpCan_Get_Run_Set_Point(uint16_t page,uint16_t serial);

/*获取定值区中具体某页中定值的具体个数*/
extern uint16_t EdpCan_Get_Set_Num(uint16_t page);

/*将通信buf中的数据刷入到文件中*/
extern EP_STATUS EdpCan_WriteSettings_From_Buf(uint8_t zoneCode,uint8_t* commbuf);




/* Check if setting area file is valid.
 * Parameters:
 *      iFd, file descriptor opened previously.
 * Return value:
 *      TRUE, the setting area file is valid.
 *      FALSE, the setting area file is NOT valid.
 * Alert:
 *      Current position of the file is changed in this function. */
extern BOOL SC_Is_Valid_Set(int iFd);






/***********************************************************************
* SC_Del_Set_Area - 删除定值区
*
*		iArea:     定值区号
* RETURNS:
*       		   EP_SUCCESS, delete OK.
*               EP_PARM_ERR, iArea is the working area, can't delete.
*               EP_FILE_ERR, the area file not exists.
*
*/
extern EP_STATUS SC_Del_Set_Area(int iArea	);

/*切换运行定值区*/
extern EP_STATUS SC_Chg_Work_Area(int iArea);

/*获取当前运行定值区*/
extern int SC_Work_Set_Area(void);




/*获取当前实际运行定值区*/
extern int SC_Real_Work_Set_Area(void);


/***********************************************************************
* SC_Get_Valid_Area - Get every valid setting area number.
*	pucRslt			to save setting area number result.
* RETURNS: Number of total valid setting area.
*
*/
extern int SC_Get_Valid_Area(	uint8_t *pucRslt);







/************************网络相关*********************************************************/

/*
typedef struct
{
  	int iNetSeqNo;
  	uint8_t aucIpAddr[4];
   	uint8_t aucIpMsk[4];
   	uint8_t aucMacAddr[6];

}ONE_NET_CFG_INFO;

typedef struct
{
  	int iValidNetNum;
  	ONE_NET_CFG_INFO NetInfArr[MAX_EDP_NET_NUM];
}EDP_NET_CFG_INFO;
*/
/***********************************************************************
* NT_GetNetRunCfg - 获得网络实际运行配置
*
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
extern EP_STATUS NT_GetNetRunCfg(		EDP_NET_CFG_INFO *pRtNetInfo	);


/***********************************************************************
* NT_SetOneNetIpAddr - 设置某网口的IP地址，须重启后才起作用
*
*	iNetSeqNo,					网络号，从0开始
*	pIpAddrBase					IP地址串基址
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
extern EP_STATUS NT_SetOneNetIpAddr(		int iNetSeqNo,	uint8_t *pIpAddrBase);


/*功能,设置某网口的MAC地址，目前由IP地址自动生成，不额外配置。
  参数,  iNetSeqNo,网络号,从0开始
         pMacAddrBase,Mac地址串基址*/
extern EP_STATUS   NT_SetOneNetMacAddr(int  iNetSeqNo,uint8_t *pMacAddrBase);


/***********************************************************************
/* 设置某网口的IP子网掩码
 * 参数:
 * iNetSeqNo, 网络号, 从0开始
 * pIpMskBase, 子网掩码串基址
 */
extern EP_STATUS NT_SetOneNetIpMsk(int iNetSeqNo, uint8_t *pIpMskBase);













/*************************时间相关**********************************************/



/***********************************************************************
* Get system time(from GPS or a master station).
 * Parameters:
 *      pdttmNow, structure to save the date/time.
 * Return value:
 *      EP_SUCCESS, successful get the date/time.
 *      EP_LOCAL_MSG, system date/time not checked for long time.
 *      EP_NOT_INIT, system time was never checked from CPU reset.
 *      EP_HARD_ERR, hardware error(such as the crystal not working.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
extern EP_STATUS TM_Get_Sys_Time(EP_DATE_TIME *pdttmNow);


/* Set the system time.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 *      EP_SYS_ERR, other unexpected system error.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
extern EP_STATUS TM_Set_Sys_Time(const EP_DATE_TIME *pdttmSet, BOOL bPecision);

extern STATUS set_date(int year, int month, int day, int hour, int minute, int second);

/***********************************************************************
* GetAdjustTimeSuccessFlag -  MMI_SOFT.C提供的对时成功标志,
*
* RETURNS:
*				   TRUE, 初始化后对时成功
*					FALSE, 初始化后对时还没有成功
*
*/
extern BOOL GetAdjustTimeSuccessFlag();









/************************压板相关***********************************************/







/*获取当前的压板序列号*/
extern SC_LINK_ITEM* SC_Get_Sw_Link(int iIdx);

/*获取当前的压板状态*/
extern uint8_t EdpCan_Get_Sw_Status(int iIdx);


/*修改压板状态
 	iIdx    压板序号
 	bSts		压板状态
*/
extern EP_STATUS SC_Chg_Sw_Link(int iIdx, BOOL bSts);


/*
	获取总的压板模式
	ulTotalLinkMode 					总模式
*/
extern EP_STATUS SC_Get_Link_Mode_Sts(		uint16_t *ulTotalLinkMode		);



/*
Description: change the content of the /set/set/edplinkmode.set
iIdx: represent the certain link idx, only valid when bTotalFlag==false
ulMode: the link's mode to be set.
bTotalFlag:  if false, ucMode represent certain link's mode
             if true, ucMode represent total link's mode
*/
extern EP_STATUS SC_Chg_Link_Mode_File(int iIdx, uint8_t ucMode, BOOL bTotalFlag);


/* 获取压板状态.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
extern EP_STATUS SC_Get_Link_Now_Sts(int iIdx,		BOOL *pbRslt);



/* DQ:
 * Get hard link status.获取硬压板状态
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g)
 *      EP_PARM_ERR, link is not relevent. */
extern EP_STATUS SC_Get_Link_HW_Sts(	int iIdx,	BOOL *pbRslt);

/* Get link attribution.
 * Parameters:
 *      iIdx, index of the link(from 0).
 * Return value:
 *      Pointer to the link attribution structure.
 *      NULL if iIdx is invalid(>=iLinkNum_g). */
extern const SC_LINK_ITEM *SC_Get_Link_Attr(int iIdx);

/*获取压板的总个数*/
extern uint16_t EdpCan_Get_Link_Num();







/*******************测量相关****************************************************/

/*请注意：这里描述的遥测和保护电量就是真正含义*/

/*获取保护电量的配置指针*/
extern VI_MEA_AI_CFG* EdpCan_Get_Protect_AI_Cfg_Stru();

/*获取保护电量的数值结构指针*/
extern VI_MEA_AI_DB* EdpCan_Get_Protect_AI_DB_Stru();



/*获取遥测量的配置指针*/
extern ME_MEA_VALUE_CFG* EdpCan_Get_Mea_AI_Cfg_Stru();


/*获取遥测量的数值结构指针*/
extern ME_MEA_AI_DB* EdpCan_Get_Mea_AI_DB_Stru();



/*获取遥测越限的遥测量的个数*/
extern uint16_t  EdpCan_Get_MeaOver_Data_Num(const VI_RUN_INFO *prunInfo);


/*根据参数制定的len长度提供相应个数的结构体数组的头指针*/
extern uint16_t  EdpCan_Get_MeaOver_Data(const VI_RUN_INFO *prunInfo,uint16_t pos, uint16_t len,ME_MEA_AI_DATA_DB *ppmeadb);



/*获取全部遥测量个数*/
extern int ME_Get_Msu_Num(void);

/***********************************************************************
* RD_Mea_AI - Read all measurment value
* 读取所有遥测量的数据的结构体（简化带一定描述的数值）
  调用者提供空间
* RETURNS: None
*
* Alert:
*        pmeaRslt must contains space to save iMeaValueNum_g members.
*/
extern void RD_Mea_AI(	RD_AI_MEA *pmeaRslt) ;

/*获取保护电量的总个数*/
extern uint16_t EdpCan_Get_Yaoce_Num();


/*获取遥测量的总个数*/

extern int ME_Get_Msu_Num(void);
/*
uint16_t EdpCan_Get_Celiang_Num()
{
	return iMeaValueNum;
}*/


/* Read all measurement AIs' value.
   获取遥测量的所有数据，调用者提供空间（只提供值）
 * Parameter:
 *      pfRslt, to save all Measure_AIs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pfRslt must contains space to save iMeaValueNum_g float numbers. */
extern void ME_Rd_Mea_AI_Val(float *pfRslt);

/***********************************************************************
* RD_Mea_Hw_AI - Read all hardware AI measurment value
*	 to save all hardware AI measurement value 读取所有物理通道的值
* RETURNS: None
* 需要注意的是这个函数不是仅仅读取数值，还包含了相当数量的计算
* Alert:
*        phwmeaRslt must contains space to save iHwAiChNum_g members.
*/
extern void RD_Mea_Hw_AI(RD_HW_AI_MEA *phwmeaRslt, BOOL bIsCalc);








/***************************电度相关********************************************/



/***********************************************************************
* GetPoCfgNum - 获取PO配置数
*
* RETURNS: PO配置数
*
*/
extern int GetPoCfgNum(void);


/***********************************************************************
* VI_New_PoClear - PO清零，供mmi调用
*
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_PoClear(uint8_t ucObjNum);

/*装置电能清零记日志ok*/
extern void PoClearAdjustToLog(int iChannel);

/***********************************************************************
* RD_Mea_Po - Read all pulse output measurment value
*ppomeaRslt 结构体数组，保存所有的电度数据（调用者提供内存资源)
* RETURNS: None
*
* Alert:
*        ppomeaRslt must contains space to save iLgcPoChNum_g members.
*/
extern void RD_Mea_Po(	RD_PO_MEA *ppomeaRslt	) ;



/*获取电度量的结构指针*/
extern RD_LGC_PO_CH* EdpCan_Get_Po_Cfg_Stru();





/*******************系统状态相关**************************************************/




/************************************************************************
  功能：获得装置设备名称，2008-7-25   张云
  参数：ppucRtDevNameAddr: 返回装置设备名称字符串基址，
            调用方，将uint8_t  *类型的变量的地址传过来，
            供返回平台内部维护的设备名称全局字符串地址,
            注意，不发生字符串拷贝操作。
        piRtNameLen，返回设备名称字符串长度(注意，不包括"\0")。

*/
extern EP_STATUS  EP_GetDevName(uint8_t  **ppucRtDevNameAddr,int   *piRtNameLen);




/***********************************************************************
* VI_New_Adjust - 校准命令，供mmi调用

	ucObjType, 			 校准对象类型，0: ai物理通道，1: 测量量
*	ucOrdType			   校准命令类型， 0: 增益校准，1: 偏置校准
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_Adjust(	uint8_t ucObjType, 	uint8_t ucOrdType	);


/***********************************************************************
* EP_Clr_Sts_Bit - Clear system status(AND operation)
*
* RETURNS: 无
*
* Alert:
*        This function can be called in ISR.
*
*/
extern void EP_Clr_Sts_Bit(		u_int uiSts	);


/*设置一些系统状态*/
extern void EP_Set_Sts_Bit(	u_int uiSts		);

/***********************************************************************
* VI_New_RepairSts - 检修状态，供MMI调用，根据是否为检修状态完成一定的操作
*
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_RepairSts(	uint32_t ulRepairSts	);

/*获取解挂锁状态，1为处于解挂锁状态
uint8_t EdpCan_Get_JGS_Status()
{
	if(uiEdpStatus_g&JGS_STATE){
		return 1;
	}
	else
	{
		return 0;
	}
}
*/
/* Change protect function run/exit status.
 * Parameters:
 *      strName, protect(sub-logic) name.
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change run/exit status OK.
 *      EP_FILE_ERR, file operating failure. */

extern EP_STATUS SC_Chg_Prtc_Sts(const uint8_t *strName, BOOL bSts);


/*
				获取具体子逻辑图的名称和该子逻辑任务是否运行
				 Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
extern const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx);


/***********************************************************************
* GetRecWrSts - 获取录波状态
*
* RETURNS:
*                TRUE，正在录波
*                FALSE，录波结束
*
*/
extern BOOL GetRecWrSts(void);

/***********************************************************************
* VI_Is_Fault - 返回是否处于故障态，故障态认为是一个特殊状态
*
* RETURNS:
*               TRUE: 是故障态
*               FALSE: 非故障态
*
*/
extern BOOL VI_Is_Fault(void);

/***********************************************************************
* EP_Get_Repair_Sts - Inquiring about if the system is in examination state
*
* RETURNS: None
*
*
*/
extern unsigned char EP_Get_Repair_Sts(	unsigned char *pbRtRepairSts);

/***********************************************************************
* ER_IsSetAlertFlag - 获得是否设置呼唤标志
*
* RETURNS: TRUE: 已经设置呼唤
*                 FALSE: 未设置呼唤
*
*/
extern BOOL ER_IsSetAlertFlag();

/*获取具体的装置的单个状态*/
extern uint8_t EdpCan_Get_Ied_Single_Status(u_int mode);

/***********************************************************************
* EP_Bgn_Hw_Test - Enter hardware test mode.  Logic function will be disabled
*
* RETURNS: 无
*
*/
extern void EP_Bgn_Hw_Test(void);


/***********************************************************************
* EP_End_Hw_Test - Exit hardware test mode.  System will reboot after several seconds
*
* RETURNS: 无
*
*/
extern void EP_End_Hw_Test(void);















/***************************DI部分处理****************************************/



/***********************************************************************
* VI_Rd_Mea_DI_Val - Read all measurement DIs' value.
*	pbRslt, to save all MEA_DIs' current value.这是一个布尔数组
*   pQuality, 品质因素,16位数组.
* RETURNS: 无
*
* alert:
* 		pbRslt must contains space to save iMeaDiNum_g BOOL numbers.
*/
extern void VI_Rd_Mea_DI_Val(BOOL *pbRslt, uint16_t *pQuality);


/*获取遥信量的总个数*/
extern uint16_t EdpCan_Get_Mea_Di_Num();


extern VI_MEA_DI_CFG* EdpCan_Get_Mea_Di_Stru();




/* Read hardware DI measurement value.
 * Parameters:
 *      iIdx, index of DI(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DI. */
extern int RD_Mea_Hw_DI(int iIdx);


/* Change force DI status.
 * Parameters:
 *      iIdx, index of DI(from 0).
 *      iSts, new status: TRUE, FALSE or -1 means release force.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
extern EP_STATUS RD_Chg_Force_DI(int iIdx, int iSts);




/**************某种保护类别是否投入运行***************************/


/* Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
extern const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx);


/********************************end*********************************************/


















