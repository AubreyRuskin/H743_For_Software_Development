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
/*      ds3231.h                                  EDPx-05-0.1           */
/*                                                                      */
/* COMPONENT                                                            */
/*                                                                      */
/*      DS3231  RTC and temperature                                     */
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
/*                                                                      */
/* DEPENDENCIES                                                         */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* HISTORY                                                              */
/*                                                                      */
/*         NAME            DATE                    REMARKS              */
/*                                                                      */
/*      Chen, Xinzhi      2006/8/25                 1.00                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/*   Note: These Functions must be called after I2C Initalized          */
/*                                                                      */
/************************************************************************/

#ifndef __INDS3231
#define __INDS3231

#ifdef __cplusplus
extern "C" {
#endif

int ds3231Read(UINT8 data_ptr,UINT8 *buf,UINT8 length);

int ds3231Write(UINT8 data_ptr,UINT8 *buf,UINT8 length);

#define ERROR_TEMPERATURE   127
/*  Function:   get temperature
    Return value: temperature, or ERROR_TEMPERATURE
*/
char ds3231_GetTemperature();

#ifdef __cplusplus
}
#endif
#endif  /*__INDS3231*/