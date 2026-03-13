/********************************************************************************/
/*                                                                              																	*/
/*      Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.)           								*/
/*      All Rights Reserved.                                                    															*/
/*                                                                              																	*/
/********************************************************************************/

/********************************************************************************/
/*                                                                              																	*/
/* FILE NAME                                            VERSION                 														*/
/*                                                                              																	*/
/*      usbMmiInterface.h                          EDP03-1.00               														*/
/*                                                                              																	*/
/* COMPONENT                                                                    															*/
/*                                                                              																	*/
/*                                                                              																	*/
/*                                                                              																	*/
/* DESCRIPTION                                                                  														*/
/*                                                                              																	*/
/*      This file provides the interface between USB and MMI.                         			 				 		*/
/*                                                                              																	*/
/* AUTHOR                                                                       															*/
/*                                                                              																	*/
/*      Yi Ding, SNAC                                                           															*/
/*                                                                              																	*/
/* DATA STRUCTURES                                                              													*/
/*                                                                              																	*/
/*                                                                              																	*/
/* FUNCTIONS                                                                    															*/
/*                                                                              																	*/
/*      None                                                                    																*/
/*                                                                              																	*/
/* DEPENDENCIES                                                                 														*/
/*                                                                              																	*/
/*      None                                                                    																*/
/*                                                                              																	*/
/* HISTORY                                                                      															*/
/*                                                                              																	*/
/*         NAME            DATE                    REMARKS                      													*/
/*                                                                              																	*/
/*      	Yi Ding          2006.05.19        Created first version 1.00.              										*/
/*                                                                              																	*/
/********************************************************************************/
#ifndef USBMMIINTERFACE_H
#define USBMMIINTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

/* #include "ossLib.h" */

/* defines */

#define TSK_PRI_USB_INIT 128
#define TSK_PRI_USB_INT 29
#define TSK_PRI_BULK 132

/* #define TSK_PRI_USBD OSS_PRIORITY_TYPICAL */

#define USBTEST 1
/* #define USB_LOG */		/* 是否打开LOG */

#ifdef USB_LOG
extern char USB_Log;
#define USB_DBG_LOG(f,a1,a2,a3,a4,a5,a6) if (USB_Log) logMsg(f,a1,a2,a3,a4,a5,a6)
#else
#define USB_DBG_LOG(f,a1,a2,a3,a4,a5,a6)
#endif /* USB_LOG */

#include "msgQLib.h"

typedef struct tag_MOUSEINFO		/* 鼠标设备信息 */
{
    long mouseX;
    long mouseY;
    UINT8 button;
} MOUSEINFO;

typedef struct BULKINFO_tag		/* 存储设备信息 */
{
    int MaxNum;		/* 总的存储设备数目 */
    char usbBulkDrvName[4][20];		/* 名称 */
    int usbBulkDrvFree[4];		/* 状态，连接为1；拔出为0 */
} BULKINFO;

extern MOUSEINFO mouse;				/* 鼠标消息结构*/
extern MSG_Q_ID touchMouseMsgId;		/* 鼠标消息队列号 */


/*************************************************************************
*
* UsbMmiInit - USB初始化
*
*/
void UsbMmiInit(void);

/*****************************************************************************
*
* usrUsbMseInit - initialize the USB mouse driver
*
* This function initializes the USB mouse driver and registers for attach
* callbacks.
*
* RETURNS: Nothing
*/

void usrUsbMseInit (void);

/*************************************************************************
*
* usrUsbBulkDevInit - initializes USB BULK Mass storage driver.
*
* This function initializes the BULK driver and registers a CBI - BULK
* drive with the USBD.  In addition, it also spawns a task to handle
* plugging / unplugging activity.
*
* RETURNS: Nothing
*/

void usrUsbBulkDevInit (void);

/*************************************************************************
*
* GetUSBDConnectState - 获取USBD连接状态
*
* RETURNS: initialized
*/
BOOL GetUSBDConnectState(void);

/*************************************************************************
*
* GetSl811hsConnectState - 获取Sl811hs连接状态
*
* RETURNS: sl811hsattached
*/
BOOL GetSl811hsConnectState(void);

/*************************************************************************
*
* GetBulkConnectState - 获取存储设备状态
*
* RETURNS: BulkConnected
*/
BOOL GetBulkConnectState(void);

/*************************************************************************
*
* GetMouseConnectState - 获取鼠标设备状态
*
* RETURNS: MouseConnected
*/
BOOL GetMouseConnectState(void);

/*************************************************************************
*
* GetBulkInfo - 获取存储设备信息
*
* RETURNS: 信息结构
*/
BULKINFO *GetBulkInfo(void);

#ifdef  __cplusplus
}
#endif

#endif
