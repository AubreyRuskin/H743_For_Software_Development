/* auto_upload.h - subroutine library for handling event uploading */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add calibration methods for measuring.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling event uploading.
*/

#ifndef AUTO_UPLOAD_H
#define AUTO_UPLOAD_H

#ifdef	__cplusplus
extern "C" {
#endif

/* functions */

/***********************************************************************
* com_init - 初始化通讯
*
* RETURNS: 无
*
*/
void com_init();

/***********************************************************************
* CM_Deal_Info - 用来主动上传事件信息
*
* RETURNS: 无
*
*/
void CM_Deal_Info();

#ifdef  __cplusplus
}
#endif

#endif