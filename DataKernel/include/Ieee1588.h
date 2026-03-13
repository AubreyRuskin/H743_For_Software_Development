/*******************************************************************************
 *  UserFun.h
 *
 *  Created on: 2010-11-30
 *      Author: yepinyong
 *
 *  user functions
 *
 ******************************************************************************/
#ifndef _USERFUN
#define _USERFUN


#ifdef  __cplusplus
extern "C" {
#endif


typedef struct
{
    int            ptpUdpIpv4;                  //bool:TRUE or FALSE
    unsigned char  delayMechanism;
    //E2E = 0x01,                               request-response delay mechanism
    //P2P = 0x02,                               peer delay mechanism
    //ONLY_SYNTONIZATION = 0xFE,                does not implement the delay mechanism

    unsigned char  domainNumber;                //0~255 default 0
    unsigned char  priority1;                   //0~255 default 128
    unsigned char  priority2;                   //0~255 default 128
    char           logAnnounceInterval;         //-128~127
    char           logSyncInterval;             //-128~127
    char           logMinDelayReqInterval;      //-128~127
    char           logMinPdelayReqInterval;     //-128~127
    unsigned char  announceReceiptTimeout;      //0~255

    unsigned short srcUdpPort;                  //udp source port eg:2010
    char           srcIpAddress[4];             //ip address  eg:{192,168,0,6}
    char           srcMacAddress[6];            //mac sddress eg:{0x00,0xA0,0x1E,0xA8,0x00,0x06}
} CfgStruct;

/*******************************************************************************
*   disable spawn 1588 task in selection ethernet port.
*
*   portNum
*       ethernet port 0-2.
*
*   return
*       success return TRUE, else FALSE.
*
*   attention
*       If you do not want to spawn 1588 task in selection
*       ethernet port, call this function before run 1588 entry function.
*******************************************************************************/
int DelPtpPort(unsigned char portNum);


/*******************************************************************************
*   set MDIO management interface address
*
*   portNum
*       ethernet port 0-2
*   mdioAddr
*       MDIO management interface address connect the ethernet port
*       default ethernet port 0 MDIO address 1
*               ethernet port 1 MDIO address 3
*               ethernet port 2 MDIO address 5
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int SetMdioAddr(unsigned char portNum, unsigned char mdioAddr);


/*******************************************************************************
*   set FPGA chip base address
*
*   fpgaBase
*       base address(byte address)
*       default 0xA2000000
*
*   return
*       nothing
*******************************************************************************/
void SetFpgaBase(unsigned int fpgaBase);


/*******************************************************************************
*   set FPGA 1588 module offset address
*
*   portNum
*       ethernet port 0-1
*   ptpBase
*       FPGA 1588 module offset address(byte address)
*       default ethernet port 0 offset address 0x0C000
*               ethernet port 1 offset address 0x10000
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int SetPtpBase(unsigned char portNum, unsigned int ptpBase);

/*******************************************************************************
*   IEEE 1588 entry function
*
*   funMode
*       1588 implement selection
*       IEEE1588Hard    0x01
*       IEEE1588Soft    0x02
*       IEEE1588Fpga    0x03
*
*   return
*       nothing
*******************************************************************************/
void IEEE1588(unsigned char funMode);


/*******************************************************************************
*   hard 1588 entry function
*
*   return
*       nothing
*******************************************************************************/
void IEEE1588Hard(void);


/*******************************************************************************
*   soft 1588 entry function
*
*   return
*       nothing
*******************************************************************************/
void IEEE1588Soft(void) ;


/*******************************************************************************
*   FPGA 1588 entry function
*
*   return
*       nothing
*******************************************************************************/
void IEEE1588Fpga(void);


/*******************************************************************************
*   reconfigure PTP data sets
*
*   portNum
*       ethernet port 0-1
*   cfg
*       pointer to CfgStruct structure
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int RecfgDataSets(unsigned char portNum, CfgStruct *cfg);


/*******************************************************************************
*   get utc time information
*
*   portNum
*       ethernet port 0-1
*   second
*       pointer to utc second information
*   nanoSecond
*       pointer to utc nanosecond information
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int GetUtcTime(unsigned char portNum, unsigned int *second, unsigned int *nanoSecond);


/*******************************************************************************
*   set utc time information
*
*   portNum
*       ethernet port 0-1
*   second
*       utc second information
*   nanoSecond
*       utc nanosecond information
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int SetUtcTime(unsigned char portNum, unsigned int second, unsigned int nanoSecond);


/*******************************************************************************
*   get synchronization communication state
*
*   portNum
*       ethernet port 0-1
*   clockState
*       pointer to current clock state
*       INITIALIZING        0x01
*       FAULTY              0x02
*       DISABLED            0x03
*       LISTENING           0x04
*       PRE_MASTER          0x05
*       MASTER              0x06
*       PASSIVE             0x07
*       UNCALIBRATED        0x08
*       SLAVE               0x09
*   msgCommStu
*       pointer to synchronization communication state
*       TRUE    communication normal
*       FALSE   communication abnormal
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int GetSyncCommState(unsigned char portNum, unsigned char *clockState, int *msgCommStu);


/*******************************************************************************
*   get offset and route delay value
*
*   portNum
*       ethernet port 0-1
*   offsetFromMaster
*       offset from master(ns)
*   meanPathDelay
*       mean path delay(ns)
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int GetOffsetDelay(unsigned char portNum, long long *offsetFromMaster, long long *meanPathDelay);


/*******************************************************************************
*   get PPS status
*
*   portNum
*       ethernet port 0-1
*   ppsStatus
*       PPS output status
*       TRUE    has PPS
*       FALSE   no PPS
*
*   return
*       success return TRUE, else FALSE
*******************************************************************************/
int GetPPSStatus(unsigned char portNum, int *ppsStatus);


/*******************************************************************************
*   get soft version
*
*   return
*       soft version
*******************************************************************************/
unsigned short GetSoftVersion(void);


#ifdef  __cplusplus
}
#endif

#endif


