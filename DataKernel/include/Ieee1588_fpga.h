
/* UserFun.h */

#ifndef _USERFUN
#define _USERFUN


typedef struct
{
    unsigned long nanosecond;              /* 0-999999999 */
    unsigned char second;                   /* 0-59 */
    unsigned char minute;                   /* 0-59 */
    unsigned char hour;                     /* 0-23 */
    unsigned char day;                     /* 1-31 */
    unsigned char month;                    /* 1-12 */
    unsigned short year;                    /* 1970-2099 */
} DateStruct;


typedef struct
{
    int ptpUdpIpv4;         //bool:TRUE or FALSE
    unsigned char delayMechanism;
    /*
        E2E = 0x01, // The port is configured to use the delay request-response mechanism
        P2P = 0x02, // The port is configured to use the peer delay mechanism
        ONLY_SYNTONIZATION = 0xFE   // The port does not implement the delay mechanism
    */

    unsigned char domainNumber;             //0~255 default 0
    unsigned char priority1;                //0~255 default 128
    unsigned char priority2;                //0~255 default 128
    char logAnnounceInterval;               //-128~127
    char logSyncInterval;                   //-128~127
    char logMinDelayReqInterval;            //-128~127
    char logMinPdelayReqInterval;           //-128~127
    unsigned char announceReceiptTimeout;   //0~255

    unsigned short srcUdpPort;              //udp source port eg:2010
    char srcIpAddress[4];                   //ip address eg:{192,168,0,6}
    char srcMacAddress[6];                  //mac sddress eg:{0x00,0xA0,0x1E,0xA8,0x00,0x06}
} CfgStruct;




/*Function:  set FPGA chip base address(Power-on initialization call)
    Parameter:
        fpgaBase,   base address(byte address) default 0xA2000000
    return value:
*/
/*void SetFpgaBase(UInteger32 fpgaBase);
*/

/*Function:  hard 1588 main function(Power-on initialization call)
    Parameter:
        portNum,    ethernet port 0-1
        ptpBase,    1588 module base address(byte address)
            default ethernet port 0 base address 0x0C000
                    ethernet port 1 base address 0x10000
    return value:   success return TRUE, else FALSE
*/
/*
int SetPtpBase(UInteger8 portNum, UInteger32 ptpBase);
*/

/*Function:  hard 1588 main function(Power-on initialization call)
    Parameter:
    return value:
*/
void IEEE1588Fpga(void);


/*Function:   reconfigure PTP data sets
    Parameter:
        portNum,    ethernet port 0-1
        cfg,        pointer to configure data
    return value:   success return TRUE, else FALSE
    pay attention:  this function will restart all 1588 program
*/
/*
int RecfgDataSets(UInteger8 portNum, CfgStruct *cfg);
*/

/*Function:   get date information
    Parameter:
        portNum,    ethernet port 0-1
        utcDate,    pointer to got date information
    return value:   success return TRUE, else FALSE
*/
int GetDate(unsigned char portNum, DateStruct *utcDate);


/*  Function:   set date information
    Parameter:
        portNum,    ethernet port 0-1
        utcDate,    pointer to set date information
    return value:   success return TRUE, else FALSE
*/
int SetDate(unsigned char portNum, DateStruct *utcDate);


/*  Function:   get state information
    Parameter:
        portNum,    ethernet port 0-1
        clockState, pointer to current clock state

            INITIALIZING     0x01
            FAULTY               0x02
            DISABLED          0x03
            LISTENING         0x04
            PRE_MASTER     0x05
            MASTER             0x06
            PASSIVE            0x07
            UNCALIBRATED  0x08
            SLAVE                0x09

        msgCommStu,     pointer to synchronization communication state
            TRUE,  communication normal
            FALSE, communication abnormal
    return value:   success return TRUE, else FALSE
*/
int GetSyncCommState(unsigned char portNum, unsigned char *clockState, int *msgCommStu);


/*  Function:   get offset and delay value
    Parameter:
        portNum,    ethernet port 0-1
        offsetFromMaster,  offset from master(ns)
        meanPathDelay,   mean path delay(ns)
    return value:   success return TRUE, else FALSE
*/
int GetOffsetDelay(unsigned char portNum, long long *offsetFromMaster, long long *meanPathDelay);


/*  Function:   get PPS status
    Parameter:
        portNum,    ethernet port 0-1
        ppsStatus,  PPS output status
            TRUE,  has PPS
            FALSE, no PPS
    return value:   success return TRUE, else FALSE
*/
int GetPPSStatus(unsigned char portNum, int *ppsStatus);


/*  Function:   get soft version
    Parameter:
        null
    return value: soft version
*/
unsigned short GetSoftVersion(void);



#endif


