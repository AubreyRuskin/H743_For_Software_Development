
/*  UserFun.h */

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


/*Function:  hard 1588 main function(Power-on initialization call)
    Parameter:
    return value:
*/
void IEEE1588Hard(void) ;


/*Function:   get date information
    Parameter:
        portNum,    ethernet port 0-1
        utcDate,     pointer to got date information
    return value:   success return TRUE, else FALSE
*/
int GetDate(unsigned char portNum, DateStruct *utcDate);


/*  Function:   set date information
    Parameter:
        portNum,    ethernet port 0-1
        utcDate,     pointer to set date information
    return value:   success return TRUE, else FALSE
*/
int SetDate(unsigned char portNum, DateStruct *utcDate);


/*  Function:   get state information
    Parameter:
        portNum,    ethernet port 0-1
        clockState,  pointer to current clock state

            INITIALIZING     0x01
            FAULTY               0x02
            DISABLED          0x03
            LISTENING         0x04
            PRE_MASTER     0x05
            MASTER             0x06
            PASSIVE            0x07
            UNCALIBRATED  0x08
            SLAVE                0x09

        msgCommState,     pointer to synchronization communication state
            TRUE,  communication normal
            FALSE, communication abnormal
    return value:   success return TRUE, else FALSE
*/
int GetSyncCommState(unsigned char portNum, unsigned char *clockState, int *msgCommState);


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


