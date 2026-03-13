#ifndef FILECRC_H
#define FILECRC_H




#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "swcfg.h"

#define CRC_ITEM_FUNC  		"FUNC"
#define CRC_ITEM_DIFDORCE  	"DIFORCE"
#define CRC_ITEM_LINKMODE  	"LINKMODE"
#define CRC_ITEM_LINKSTATS  "LINKSTATS"
#define CRC_ITEM_LINKSTAT_WR_STS "LINKSTATWRSTS"  /* 压板写状态 */
#define CRC_ITEM_NBSET  	"NBSET"
#define CRC_ITEM_CKSET  	"CKSET"
#define CRC_ITEM_CKSET_WR_STS "CKSETWRSTS" /* 测控定值写状态 */
#define CRC_ITEM_HDCOF  	"HDCOF"
#define CRC_ITEM_CLCOF  	"CLCOF"
#define CRC_ITEM_AREA  		"AREA"
#define CRC_ITEM_AREA_WR_STS "AREAWRSTS" /* 保护定值写状态 */
#define CRC_ITEM_FUN_STS_WR_STS "FUNCWRSTS" /* 功能投退文件写状态 */

typedef struct
{
    uint16_t unFuncCrc;
    uint16_t unDiforceCrc;
    uint16_t unLinkmodeCrc;
    uint16_t unLinkstatsCrc;
    BOOL bLinkStatCrcWrFlag; /* 压板状态CRC写入标识 */
    uint16_t unNbsetCrc;
    uint16_t unCksetCrc;
    BOOL bCkSetCrcWrFlag; /* 测控定值CRC写入标识 */
    uint16_t unHdcofCrc;
    uint16_t unClcofCrc;
    uint16_t unAreaCrc[MAX_SET_AREA_NUM];
    BOOL bAreaCrcWrFlag[MAX_SET_AREA_NUM]; /* 保护定值CRC写入标识 */
    BOOL bFunStsWrFlag; /* 功能投退文件写入标识 */
} CRC_FILE_INFO;

extern CRC_FILE_INFO CrcInfo_g;

/* check nbset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      TRUE,
 *      FALSE. */

BOOL Check_Nbset_CRC();

/* check Ckset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      TRUE,
 *      FALSE. */

BOOL Check_Ckset_CRC();


/* Read system config item.
 * Parameters:
 *      strItems, string of items name.
 * Return value:
 *      Crc value;
*/

BOOL Check_Areaset_CRC(uint8_t *strFile);


/* write nbset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      TRUE,
 *      FALSE. */

void Write_Nbset_CRC();

/* write ckset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      NONE */

void Write_Ckset_CRC();

/* 写压板文件CRC.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void Write_Link_CRC(void);

/* 设置测控定值写状态.
 * Para:
 *     usWrSts, 写状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS Set_Ckset_Wr_Sts(uint16_t usWrSts);

/* write ckset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      strFilename, string of items name.
 * Return:
 *      NONE */

void Write_Areaset_CRC(uint8_t *strFilename,uint8_t iArea);

/*  slow task wtite it.
 * Parameters:
 *      NONE
 * Return:
 *      NONE */

void Write_Hdcof_CRC();

/*  slow task wtite it.
 * Parameters:
 *      NONE
 * Return:
 *      NONE */

void Write_Clcof_CRC();

/* Read system config item.
 * Parameters:
 *      strFile, file name.
 *      strItems, string of items name.
 * Return value:
 *      Crc value;
*/
extern uint16_t Get_Crc_Item(const uint8_t *strFile, const uint8_t *strItems);


/* Write system config item. (Create it when not existing)
 * Parameters:
 *      strField, field name.
 *      strItems, string of items name.
 *      ulCrc, Crc value.
 * Return value:
 *      EP_SUCCESS, item/field was added success.
 *      EP_LOCAL_MSG, modify the existing item.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is MAX_ID_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating. */
EP_STATUS FT_Wr_INI_CRC(const uint8_t *strField,
                        const uint8_t *strItems, const uint16_t ulCrc);


/* Initilize the config file module.
 * Parameters:
 *      None
 * Return:
 *      EP_SUCCESS,
 *      EP_ERROR. */
void FileCRC_Check(void);


void FileCRC_Init(void);


/* 设置保护定值写状态.
 * Para:
 *     usWrSts, 写状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS Set_Areaset_Wr_Sts(uint8_t iArea, uint16_t usWrSts);

/* 压板写状态.
 * Para:
 *     usWrSts, 写状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS Set_Link_Wr_Sts(uint16_t usWrSts);

/* 设置功能状态写标识.
 * Para:
 *     usWrSts, 状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS Set_FunSts_Wr_Sts(uint16_t usWrSts);

/* 写功能投退文件CRC.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void Write_FunSts_CRC(void);

/* 校验功能投退状态文件.
 * Parameters:
 *     None
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL Check_FunSts_CRC(void);

#ifdef	__cplusplus
}
#endif

#endif

