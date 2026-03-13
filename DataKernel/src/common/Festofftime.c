

#include "Festofftime.h"
#include "hwcfg.h"
// #include "config04.h"
#include "miscfunc.h"
#include "semLib.h"

#define POFESTRAMMEM  		/* 铁电保存宏 */
#define FEST_SIZE 0x2000
#define TIME_SHOW_FORMAT "%04u-%02u-%02u %02u:%02u:%02u"  /* 包括字符串结尾符, 必须保证是20个字节 */
#define MAX_TIME_SHOW_LEN 20

#define TIME_GET_FORMAT "%d-%d-%d %d:%d:%d"
#define TIME_FORMAT_ELEMENTS 6

BOOL  bFestOK =FALSE;

#define FRAM_BUF_SIZE 64
unsigned char framBuf[FRAM_BUF_SIZE];

void Init_fram_ary()
{
    int i;

    for(i=0; i<FRAM_BUF_SIZE; i++)
    {
        framBuf[i] = i;
    }
}


void SetFset(uint32_t iCheck)
{
    if(iCheck ==0)
        bFestOK =TRUE;
    else
        bFestOK =FALSE;
}
/*提前写一次测试fest是否可用，如此测试不合理，需bsp提供返回值确定*/
/*01,02,BSP没有提供fm24cl64chk()*/
void CheckFset()
{
    /*	int retval;
    	int i =0;
    	char TempInfo[256];

    	uint32_t timebegin;
    	uint32_t timeend;

    	Init_fram_ary();
    	timebegin =tickGet();




    	for(i =0;i<(FEST_SIZE/2/4);i++)
    	{
    		if(read_ram_data(i*4, framBuf, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		if(write_ram_data(i*4, framBuf+8, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}


    		if(read_ram_data(i*4, framBuf+0x10, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		retval = memcmp(framBuf+8, framBuf+0x10, 4);
    		if(retval != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		if(write_ram_data(i*4, framBuf, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}
    		if(i ==1)
    		{
    			timeend =tickGet();
    			sprintf(TempInfo,"铁电检测耗时%dms!!",(timeend-timebegin)*10);
    			LOG_Write(LOG_KERNEL, TempInfo, NULL);
    		}
    	}

    	timeend =tickGet();
    	sprintf(TempInfo,"铁电检测耗时%dms!!",(timeend-timebegin)*10);
    	LOG_Write(LOG_KERNEL, TempInfo, NULL);

    	for(i =0;i<(FEST_SIZE/2/4);i++)
    	{
    		if(read_ram_data(FEST_SIZE/2+i*4, framBuf, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		if(write_ram_data(FEST_SIZE/2+i*4, framBuf+8, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}


    		if(read_ram_data(FEST_SIZE/2+i*4, framBuf+0x10, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		retval = memcmp(framBuf+8, framBuf+0x10, 4);
    		if(retval != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}

    		if(write_ram_data(FEST_SIZE/2+i*4, framBuf, 4) != 0)
    		{
    			bFestOK =FALSE;
    			return;
    		}
    	}

    	bFestOK =TRUE;

    	timeend =tickGet();
    	sprintf(TempInfo,"铁电检测耗时%dms!!",(timeend-timebegin)*10);
    	LOG_Write(LOG_KERNEL, TempInfo, NULL);
    */

    semI2CWrEnableFlag = semMCreate(SEM_Q_PRIORITY);		/* I2C访问许可 */
    assert (semI2CWrEnableFlag != NULL);

    bFestOK =TRUE;

}


BOOL GetFest()
{
    return bFestOK;
}

void WriteOfftime(uint32_t time32)
{

    uint32_t time;
    unsigned short ulenth;
    EP_DATE_TIME timer;
    uint8_t TempInfo[256] = {0};
    int i;
    uint16_t LastCRC=0;
    static uint32_t icount =0;
//	static BOOL bStats =FALSE;

    time =time32;
    TM_To_Dttm(time, &timer);


    ulenth = sprintf(TempInfo,TIME_SHOW_FORMAT,
                     timer.unYear, timer.ucMonth,
                     timer.ucDate, timer.ucHour, timer.ucMinute,
                     timer.ucSec);




    if(icount++%2) /*是否需要每次写出错报出来?*/
    {
        for(i =0; i<5; i++)
        {
            if (write_ram_data_cycle(0x0808+i*4, (unsigned char *)(&TempInfo[i*4]), 4) != OK)
            {
                /*		if(bStats ==FALSE)
                		{
                			if(ENG_MODE == 1)
                			{
                				LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                			}
                			else if(ENG_MODE == 0)
                			{
                				LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                	        }
                			bStats =TRUE;
                		}*/
                return;
            }
            LastCRC=EP_CCITT_CRC16(&TempInfo[i*4], 4, LastCRC);
        }

        if (write_ram_data_cycle(0x0800, (unsigned char *)&LastCRC, 2) != OK)
        {
            /*	if(bStats ==FALSE)
            	{
            		if(ENG_MODE == 1)
            		{
            			LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            		}
            		else if(ENG_MODE == 0)
            		{
            			LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
            	    }
            		bStats =TRUE;
            	}*/
            return;
        }
    }
    else
    {
        LastCRC =0;
        for(i =0; i<5; i++)
        {
            if (write_ram_data_cycle(0x1808+i*4, (unsigned char *)(&TempInfo[i*4]), 4) != OK)
            {
                /*if(bStats ==FALSE)
                {
                	if(ENG_MODE == 1)
                	{
                		LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                	}
                	else if(ENG_MODE == 0)
                	{
                		LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                    }
                	bStats =TRUE;
                }*/
                return;
            }
            LastCRC=EP_CCITT_CRC16(&TempInfo[i*4], 4, LastCRC);
        }

        if (write_ram_data_cycle(0x1800, (unsigned char *)&LastCRC, 2) != OK)
        {
            /*if(bStats ==FALSE)
            {
            	if(ENG_MODE == 1)
            	{
            		LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            	}
            	else if(ENG_MODE == 0)
            	{
            		LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            	bStats =TRUE;
            }
            */
            return;
        }
    }
}


BOOL ReadOfftime(char *TempInfo)
{
    // char Info[256]= {0};
    // int i;
    // uint16_t LastCRC=0;
    // uint16_t ComCRC =0;

    // if(TempInfo==NULL)
    //     return FALSE;

    // if(bFestOK ==FALSE)
    //     return FALSE;

    // if (read_ram_data(0x0800, (unsigned char *)&LastCRC, 2) == OK)
    // {
    //     for(i =0; i<5; i++)
    //     {
    //         if (read_ram_data(0x0808+i*4, (unsigned char *)(&TempInfo[i*4]), 4) != OK)
    //         {
    //             break;
    //         }
    //         ComCRC=EP_CCITT_CRC16(&TempInfo[i*4], 4, ComCRC);
    //     }

    //     TempInfo[MAX_TIME_SHOW_LEN] = '\0';

    //     if((i==5)&&(LastCRC==ComCRC))
    //     {
    //         if(ENG_MODE == 0)
    //         {
    //             sprintf(Info,"上次关机时间:%s\n",TempInfo);
    //         }
    //         else if(ENG_MODE == 1)
    //         {
    //             sprintf(Info,"latest turn off time:%s\n",TempInfo);
    //         }
    //         LOG_Write(LOG_KERNEL, Info, NULL);
    //         return TRUE;
        // }
    // }





    // ComCRC =0;
    // if (read_ram_data(0x1800, (unsigned char *)&LastCRC, 2) == OK)
    // {
    //     for(i =0; i<5; i++)
    //     {
    //         if (read_ram_data(0x1808+i*4, (unsigned char *)(&TempInfo[i*4]), 4) != OK)
    //         {
    //             break;
    //         }
    //         ComCRC=EP_CCITT_CRC16(&TempInfo[i*4], 4, ComCRC);
    //     }
    //     if((i==5)&&(LastCRC==ComCRC))
    //     {
    //         if(ENG_MODE == 0)
    //         {
    //             sprintf(Info,"上次关机时间:%s\n",TempInfo);
    //         }
    //         else if(ENG_MODE == 1)
    //         {
    //             sprintf(Info,"latest turn off time:%s\n",TempInfo);

    //         }
    //         LOG_Write(LOG_KERNEL, Info, NULL);
    //         return TRUE;
    //     }
    // }

    return FALSE;
}

BOOL GetDttmfromStr(char *pbuf, EP_DATE_TIME *pDttm)
{
    int year,month,date,hour,minute,second;

    if((pbuf==NULL)||(pDttm==NULL))
        return FALSE;

    if(sscanf(pbuf,TIME_SHOW_FORMAT, &year, &month,
              &date, &hour, &minute,&second)!=TIME_FORMAT_ELEMENTS)
        return FALSE;

    pDttm->unYear=year;
    pDttm->ucMonth=month;
    pDttm->ucDate=date;
    pDttm->ucHour=hour;
    pDttm->ucMinute=minute;
    pDttm->ucSec=second;

    pDttm->unMSEL = 0;
    pDttm->unMicroSec = 0;

    return TRUE;
}
