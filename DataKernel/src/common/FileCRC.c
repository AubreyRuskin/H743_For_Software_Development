#include "filetool.h"
#include "FileCRC.h"
#include "logmsg.h"
#include "errtest.h"
#include "swcfg.h"

#include "string_compat.h"
#include "ctype_compat.h"
#include <stdio_compat.h>
#include <dirent_compat.h>
#include <sys_stat_compat.h>
#include <ioLib.h>
#include <lstLib.h>
#include <semLib.h>
#include <dosFsLib.h>
#include <taskLib.h>


SEM_ID semCkCRCIni_g;
CRC_FILE_INFO CrcInfo_g;




/* Write system config item. (Create it when not existing)
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      ulCrc, Crc value.
 * Return value:
 *      >0, number of new added items.
 *      =0, update all items success.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating. */
EP_STATUS FT_Wr_INI_CRC(const uint8_t *strHeader,
                        const uint8_t *strItems, const uint16_t ulCrc)
{
    EP_STATUS sts;
    uint8_t aucCrc[8];
    uint16_t i;

    if(!strcmp(strItems,CRC_ITEM_FUNC))
        CrcInfo_g.unFuncCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_DIFDORCE))
        CrcInfo_g.unDiforceCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_LINKMODE))
        CrcInfo_g.unLinkmodeCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_LINKSTATS))
        CrcInfo_g.unLinkstatsCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_HDCOF))
        CrcInfo_g.unHdcofCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_CLCOF))
        CrcInfo_g.unClcofCrc =ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_NBSET))
        CrcInfo_g.unNbsetCrc=ulCrc;
    else if(!strcmp(strItems,CRC_ITEM_CKSET))
        CrcInfo_g.unCksetCrc=ulCrc;
    else
    {
        i =strtol(strItems+4,NULL,10);
        CrcInfo_g.unAreaCrc[i]=ulCrc;
    }

    memset(aucCrc,0,sizeof(aucCrc));
    sprintf(aucCrc,"%04X",ulCrc);
    sts=FT_Wr_Sys_INI(strHeader,strItems,aucCrc);
    return sts;
}


/* Read system config item.
 * Parameters:
 *      strFile, file name.
 *      strItems, string of items name.
 * Return value:
 *      Crc value;
*/
uint16_t Get_Crc_Item(const uint8_t *strFile, const uint8_t *strItems)
{
    uint8_t i;
    uint8_t TempInfo[256];

    if(!strcmp(strItems,CRC_ITEM_FUNC))
    {
        if (CrcInfo_g.bFunStsWrFlag)
        {
            sprintf(TempInfo, "功能投退状态文件写入时异常!!\n");
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            CrcInfo_g.bFunStsWrFlag = FALSE;
            Write_FunSts_CRC(); /* 更新CRC, 写失败后续继续写或报错 */
            Set_FunSts_Wr_Sts(0); /* 清除写标识 */
        }

        return CrcInfo_g.unFuncCrc;
    }
    else if(!strcmp(strItems,CRC_ITEM_DIFDORCE))
        return CrcInfo_g.unDiforceCrc;
    else if(!strcmp(strItems,CRC_ITEM_LINKMODE))
        return CrcInfo_g.unLinkmodeCrc;
    else if(!strcmp(strItems,CRC_ITEM_LINKSTATS))
    {
        /* 压板文件 */
        if (CrcInfo_g.bLinkStatCrcWrFlag)
        {
            sprintf(TempInfo, "压板文件写入时异常!!\n");
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            CrcInfo_g.bLinkStatCrcWrFlag = FALSE;
            Write_Link_CRC(); /* 更新CRC, 写失败后续继续写或报错 */
            Set_Link_Wr_Sts(0); /* 清除写标识 */
        }

        return CrcInfo_g.unLinkstatsCrc;
    }
    else if(!strcmp(strItems,CRC_ITEM_HDCOF))
        return CrcInfo_g.unHdcofCrc;
    else if(!strcmp(strItems,CRC_ITEM_CLCOF))
        return CrcInfo_g.unClcofCrc;
    else if(!strcmp(strItems,CRC_ITEM_NBSET))
        return CrcInfo_g.unNbsetCrc;
    else if(!strcmp(strItems,CRC_ITEM_CKSET))
    {
        /* 测控定值 */
        if (CrcInfo_g.bCkSetCrcWrFlag)
        {
            sprintf(TempInfo, "测控定值文件写入时异常!!\n");
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            CrcInfo_g.bCkSetCrcWrFlag = FALSE;
            Write_Ckset_CRC(); /* 重新写入CRC, 写失败后续继续写或报错 */
            Set_Ckset_Wr_Sts(0);  /* 清除写标识 */
        }

        return CrcInfo_g.unCksetCrc;
    }
    else
    {
        i =strtol(strItems+4,NULL,10);

        if (CrcInfo_g.bAreaCrcWrFlag[i])
        {
            sprintf(TempInfo, "保护定值文件%d写入时异常!!\n", i);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            CrcInfo_g.bAreaCrcWrFlag[i] = FALSE;
            Write_Areaset_CRC((uint8_t *)strFile, i); /* 重新写入CRC */
            Set_Areaset_Wr_Sts(i, 0); /* 清除写标识 */
        }

        return CrcInfo_g.unAreaCrc[i];
    }
}




/* Check file crc.
 * Parameters:
 *		strFile,  string of file name.
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 * Return:
 *      EP_SUCCESS,
 *      EP_ERROR. */
EP_STATUS Check_Item_CRC(const uint8_t *strFile,const uint8_t *strHeader,const uint8_t *strItems)
{
    uint16_t unRdCrc;
    uint16_t unComCrc;



    if(FT_Is_File(strFile))
    {
        unRdCrc = Get_Crc_Item(strFile, strItems);
        unComCrc=FT_File_CRC16(strFile);
        LOG_Dbg_Msg("read  is %d,com is %d \n!",unRdCrc,unComCrc,0,0,0,0);

        if(unRdCrc !=unComCrc)
            return EP_ERROR;
        else
            return EP_SUCCESS;
    }
    else
        return EP_SUCCESS;
}



/* check nbset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      TRUE,
 *      FALSE. */


BOOL Check_Nbset_CRC()
{

    if(Check_Item_CRC(EP_INNER_SET_FILE,"CRC",CRC_ITEM_NBSET)!=EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "无效的内部定值文件\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "Invalid internal setting file\n", 0, 0);
        }
        return FALSE;
    }
    return TRUE;
}

/* check ckset crc when mmi or sgview use it.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      TRUE,
 *      FALSE. */


BOOL Check_Ckset_CRC()
{
    EP_STATUS sts;
    int i;
    uint8_t aucBuf[32];

    /* 读取写入状态 */
    if ((i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_CKSET_WR_STS, aucBuf, 32)) == 1)
    {
        CrcInfo_g.bCkSetCrcWrFlag = strtol(aucBuf, NULL, 16);
    }

    sts =Check_Item_CRC(EP_CK_SET_FILE,"CRC",CRC_ITEM_CKSET);

    if(sts!=EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "无效的参数定值文件\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "Invalid Measure&Control set file\n", 0, 0);
        }
        return FALSE;
    }
    return TRUE;
}

BOOL Check_Areaset_CRC(uint8_t *strFile)
{
    uint8_t puc[4];
    int iArea;
    uint8_t aucTemp[32];
    int i;
    uint8_t aucBuf[32];

    memcpy(puc,strFile+13,2);
    puc[2] ='\0';

    iArea =strtol(puc,NULL,16);

    /* 读取写入状态 */
    memset(aucTemp, 0, sizeof(aucTemp));
    sprintf(aucTemp, "%s%d", CRC_ITEM_AREA_WR_STS, iArea);
    if ((i = FT_Rd_Sys_INI("[CRC]", aucTemp, aucBuf, 32)) == 1)
    {
        CrcInfo_g.bAreaCrcWrFlag[i] = strtol(aucBuf, NULL, 16);
    }

    memset(aucTemp,0,sizeof(aucTemp));
    sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,iArea);
    if(Check_Item_CRC(strFile, "CRC", aucTemp)!=EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                       "定值区%d无效\n", iArea, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                       "Invalid setting sector %d\n", iArea, 0);
        }
        return FALSE;
    }
    else
        return TRUE;



    /*	strcpy(aucBuf, (EP_SET_AREA_DIR "/area00.dza"));
    	puc = aucBuf+strlen(aucBuf)-6;
    	for(iArea=0;iArea<32;iArea++)
    	{
    		puc[0] = (iArea/16)+'0';
    	    if (puc[0]>'9')
    	        puc[0] += ('a'-'9'-1);

    	   	puc[1] = (iArea%16)+'0';
    	    if (puc[1]>'9')
    	        puc[1] += ('a'-'9'-1);

    		if(strcmp(strFile,aucBuf)==0)
    		{
    			memset(aucTemp,0,sizeof(aucTemp));
    			sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,iArea);
    			if(Check_Item_CRC(strFile, "CRC", aucTemp)!=EP_SUCCESS)
    			{
    				if (ENG_MODE == 0)
    		        {
    		        	ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
    						"定值区%d无效\n", iArea, 0);
    				}
    				else if (ENG_MODE == 1)
    		       	{
    			       	ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
    						"Invalid setting sector %d\n", iArea, 0);
    				}
    				return FALSE;
    			}
    		}
    	}
    	return TRUE;
    	*/
}


void Write_Nbset_CRC()
{
    EP_STATUS sts;
    uint16_t unComCrc;
    unComCrc =0;
    unComCrc=FT_File_CRC16(EP_INNER_SET_FILE);
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_NBSET,unComCrc);
    assert (sts != EP_ERROR);

}

void Write_Ckset_CRC()
{
    EP_STATUS sts;
    uint16_t unComCrc;

    unComCrc =0;

    unComCrc=FT_File_CRC16(EP_CK_SET_FILE);
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CKSET,unComCrc);

    assert (sts != EP_ERROR);
}

/* 写压板文件CRC.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void Write_Link_CRC(void)
{
    uint16_t unComCrc = 0;

    unComCrc = FT_File_CRC16(EP_LINK_STS_FILE);
    FT_Wr_INI_CRC("[CRC]", CRC_ITEM_LINKSTATS, unComCrc);
}

/* 设置测控定值写状态.
 * Para:
 *     usWrSts, 状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS Set_Ckset_Wr_Sts(uint16_t usWrSts)
{
    uint8_t aucBuf[32];
    EP_STATUS Sts = EP_SUCCESS;

    sprintf(aucBuf, "%d", usWrSts);

    if (FT_Wr_Sys_INI("[CRC]", CRC_ITEM_CKSET_WR_STS, aucBuf)<0)
    {
        Sts = EP_ERROR;
    }

    return Sts;
}

void Write_Areaset_CRC(uint8_t *strFilename,uint8_t iArea)
{
    EP_STATUS sts;
    uint16_t unComCrc;
    uint8_t aucTemp[32];
    unComCrc =0;
    unComCrc=FT_File_CRC16(strFilename);
    memset(aucTemp,0,sizeof(aucTemp));
    sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,iArea);
    sts=FT_Wr_INI_CRC("[CRC]",aucTemp,unComCrc);
    assert (sts != EP_ERROR);
}

/* 设置保护定值写状态.
 * Para:
 *     usWrSts, 写状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS Set_Areaset_Wr_Sts(uint8_t iArea, uint16_t usWrSts)
{
    uint8_t aucTemp[32];
    uint8_t aucBuf[32];
    EP_STATUS Sts = EP_SUCCESS;

    sprintf(aucTemp, "%s%d", CRC_ITEM_AREA_WR_STS, iArea);
    sprintf(aucBuf, "%d", usWrSts);

    if (FT_Wr_Sys_INI("[CRC]", aucTemp, aucBuf)<0)
    {
        Sts = EP_ERROR;
    }

    return Sts;
}

/* 压板写状态.
 * Para:
 *     usWrSts, 写状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS Set_Link_Wr_Sts(uint16_t usWrSts)
{
    uint8_t aucBuf[32];
    EP_STATUS Sts = EP_SUCCESS;

    sprintf(aucBuf, "%d", usWrSts);

    if (FT_Wr_Sys_INI("[CRC]", CRC_ITEM_LINKSTAT_WR_STS, aucBuf)<0)
    {
        Sts = EP_ERROR;
    }

    return Sts;
}

void Write_Hdcof_CRC()
{
    EP_STATUS sts;
    uint16_t unComCrc;
    unComCrc =0;
    unComCrc=FT_File_CRC16(EP_AI_GAIN_FILE);
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,unComCrc);
    assert (sts != EP_ERROR);
}

void Write_Clcof_CRC()
{
    EP_STATUS sts;
    uint16_t unComCrc;
    unComCrc =0;
    unComCrc=FT_File_CRC16(EP_CL_GAIN_FILE);
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,unComCrc);
    assert (sts != EP_ERROR);
}

/* Initilize the config file module.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      EP_SUCCESS,
 *      EP_ERROR. */

void FileCRC_Check(void)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    uint8_t *puc;
    uint8_t aucTemp[32];
    int i;
    int iFd;


    if(FT_Is_File(EP_SYS_INI_FILE))
    {

        if(Check_Item_CRC(EP_INNER_SET_FILE,"CRC",CRC_ITEM_NBSET)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                           "无效的内部定值文件\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                           "Invalid internal setting file\n", 0, 0);
            }
        }
        if(Check_Item_CRC(EP_CK_SET_FILE,"CRC",CRC_ITEM_CKSET)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                           "无效的参数定值文件\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                           "Invalid Measure&Control set file\n", 0, 0);
            }
        }
        if(Check_Item_CRC(EP_FUNC_STS_FILE,"CRC",CRC_ITEM_FUNC)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "保护投退文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "Function file CRC check error.\n", 0, 0);
            }
        }

#if 0
        /* 修改为自身文件校验 */
        if(Check_Item_CRC(EP_DI_STS_FILE,"CRC",CRC_ITEM_DIFDORCE)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "开入强制文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "DI force file CRC check error.\n", 0, 0);
            }
        }
#endif

        if(Check_Item_CRC(EP_LINK_MODE_FILE,"CRC",CRC_ITEM_LINKMODE)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "压板模式文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "Link mode file CRC check error.\n", 0, 0);
            }
        }

#if 0
        /* 修改为文件自身校验 */
        if(Check_Item_CRC(EP_LINK_STS_FILE,"CRC",CRC_ITEM_LINKSTATS)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "压板状态文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "Link stats file CRC check error.\n", 0, 0);
            }
        }
#endif

        if(Check_Item_CRC(EP_AI_GAIN_FILE,"CRC",CRC_ITEM_HDCOF)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "通道系数文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "Ai cof file CRC check error.\n", 0, 0);
            }
        }
        if(Check_Item_CRC(EP_CL_GAIN_FILE,"CRC",CRC_ITEM_CLCOF)!=EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "测量系数文件校验出错.\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                           "cl cof file CRC check error.\n", 0, 0);
            }
        }
        strcpy(aucBuf, (EP_SET_AREA_DIR "/area00.dza"));
        puc = aucBuf+strlen(aucBuf)-6;
        for(i=0; i<32; i++)
        {
            puc[0] = (i/16)+'0';
            if (puc[0]>'9')
                puc[0] += ('a'-'9'-1);

            puc[1] = (i%16)+'0';
            if (puc[1]>'9')
                puc[1] += ('a'-'9'-1);
            if(FT_Is_File(aucBuf))
            {
                memset(aucTemp,0,sizeof(aucTemp));
                sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,i);


                if ((iFd = open(aucBuf, O_RDONLY, 0))!=ERROR)
                {
                    if (SC_Is_Valid_Set(iFd))
                    {
                        if(Check_Item_CRC(aucBuf, "CRC", aucTemp)!=EP_SUCCESS)
                        {
                            if (ENG_MODE == 0)
                            {
                                ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                           "定值区%d无效\n", i, 0);
                            }
                            else if (ENG_MODE == 1)
                            {
                                ER_Set_Err(EV_SET_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                           "Invalid setting sector %d\n", i, 0);
                            }
                        }

                    }
                    close(iFd);
                }
            }
        }
    }

}

void FileCRC_Init(void)
{
    int i,j;
    uint8_t aucBuf[32];
    uint8_t aucTemp[8];

    /* 由计数信号量改为互斥信号量, 防止死锁 */
    semCkCRCIni_g = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    assert (semCkCRCIni_g != NULL);

    if ((i=FT_Rd_Sys_INI("[CRC]", "FUNC", aucBuf, 32))==1)
        CrcInfo_g.unFuncCrc=strtol(aucBuf, NULL, 16);

    /* 读取功能投退文件写状态 */
    if ((i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_FUN_STS_WR_STS, aucBuf, 32)) == 1)
    {
        CrcInfo_g.bFunStsWrFlag = strtol(aucBuf, NULL, 16);
    }

    if ((i=FT_Rd_Sys_INI("[CRC]", "DIFORCE", aucBuf, 32))==1)
        CrcInfo_g.unDiforceCrc=strtol(aucBuf, NULL, 16);
    if ((i=FT_Rd_Sys_INI("[CRC]", "LINKSTATS", aucBuf, 32))==1)
        CrcInfo_g.unLinkstatsCrc=strtol(aucBuf, NULL, 16);

    /* 读取写入状态 */
    if ((i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_LINKSTAT_WR_STS, aucBuf, 32)) == 1)
    {
        CrcInfo_g.bLinkStatCrcWrFlag = strtol(aucBuf, NULL, 16);
    }

    if ((i=FT_Rd_Sys_INI("[CRC]", "LINKMODE", aucBuf, 32))==1)
        CrcInfo_g.unLinkmodeCrc=strtol(aucBuf, NULL, 16);
    if ((i=FT_Rd_Sys_INI("[CRC]", "NBSET", aucBuf, 32))==1)
        CrcInfo_g.unNbsetCrc=strtol(aucBuf, NULL, 16);
    if ((i=FT_Rd_Sys_INI("[CRC]", "CKSET", aucBuf, 32))==1)
        CrcInfo_g.unCksetCrc=strtol(aucBuf, NULL, 16);

    /* 读取写入状态 */
    if ((i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_CKSET_WR_STS, aucBuf, 32)) == 1)
    {
        CrcInfo_g.bCkSetCrcWrFlag = strtol(aucBuf, NULL, 16);
    }

    if ((i=FT_Rd_Sys_INI("[CRC]", "HDCOF", aucBuf, 32))==1)
        CrcInfo_g.unHdcofCrc=strtol(aucBuf, NULL, 16);
    if ((i=FT_Rd_Sys_INI("[CRC]", "CLCOF", aucBuf, 32))==1)
        CrcInfo_g.unClcofCrc=strtol(aucBuf, NULL, 16);
    for(i=0; i<32; i++)
    {
        memset(aucTemp,0,sizeof(aucTemp));
        sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,i);
        if((j=FT_Rd_Sys_INI("[CRC]", aucTemp, aucBuf, 32))==1)
        {
            CrcInfo_g.unAreaCrc[i]=strtol(aucBuf, NULL, 16);
        }

        /* 读取写入状态 */
        sprintf(aucTemp, "%s%d", CRC_ITEM_AREA_WR_STS, i);
        if ((j = FT_Rd_Sys_INI("[CRC]", aucTemp, aucBuf, 32)) == 1)
        {
            CrcInfo_g.bAreaCrcWrFlag[i] = strtol(aucBuf, NULL, 16);
        }
    }
}

/* 设置功能状态文件写状态.
 * Para:
 *     usWrSts, 状态, 0: 写结束; 1: 正在写.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS Set_FunSts_Wr_Sts(uint16_t usWrSts)
{
    uint8_t aucBuf[32];
    EP_STATUS Sts = EP_SUCCESS;

    sprintf(aucBuf, "%d", usWrSts);

    if (FT_Wr_Sys_INI("[CRC]", CRC_ITEM_FUN_STS_WR_STS, aucBuf)<0)
    {
        Sts = EP_ERROR;
    }

    return Sts;
}

/* 写功能状态投退文件CRC.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void Write_FunSts_CRC(void)
{
    uint16_t unComCrc = 0;

    unComCrc = FT_File_CRC16(EP_FUNC_STS_FILE);
    FT_Wr_INI_CRC("[CRC]", CRC_ITEM_FUNC, unComCrc);
}

/* 校验功能投退状态文件.
 * Parameters:
 *     None
 * Return:
 *     TRUE,
 *     FALSE.
 */
BOOL Check_FunSts_CRC(void)
{
    if (Check_Item_CRC(EP_FUNC_STS_FILE,"CRC",CRC_ITEM_FUNC) != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "无效的功能投退状态文件\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR,  ER_ALARM | ER_LOCK | ER_REPORT,
                       "Invalid function status file\n", 0, 0);
        }
        return FALSE;
    }

    return TRUE;
}