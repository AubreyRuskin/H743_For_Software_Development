/* MMI_MasterCPU.c - This file contains system MMI and Master CPU communicate functions */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
03b, 02May, sdm optimize  communication part ,assert , malloc() ,calloc().
03a, 25sep08, dy optimize the code adding assert after malloc() and calloc().
02b, 22jun06, hcj add 0x1200.
02a, 30jun06, hcj add modifycase 0x0d40, read files in a directory.
01a, 19dec02, hl first created.
*/

/*
DESCRIPTION
This file contains system MMI and Master CPU communicate functions.
INCLUDES: MMI_MasterCPU.h
*/

/* includes */

#include "string_compat.h"
#include "ioLib.h"
// #include "inetLib.h"
// #include "hostLib.h"
#include "semLib.h"
// #include "sysLib.h"
// #include "tickLib.h"
#include <dirent_compat.h>
#include <sys/stat.h>
#include "time_compat.h"
// #include <routeLib.h>

#include "edpbase.h"
#include "com_api.h"
#include "MMI_MasterCPU.h"
#include "MMI_MasterCPU_Inter.h"
#include "miscfunc.h"
#include "logmsg.h"
#include "stdio_compat.h"
#include "view.h"
#include "realdata.h"
#include "rec.h"
#include "errtest.h"

#include "stdlib_compat.h"
// #include "sockLib.h"
#include "taskLib.h"
// #include "netinet\in.h"
// #include "netinet\tcp_timer.h"
// #include "netinet\tcp.h"
// #include "netdb.h"
#include "sys_types_compat.h"
#include "datetime.h"
// #include "pipeDrv.h"
#include "filetool.h"
#include "swcfg.h"
#include "auto_upload.h"
#include "spiio.h"
#include "OPT_MmiInterface.h"
#include "VoltageWatch.h"
#include "RE_RelayEngine.h"
#include "EdpNetCfg.h"
#include "measure.h"
#include "sysinfo.h"
// #include "config04.h"
#include "FileSynPro.h"			/* File synchronization */
#include "msgQLib.h"
#include "detailoperate_log.h"

#include "edp_asst.h"
#include "smvcfg.h"
#include "FileCRC.h"
#include  "HDL_VtBox.h"
#include "smvcfg.h"
#include "Smv_Go_CommStat_File.h"
#include "sys_socket_compat.h"
#include "netinet_in_compat.h"
#include "arpa_inet_compat.h"
#include "netinet_tcp_compat.h"
#include <tickLib.h>
#include "bspinterface.h"
#include "error_compat.h"


int connect_with_timeout_posix(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout_sec)
{
    int flags, result, error;
    socklen_t len;
    fd_set wset;
    struct timeval tv;

    // 1. 将套接字设为非阻塞
    // 先获取原始的标志位
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return ERROR;
    }
    // 添加非阻塞标志
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        return ERROR;
    }

    // 2. 发起 connect()
    result = connect(sockfd, addr, addrlen);
    if (result == 0) {
        // 连接立即成功
        goto success;
    } else if (errno != EINPROGRESS) {
        // 立即出错
        perror("connect");
        goto failure;
    }
    // 如果 result 是 -1 且 errno 是 EINPROGRESS，则连接正在后台进行

    // 3. 使用 select() 等待并设置超时
    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    result = select(sockfd + 1, NULL, &wset, NULL, &tv);
    if (result < 0) {
        // select 本身出错
        perror("select");
        goto failure;
    } else if (result == 0) {
        // 超时
        fprintf(stderr, "connect timed out after %d seconds\n", timeout_sec);
        errno = ETIMEDOUT;
        goto failure;
    }
    // 如果 result > 0，表示套接字状态已改变（变为可写或有错误）

    // 4. 使用 getsockopt 检查套接字上的错误状态，确认连接是否真的成功
    len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        perror("getsockopt(SO_ERROR)");
        goto failure;
    }

    if (error != 0) {
        // 连接失败，error 变量中包含了真正的错误码
        fprintf(stderr, "connect failed: %s\n", strerror(error));
        errno = error;
        goto failure;
    }

success:
    // 5. (可选) 将套接字恢复为阻塞模式
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        perror("fcntl(F_SETFL, blocking)");
        return ERROR; // 即使恢复失败，连接也已成功
    }
    return OK;

failure:
    // 恢复原始标志位
    fcntl(sockfd, F_SETFL, flags);
    return ERROR;
}

//#ifdef EDP02_PSR_BUILD
///*add by xts 090421*/
///*文件名列表*/
//struct FileProtect *pfplhead=NULL;      /*指向文件写保护链表头*/
//char dza_filename[100]="/tffs/set/area";
//#endif
/* defines */

#define DBG_ALL_TRUE 1   /* 临时测试报文用, 表示所有操作都正常 */
#define YES 0x55
#define NO 0xaa
#define Event_Flag 0x01010101
#define Lamp_Flag 0x02020202
#define Pulse_Flag 0x03030303
#define RSV_Flag 0x04040404
#define SOE_Flag 0x05050505
#define AUTO_FRAME_ID 0xa55a3ae3
#define	Frame_Head_Lenth 20
#define Max_Frame_Data_Lenth 1900
#define Max_Frame_Lenth (Frame_Head_Lenth+Max_Frame_Data_Lenth+1)
#define Max_Filename_Len 200
#define EventBuf_Size 0x10000
#define EVENT_PIPE_NAME	"pipe_for_event"
#define SOE_PIPE_NAME "pipe_for_soe"
#define AutoSend_Pipe_Name "pipe_for_auto"
#define NUM_MSGS 20
#define ACK_TIMEOUT 2000  /* 报文等待应答的超时时间长度 */
/* #define Max_Common_Lenth 5000 */  /* 定义移至swcfg.h 文件 */
#define DELAY_TIME (sysClkRateGet()/20) /* 报文帧之间的发送时间间隔，暂定50毫秒 */

#define Rpt_IntelType 0x08            	/* 内部规约类型 */
#define Rpt_version 0x10   /* 规约版本号 */
#define MMI_addr 0x00	/* 报文中MMI的地址 */
#define PC_addr 0xd0

#define MAX_MEA_NUM 45

#define ADD_COMM_LOG

#define SERVER_STACK_SIZE (0x50000)  /* 服务任务栈空间大小 */
#define PASSWORD_FOR_INNER_SET_READ 0x5A3C /* 内部定值读取密码 */

/* 操作源类型定义 */

#define OP_MMI 0 /* MMI */
#define OP_MONITOR 1   /* 后台监控 */
#define OP_SGVIEW 2    /* SgView */
#define OP_MMI_NEW 3     /* MMI, 新模式 */


/*格式化盘符，目前只允许格式化/data */
#define FORMATTFFS 	0
#define FORMATSET 	1
#define FORMATDATA 	2
/*CHECK NAND FLASH OR FORMAT NAND FLASH*/
#define CHECKNANDFLASH 0
#define FORMATNANDFLASH 1

#define MAX_LINK_HALT_NUM 128 /* 最大通信中断记录次数 */

#define FASTER_OPR_SUPPORT /* 增加一个更快速的内部通信操作任务 */

/* 双点开入取值方式 */
#define DP_DI_VAL_MODE_UNDEF 0      /* 双点开入取值方式未定义,按照之前的逻辑执行 */
#define DP_DI_VAL_MODE_POSITIVE 1   /* 双点开入取值方式正向 */
#define DP_DI_VAL_MODE_COUNTER 2    /* 双点开入取值方式取反 */

#define CCD_CRC_ERR             0x01    /* CCD文件CRC校验出错 */
#define CCD_FILE_ERR            0x02    /* CCD文件内容校验出错 */

#define HMI_DP_DI_VAL_POSITIVE  0x20    /* 上送HMI状态,双点开入取值方式为不取反 */

BOOL g_bChkStatus = FALSE;	/*FALSE,非检测态,TRUE,检测态*/
BOOL g_bChkResult = FALSE;	/*FALSE,检测正常,TRUE,检测不正常*/
BOOL g_bFormatStatus = FALSE;	/*FALSE,非格式化,TRUE,格式化状态*/

/* typedefs */

struct AutoMsg
{
    uint8_t *MsgAddr;
};

typedef struct
{
    char filename[128];
    time_t timer;
    uint16_t FileSN;
} File_Info;

/* globals */

extern BOOL bSetIsValid_g;
extern u_int uiEdpStatus_g;
extern int iMeaAiNum_g;
extern int iMeaDiNum_g;
extern int iHwLedChNum_g;  /* 配置的面板LED总数 */
extern int iSwLedChNum_g;     /* 配置的显示屏LED总数 */
extern int iSubLgcNum_g; /* 逻辑分图个数 */
extern int iLinkNum_g; 			/* 配置的压板总个数 */
extern int iHwAiChNum_g;   /* 配置的物理AI总数 */
extern int iLgcDiChNum_g;     /* 配置的DI总数 */
extern int iLgcDoChNum_g;   /* 配置的DO总数 */
extern int iEvtNum_g;     /* 配置的事件总数*/
extern UNITE_VER_INFO UnVerInfo_g;
extern UNITE_VER_INFO UnVerInfoRd_g;
extern BOOL bulRelayTaskHasAutoSet_g;
extern SEM_ID semCkCRCIni_g;

extern BOOL g_bMMITimeValid;  	/*MMI的对时时间是否有效, 默认无效*/

extern BOOL bGetAbsTime;
extern uint32_t GetAbsTimeInterval;
extern BOOL bFstOrSecFlag;   /* 一次/二次选择, FALSE: 二次; TRUE: 一次 */

extern uint32_t g_ulCcdFileCheckCrc; /* 通过计算得出的CCD文件校验码 */

extern BOOL g_bCcdCrcErr;          /* CCD文件CRC校验出错 */
extern BOOL g_bCcdFileErr;         /* CCD文件内容错误 */

uint8_t MasterCPU_addr;
char local_IP[20];
char PSVIEW_IP[] = "192.168.10.200";
char testIP[] = "172.20.20.120";
struct sockaddr_in PCVIEW_IP;
uint8_t *pEventBuf;
uint32_t m_ulRecvAutoPtr=0;
char MMI_IP[] = "10.10.10.5";
char PC_IP[] = "192.168.10.132";

/* socket descriptor */
/* sock1_fd :transfer data but file */
/* sock2_fd :transfer file */
/* sock3_fd :for PC */
/* sock4_fd :for PC */
/* sock5_fd :initiative upload */

int sock1_fd, sock2_fd, sock3_fd, sock4_fd, sock5_fd, sock6_fd;
int sockrec_fd;
int new_fd1, new_fd2, new_fd3, new_fd4, new_fd5;
int new_fdrec;
uint8_t *pUsingBuf[5];
int UsingFD[5];
int pipe_fd;
BOOL server4_OK = FALSE;

BOOL server_hmi_1_OK = FALSE;
BOOL server_hmi_2_OK = FALSE;
BOOL server_hmi_3_OK = FALSE;
BOOL server_hmi_rec_OK = FALSE;

BOOL server3_OK = FALSE;
SEM_ID autosem;
SEM_ID server_sem1, server_sem2, server_sem3, server_sem4;
SEM_ID server_semrec;

BOOL AdjustTimeSuccessFlag = FALSE ;
SUB_MOD_INFO subModInfo[MAX_MOD_NUM]; /* 最多支持MAX_MOD_NUM个模件 */
uint8_t temp_Io_count;
OPT_CH_STS_REPORT chStsReportArray[6];

uint16_t RI_CNT;

MSG_Q_ID SendEventToMmiMsgQFd;		/* 到MMI队列 */
MSG_Q_ID SendEventToSgviewMsgQFd ; 		/* 到SGVIEW队列 */

uint16_t SOCK5_RI_CNT = 0;
uint16_t SOCK6_RI_CNT = 0;

/* 更快速内部通信任务建立 */

#ifdef FASTER_OPR_SUPPORT
int sock_faster_fd;
int new_faster_fd;
BOOL faster_server_OK = FALSE;
SEM_ID server_sem_faster;
#endif

#ifdef PANEL_HMI_SUPPORT
int ph_sock_fast_fd, ph_sock_slow_fd, ph_sock_auto_fd;
int ph_sockrec_fd;
int ph_new_fast_fd, ph_new_slow_fd;
int ph_new_sockrec_fd;

BOOL ph_server_OK = FALSE;

SEM_ID ph_server_sem_fast, ph_server_sem_slow, ph_server_sem_rec;

char PH_IP[] = "10.10.11.5";

uint16_t PH_SOCK_AUTO_RI_CNT = 0;
#endif

uint32_t ulLinkHaltTm; /* 上一终止时间 */
int g_ComRcvSts = 0;
BOOL bAlmDoIsForced = FALSE;    /* 告警开出是否强制 */

static uint8_t n_ucDpDiValMode = DP_DI_VAL_MODE_UNDEF;  /* 双点开入取值方式 */

/* 对时状态 */
BOOL g_TimeSynIntSts; /* 接口状态 */
BOOL g_TimeServeSts; /* 服务状态 */
BOOL g_TimeLeapSts; /* 跳变状态 */

/* functions */

/*
描述:获取CCD文件的CRC.
暂时直接将文件中的CRC传给HMI，后续进行计算，并校验
给出实际的CRC，如不一致要闭所保护。
*/
extern char* EDP_GetCcdCreatTime();

void MasterCPU_socket1_service(void);
void MasterCPU_socket2_service(void);

void MasterCPU_socket_service(
    int lsock_fd,int hsock_fd
);

EP_STATUS reply(
    int sockid,
    uint8_t *p_sendbuff,
    uint8_t *p_rcvbuff,
    uint16_t TYPE,
    uint32_t FDTN
);

void explain(
    int new_fd,
    uint8_t *p_rcv_buffer,
    int rcv_number,
    int *psock_fd
);

extern BOOL isNumber_2_04CPU();
extern int LowFormat(unsigned char *pBuf);

/*功能：得到CPU的SV虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回SV虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
extern EP_STATUS  SMV_Get_Vt_SV_Term_Cfg(SMV_TOTAL_VT_SV_TERM_CFG   **ppRtTotalCfgAddr);

/*功能：得到CPU虚拟SV虚端子状态信息  2013-6-5 ZY
  参数：pRtTotalSTS：供返回SV虚端子的总体状态信息变量指针。
                  该变量，由调用方分配，被调用方填充
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
extern EP_STATUS  SMV_Get_Vt_SV_Term_Sts(SMV_TOTAL_VT_SV_TERM_STS   *pRtTotalSts);

extern STATUS chkdsk
(
    const char * pDevName,    /* device name */
    u_int        repairLevel, /* how to fix errors */
    u_int        verbose      /* verbosity level */
);

int PipeServerTask();

int WaitAutoTask(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

#ifdef GXC01U
int WaitAutoSendToGxc01UTask(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);
#endif

/***********************************************************************
* ReceiveWithTimeout - 接收函数
*
* RETURNS: 无
*
*/
EP_STATUS ReceiveWithTimeout(
    int sock_fd,
    int8_t *pcBuf,
    int32_t lLength,
    uint32_t ulTime
);

STATUS set_date(int year, int month, int day, int hour, int minute, int second){
    return ERROR;
}

/* global functions */

/***********************************************************************
* GetAdjustTimeSuccessFlag - 获取对时是否成功标志
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetAdjustTimeSuccessFlag()
{
    return AdjustTimeSuccessFlag ;
}

/***********************************************************************
* GetConnectMmiSuccessFlag - 获取与MMI连接是否成功标志
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetConnectMmiSuccessFlag()
{
    return bConnectMmiSuccessFlag;
}

/***********************************************************************
* SetAdjustTimeSuccessFlag - 设置系统对时标志
*
* RETURNS: 无
*
*/
void SetAdjustTimeSuccessFlag(
    BOOL bOkFlag
)
{
    AdjustTimeSuccessFlag=bOkFlag;
}

/***********************************************************************
* MasterCPU_autosocket_create - 建立socket5、6连接,用于主动上传
*
* RETURNS: 无
*
*/
EP_STATUS MasterCPU_autosocket_create(
    int *sfd
);

/* functions */

/***********************************************************************
* Comm_Log_Write - 将通讯错误记入日志
*
* RETURNS: 无
*
*/
void Comm_Log_Write(
    int code,
    char *pShowStr
)
{
#ifdef ADD_COMM_LOG
    static int32_t LogWritefreq[20][3]=
    {
        {1,100,10000},  /* 0 */
        {1,100,10000}, /* 1 */
        {10,100,1000}, /* 2 */
        {1,10,1000}, 		/* 3 */
        {1,10,1000}, 	/* 4 */
        {100,1000L,50000L}, 	/* 5 */
        {5,100,10000}, 	/* 6 */
        {5,100,10000}, 		/* 7 */
        /* 以下未使用 */
        {5,100,10000},  	/* 8 */
        {5,100,10000},  /* 9 */
        {5,100,10000},  /* 10 */
        {5,100,10000},  /*	11 */
        {5,100,10000},  /*  12 */
    };

    static uint32_t logCountArr[20]= {0};
    static char logStr[129]= {0};

    logCountArr[code]++ ;
    if((logCountArr[code] == LogWritefreq[code][0])||
            (logCountArr[code] == LogWritefreq[code][1]) ||
            ((logCountArr[code]%LogWritefreq[code][2]) == 0))
    {
        sprintf(logStr, "%s-%ld\n", pShowStr, logCountArr[code]);
        LOG_Write(LOG_RUN, logStr, NULL);
    }
#endif
}

/***********************************************************************
* MasterCPU_serversock_create - 建立socket连接
*
* RETURNS: 无
*
*/
int MasterCPU_serversock_create(
    char *LocalIP, 		/* IP地址 */
    int LocalPort	/* 端口地址 */
)
{
    struct sockaddr_in server_addr;
    int retcode;
    int sock_fd;
    int sockoptval;
    struct linger lingerval;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(LocalPort);
    server_addr.sin_addr.s_addr=inet_addr(LocalIP);

    sock_fd=socket(PF_INET, SOCK_STREAM, 0);		/* 打开socket */
    if(sock_fd<0)
    {
        char logStr[129]= {0};

        LOG_Dbg_Msg("can't create socket\n", 0, 0, 0, 0, 0, 0);
        sprintf(logStr, "can't create socket, LocalPort is %d", LocalPort);
        Comm_Log_Write(0, logStr);

        return EP_ERROR;
        /* 错误处理(代码待填) */
    }
    sockoptval=1;
    setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&sockoptval, sizeof (sockoptval));	/* 设置socket属性 */

    lingerval.l_onoff=1;
    lingerval.l_linger=0;
    setsockopt (sock_fd, SOL_SOCKET, SO_LINGER,(char *)&lingerval, sizeof (lingerval));

    sockoptval=1;
    setsockopt (sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&sockoptval, sizeof (sockoptval));
    if((retcode=bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))<0)		/* 绑定IP地址 */
    {
        char logStr[129]= {0};

        LOG_Dbg_Msg("can't bind to port\n", 0, 0, 0, 0, 0, 0);
        sprintf(logStr, "can't bind to port, LocalPort is %d, IP is %s\n", LocalPort, LocalIP);
        Comm_Log_Write(1, logStr);
        perror("MasterCPU_serversock_create");
        if (sock_fd >= 0)
        {
            /* 关闭已打开socket */
            close(sock_fd);
            sock_fd = -1;
        }

        return EP_ERROR;
        /* 错误处理(代码待填) */
    }
    if(listen(sock_fd, 5)<0)		/* 侦听端口 */
    {
        char logStr[129]= {0};

        LOG_Dbg_Msg("can't listen on port\n", 0, 0, 0, 0, 0, 0);
        sprintf(logStr, "can't listen on port, LocalPort is %d", LocalPort);
        Comm_Log_Write(1, logStr);

        if (sock_fd >= 0)
        {
            /* 关闭已打开socket */
            close(sock_fd);
            sock_fd = -1;
        }

        return EP_ERROR;
        /* 错误处理(代码待填) */
    }
    return sock_fd;
}

/***********************************************************************
* MasterCPU_socket_listen - 侦听端口
*
* RETURNS: 无
*
*/
void MasterCPU_socket_listen(
    int *sock_fd,		/* 当前端口 */
    int *pnew_fd	/* 新端口 */
)
{
    struct sockaddr_in client_addr;
    socklen_t sin_size;
    int new_fd;
    TASK_ID tid;
    char staskname1[16]="server";/*sdm 从10改为16，防止PANEL_HMI_SUPPORT打开时越界访问*/
    char staskname[20]="";
    int sockoptval;
    struct linger lingerval;
    uint8_t ucCnt=1;
    uint32_t ulAcceptCnt = 0;

    /******************************测试用代码**********************************************/
    if(sock_fd == &sock1_fd)
        strncat(staskname1, "1", 1);
    if(sock_fd == &sock2_fd)
        strncat(staskname1, "2", 1);
    if(sock_fd == &sockrec_fd)
        strncat(staskname1, "rec", 3);
    if(sock_fd == &sock3_fd)
        strncat(staskname1, "3", 1);
    if(sock_fd == &sock4_fd)
        strncat(staskname1, "4", 1);

#ifdef FASTER_OPR_SUPPORT
    if (sock_fd == &sock_faster_fd)
    {
        strcat(staskname1, "_faster");
    }
#endif

#ifdef PANEL_HMI_SUPPORT
    if(sock_fd == &ph_sock_fast_fd)
        strcat(staskname1, "_ph_fast");
    if(sock_fd == &ph_sock_slow_fd)
        strcat(staskname1, "_ph_slow");
    if(sock_fd == &ph_sockrec_fd)
        strcat(staskname1, "_ph_rec");
#endif
    /**************************************************************************************/

    /* MasterCPU_socket1_create(); */
    sin_size=sizeof(client_addr);

    while(1)
    {
        if((new_fd=accept(*sock_fd, (struct sockaddr *)&client_addr, &sin_size)) <= 0)		/* 接收数据 */
        {
            char logStr[129]= {0};
            
            // perror("MasterCPU_socket_listen"); 
            sprintf(logStr, "%s sock_fd=%d", "accept error on port", *sock_fd);
            Comm_Log_Write(2, logStr);
            ulAcceptCnt++;

            /* 错误处理(代码待填) */
            continue;
        }

        /* 20130719 经尹丁佘讨论决定此时拒绝新连接
         * 此时如果server任务有问题, 将导致通信中断
         */
        if (*pnew_fd>0)
        {
            uint8_t s_aucPrompt[MESSAGE_MAX_LEN];
            static uint32_t ulLinkNum = 0;

            ulLinkNum++;
            if (ulLinkNum <= MAX_LINK_HALT_NUM)
            {
                sprintf(s_aucPrompt, "%s重连 %lu-%lu-%lu.\n",
                        staskname1, TM_Get_usCnt(), ulLinkHaltTm, ulAcceptCnt);

                LOG_Write(LOG_KERNEL, s_aucPrompt, NULL);
            }

            close(new_fd);
            new_fd = -1;
            continue;
        }
        else
        {
            uint8_t s_aucPrompt[MESSAGE_MAX_LEN];
            static uint32_t ulLinkNum = 0;

            ulLinkNum++;
            if (ulLinkNum <= MAX_LINK_HALT_NUM)
            {
                sprintf(s_aucPrompt, "%s重连,前一任务已退出, %lu-%lu-%lu.\n",
                        staskname1, TM_Get_usCnt(), ulLinkHaltTm, ulAcceptCnt);

                LOG_Write(LOG_KERNEL, s_aucPrompt, NULL);
            }

            *pnew_fd = new_fd;
        }

        sprintf(staskname,"%s-%d",staskname1,ucCnt);
        ucCnt++;

        sockoptval=1;
        setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE,(char *)&sockoptval, sizeof (sockoptval));
        lingerval.l_onoff=1;
        lingerval.l_linger=0;
        setsockopt (new_fd, SOL_SOCKET, SO_LINGER,(char *)&lingerval, sizeof (lingerval));
        sockoptval=1;
        setsockopt (new_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&sockoptval, sizeof (sockoptval));		/* 设置属性 */

        if(sock_fd == &sock1_fd)
        {
            server3_OK=TRUE;
            server_hmi_1_OK = TRUE;
            semGive(server_sem1);
        }
        else if(sock_fd == &sock2_fd)
        {
            server3_OK=TRUE;
            server_hmi_2_OK = TRUE;
            semGive(server_sem2);
        }
        else if(sock_fd == &sockrec_fd)
        {
            server3_OK=TRUE;
            server_hmi_rec_OK = TRUE;
            semGive(server_semrec);
        }
        else if(sock_fd == &sock3_fd)
        {
            server3_OK=TRUE;
            server_hmi_3_OK = TRUE;

            semGive(server_sem3);
        }
        else if(sock_fd == &sock4_fd)
        {
            PCVIEW_IP.sin_addr.s_addr=client_addr.sin_addr.s_addr;
            server4_OK=TRUE;

            //MasterCPU_autosocket_create(&sock6_fd);
            semGive(server_sem4);
        }

#ifdef PANEL_HMI_SUPPORT
        else if(sock_fd == &ph_sock_fast_fd)
        {
            ph_server_OK=TRUE;
            semGive(ph_server_sem_fast);
        }
        else if(sock_fd == &ph_sock_slow_fd)
        {
            ph_server_OK=TRUE;
            semGive(ph_server_sem_slow);
        }
        else if(sock_fd == &ph_sockrec_fd)
        {
            ph_server_OK=TRUE;
            semGive(ph_server_sem_rec);
        }

        if((sock_fd == &ph_sockrec_fd)||(sock_fd == &sockrec_fd))
        {
            tid=taskSpawn(staskname, TSK_PRI_REC_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR )MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }
        else if((sock_fd == &ph_sock_fast_fd)||(sock_fd == &sock1_fd))
        {
            /* 与MMI快速通信任务 */
            tid=taskSpawn(staskname, TSK_PRI_FAST_SEVER_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR)MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }
        else
        {
            tid=taskSpawn(staskname, TSK_PRI_SEVER_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR)MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }

#else
        /* 此处激活接收发送任务(代码待填) */
        if(sock_fd == &sockrec_fd)
        {
            tid=taskSpawn(staskname, TSK_PRI_REC_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR )MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }
        else if (sock_fd == &sock1_fd)
        {
            /* 与MMI快速通信任务 */
            tid=taskSpawn(staskname, TSK_PRI_FAST_SEVER_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR)MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }
#ifdef FASTER_OPR_SUPPORT
        else if (sock_fd == &sock_faster_fd)
        {
            faster_server_OK = TRUE;
            semGive(server_sem_faster);
            tid = taskSpawn(staskname, TSK_PRI_EVT_SND-2, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR)MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
        }
#endif
        else
            tid=taskSpawn(staskname, TSK_PRI_SEVER_SVR, VX_FP_TASK, SERVER_STACK_SIZE, (FUNCPTR)MasterCPU_socket_service, (int)pnew_fd, (int)(((uint64_t)(pnew_fd))>>32), 0, 0, 0, 0, 0, 0, 0, 0);
#endif

        if(tid ==ERROR )
        {
            static BOOL bTaskError=TRUE;
            if (bTaskError)
            {
                bTaskError=FALSE;
                if(ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "Commnication server task create error.\n", NULL);
                }
                else if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "通信server任务创建失败.\n", NULL);
                }
            }

            /* 如果创建任务失败, 自动关闭该fd , 等待下一次连接 */
            close(*pnew_fd);

            *pnew_fd = -1;
        }
        else
        {
            static uint32_t ulLinkNum = 0;
            char TempInfo[TEMP_INFO_MAX_LEN];

            ulLinkNum++;
            if (ulLinkNum <= MAX_LINK_HALT_NUM)
            {
                sprintf(TempInfo, "%s通信任务创建.\n",
                        staskname);
                AddTaskToList(tid, TRUE, TempInfo, FALSE);
            }
        }

        taskDelay(sysClkRateGet()/10);		/* 等待100ms */
    }
}

/***********************************************************************
* Get_FWS - 添加保护文进链表
*
* RETURNS: 无
*
*/
uint8_t Get_FWS(
    char *pfilename,
    struct FileProtect *pFileProtectList
)
{
    return 0;
#if 0
    struct FileProtect *pfplthis;			/* 指向文件写保护链表当前位置 */
    struct FileProtect *pfplnew;			/* 指向新分配的链表结点 */
    struct FileProtect *pfplnext;

    if(pFileProtectList == NULL)
    {
        /* 如果链表为空，则分配新链表 */
        pfplnew=(struct FileProtect *)malloc(sizeof(struct FileProtect));
        assert(pfplnew);
        pFileProtectList=pfplnew;
        strcpy(pFileProtectList->filename, pfilename);
        pFileProtectList->writing_staus=0;
        pFileProtectList->next=NULL;
        return 0;
    }
    else
    {
        pfplthis=pFileProtectList;
        pfplnext=pfplthis;
        while(pfplnext != NULL)
        {
            if(!strcmp(pfplthis->filename, pfilename))		/* 找到当前文件 */
                return pfplthis->writing_staus;
            pfplnext=pfplnext->next;
            if(pfplnext != NULL)
            {
                pfplthis=pfplnext;
            }
        }
        pfplnew=(struct FileProtect *)malloc(sizeof(struct FileProtect));
        assert(pfplnew);
        pfplthis->next=pfplnew;
        pfplthis=pfplnew;
        strcpy(pfplthis->filename,pfilename);
        pfplthis->writing_staus=0;
        pfplthis->next=NULL;
        return 0;
    }
#endif
}

/***********************************************************************
* Set_FWS - 设置某文件属性
*
* RETURNS: 无
*
*/
void Set_FWS(
    char *pfilename,
    struct FileProtect *pFileProtectList,
    uint8_t WritingSTAUS
)
{
    struct FileProtect *pfplthis;			/* 指向文件写保护链表当前位置 */

    pfplthis=pFileProtectList;
    while(pfplthis != NULL)
    {
        if(strcmp(pfplthis->filename, pfilename) == 0)
        {
            pfplthis->writing_staus=WritingSTAUS;

            return;
        }
        pfplthis=pfplthis->next;
    }
}
/***********************************************************************
* Solid_Set_OK - 固化定值成功报文
*
* RETURNS: 无
*
*/
void Solid_Set_OK(
    int 		sockid,
    uint8_t 	*p_send_buffer,
    uint8_t 	*rcv_buffer,
    uint16_t 	send_lenth)
{
    uint8_t *p;
    p=p_send_buffer;
    *p++=0x00;
    *p++=0x00;
    *p++=0xc0;
    *p++=0x01;
    *p++=0x00;
    send_lenth=p-p_send_buffer;
    reply(sockid,p_send_buffer,rcv_buffer,0x8e00,send_lenth);

}
/***********************************************************************
* Solid_Set_Err - 固化定值失败报文
* 因定值区文件内容错误，导致的固化定值区文件失败
* RETURNS: 无
*
*/
void Solid_Set_Err(
    int 		sockid,
    uint8_t 	*p_send_buffer,
    uint8_t 	*rcv_buffer,
    uint16_t 	send_lenth)

{
    uint8_t *p;
    p=p_send_buffer;
    *p++=0x00;
    *p++=0x00;
    *p++=0xd1;
    *p++=0x01;
    *p++=0x00;
    send_lenth=p-p_send_buffer;
    reply(sockid,p_send_buffer,rcv_buffer,0x8e00,send_lenth);
}

/***********************************************************************
* Solid_Set_Err - 固化定值失败报文
*  因其他未知错误，导致的固化定值区文件失败
* RETURNS: 无
*
*/
void Solid_Set_Unknown_Err(
    int 		sockid,
    uint8_t 	*p_send_buffer,
    uint8_t 	*rcv_buffer,
    uint16_t 	send_lenth)

{
    uint8_t *p;
    p=p_send_buffer;
    *p++=0x00;
    *p++=0x00;
    *p++=0xdf;
    *p++=0x01;
    *p++=0x00;
    send_lenth=p-p_send_buffer;
    reply(sockid,p_send_buffer,rcv_buffer,0x8e00,send_lenth);
}

/***********************************************************************
* Send_Download_File_OK - 发送下载文件成功报文
*
* RETURNS: 无
*
*/
void Send_Download_File_OK(
    int 		sockid,
    uint8_t 	*p_send_buffer,
    uint8_t 	*rcv_buffer,
    uint16_t 	filenamelen,
    char        *filename,
    uint32_t 	file_all_lenth)
{
    uint8_t *p;
    p=p_send_buffer;
    *p++=LO8(filenamelen);
    *p++=HI8(filenamelen);
    memcpy(p,filename,filenamelen);
    p+=filenamelen;
    *p++=0x00;
    *p++=0x00;
    *p++=LL8(file_all_lenth);
    *p++=LH8(file_all_lenth);
    *p++=HL8(file_all_lenth);
    *p++=HH8(file_all_lenth);
    reply(sockid,p_send_buffer,rcv_buffer,0x9100,p-p_send_buffer);
}
/***********************************************************************
* Send_Download_File_Err - 发送下载文件失败报文
*
* RETURNS: 无
*
*/
void Send_Download_File_Err(
    int 		sockid,
    uint8_t 	*p_send_buffer,
    uint8_t 	*rcv_buffer,
    char        *filename,
    uint16_t 	filenamelen,
    uint8_t		err_type)
{
    uint8_t *p;
    p=p_send_buffer;
    *p++=LO8(filenamelen);
    *p++=HI8(filenamelen);
    memcpy(p,filename,filenamelen);
    p+=filenamelen;
    *p++=0x00;
    *p++=0x00;
    *p++=err_type;
    *p++=0x00;
    reply(sockid,p_send_buffer,rcv_buffer,0x9400,p-p_send_buffer);
}
/***********************************************************************
* Bak_File_Name - 获取临时文件名
*
* RETURNS: 无
*
*/
void Bak_File_Name(
    char *temp_filename,
    char *strFile,
    char *insChar
)
{
    strcpy(temp_filename, strFile );
    strcat(temp_filename, insChar);
}
/***********************************************************************
* Temp_File_Name - 获取临时文件名
*
* RETURNS: 无
*
*/
void Temp_File_Name(
    char *temp_filename,
    char *strFile,
    char insChar
)
{
    uint8_t *puc;

    strcpy(temp_filename, strFile );
    /* Let's add a '#' or a '$' just after the last '/'. */
    for (puc=temp_filename+strlen(temp_filename)-1; puc >= (uint8_t *)temp_filename; puc--)
    {
        if (*puc == '/')
            break;
    }
    memmove(puc+2, puc+1, strlen(puc)+1);
    puc[1]=insChar;
}

/***********************************************************************
* MasterCPU_socket_service - 服务端函数
*
* RETURNS: 无
*
*/
void MasterCPU_socket_service(
    int lpsock_fd,		/* socket */
    int hpsock_fd
)
{
    uint8_t rcv_buffer[Max_Frame_Lenth];
    uint32_t rcved_number=0;
    uint32_t rpt_lenth = 0;
    uint32_t Framedatalen=0;
    uint16_t Current_FrameSeq=0;
    uint16_t rcvFrameCnt=0;
    uint16_t filenamelen=0;
    uint32_t filelenth=0;
    uint32_t temp_val1;
    uint32_t file_lenth = 0;
    uint16_t ReportType;
    uint16_t file_head_lenth=0;
    uint16_t LastCRC = 0;
    uint32_t file_all_lenth=0;
    char delete_filename[Max_Filename_Len];
    char old_PIO_name[256]="";
    char PIO_name[256]="";
    char filename[Max_Filename_Len];
    char back_filename[Max_Filename_Len];
    char new_tmp_filename[Max_Filename_Len];
    char writing_filename[Max_Filename_Len]=""; /* 结尾符 */
    int rcv_number=0;
    int new_fileID=-1;
    uint8_t p_send_buffer[Max_Frame_Lenth];
    char ext_filename[4]="";  /* 原有为3, 有溢出现象 */
    char error_msg[256]="";
    uint8_t err_msg_len;
    char temp_val;
    int fp = -1;
    uint8_t *p;
    int opt_success=1;
    int i = 0;
    uint8_t uctemp_val;
    uint8_t remote_Type;
    uint8_t remote_ObjCode;
    uint8_t remote_OprCode;
    uint8_t kc_Code;
    uint8_t kc_SN;
    SC_SUB_LGC_ITEM *pSC_SUB_LGC_ITEM = NULL;
    BOOL Opc = FALSE;
    BOOL bOldStats;
    uint8_t OptSts = 0;
    uint16_t rest_lenth=Max_Frame_Lenth;
    uint16_t data_lenth=Frame_Head_Lenth;
    uint16_t rcv_cnt=0;
    uint8_t ptemp_buffer[Max_Frame_Lenth];
    int rtcode;
    uint16_t ulTotalLinkMode=0;
    uint8_t fileCRC_L = 0;  /* low file CRC. */
    uint8_t fileCRC_H = 0;	    /* high file CRC. */
    uint16_t FrameNum = 0;    /* frame number. */
    uint16_t usOpSrc = 0;    /* 操作源类型. */
    uint8_t aucBuf[FULL_NAME_LEN+1];
    STATUS vxsts;
    int sock_tmp;
    uint16_t ulCrc=0;
    SC_LINK_ITEM *plink = NULL;
    uint8_t ucServerFlag = 0;

    uint16_t linkNum=0;
    rcv_number=0;

    //64位支持
    uint64_t psock_fd_u64=hpsock_fd;
    psock_fd_u64<<=32;
    psock_fd_u64=psock_fd_u64|lpsock_fd;
    int * psock_fd=(int *)psock_fd_u64;

    /* 完成接收发送任务(代码待填) */
    while(1)
    {
        if(psock_fd == &new_fd1)
        {
            ucServerFlag = 1;
            semTake(server_sem1, WAIT_FOREVER);
        }
        else if(psock_fd == &new_fd2)
        {
            ucServerFlag = 2;
            semTake(server_sem2, WAIT_FOREVER);
        }
        else if(psock_fd == &new_fdrec)
        {
            ucServerFlag = 10;
            semTake(server_semrec, WAIT_FOREVER);
        }
        else if(psock_fd == &new_fd3)
        {
            ucServerFlag = 3;
            semTake(server_sem3, WAIT_FOREVER);
        }
        else if(psock_fd == &new_fd4)
        {
            ucServerFlag = 4;
            semTake(server_sem4,WAIT_FOREVER);
        }

#ifdef FASTER_OPR_SUPPORT
        else if (psock_fd == &sock_faster_fd)
        {
            semTake(server_sem_faster, WAIT_FOREVER);
        }
#endif

#ifdef PANEL_HMI_SUPPORT
        else if(psock_fd == &ph_new_fast_fd)
        {
            semTake(ph_server_sem_fast,WAIT_FOREVER);
        }
        else if(psock_fd == &ph_new_slow_fd)
        {
            semTake(ph_server_sem_slow,WAIT_FOREVER);
        }
        else if(psock_fd == &ph_new_sockrec_fd)
        {
            semTake(ph_server_sem_rec,WAIT_FOREVER);
        }
#endif
        sock_tmp=*psock_fd;
        while(1)
        {
start:
            rest_lenth=data_lenth-rcv_cnt;
            rcv_number=ReceiveWithTimeout(sock_tmp, ptemp_buffer, rest_lenth, 5000);		/* 接收数据 */
            if(rcv_number <= 0)
            {
                data_lenth=Frame_Head_Lenth;
                rcv_cnt=0;
                goto start_process;
            }

            memcpy(rcv_buffer+rcv_cnt, ptemp_buffer, rcv_number);
            rcv_cnt += rcv_number;		/* 累加接收数据 */

            if(rcv_cnt >= Frame_Head_Lenth)
            {
                /* 大于帧头长度 */
                if((rcv_buffer[0] != Rpt_IntelType)||
                        (rcv_buffer[1] != Rpt_version )||
                        (rcv_buffer[2] != MasterCPU_addr))
                {
                    data_lenth=Frame_Head_Lenth;
                    rcv_cnt=0;
                    goto start;
                }

                data_lenth=U8_TO_U16(rcv_buffer[19], rcv_buffer[18])+21;		/* 可变帧头 */
                if(rcv_cnt<data_lenth)
                    goto start;
                rcv_number=rcv_cnt;
            }
            else
                goto start;

start_process:

            data_lenth=Frame_Head_Lenth;
            rcv_cnt=0;
            if(rcv_number<0)
            {
                /* 出错 或者超时*/
                data_lenth=Frame_Head_Lenth;
                rcv_cnt=0;
                if (rcv_number == -1)
                {

                    uint8_t s_aucPrompt[MESSAGE_MAX_LEN];
                    static uint32_t ulLinkNum = 0;

                    ulLinkNum++;
                    if (ulLinkNum <= MAX_LINK_HALT_NUM)
                    {
                        sprintf(s_aucPrompt, "%d任务因接收出错退出 %d %d.\n", ucServerFlag, rcv_number, g_ComRcvSts);

                        LOG_Write(LOG_KERNEL, s_aucPrompt, NULL);
                    }
                    ulLinkHaltTm = TM_Get_usCnt();

                    /* 错误处理(代码待填) */
                    if(psock_fd==&new_fd4)
                        server4_OK=FALSE;
                    close(sock_tmp);
                    if (*psock_fd == sock_tmp)
                        *psock_fd=-1;

                    if(new_fileID>0)
                    {
                        close(new_fileID);
                        new_fileID=-1;
                    }
                    if(strcmp(writing_filename,""))
                    {
                        Set_FWS(writing_filename,pfplhead,0);
                        strcpy(writing_filename,"");
                    }

                    return;
                }
            }
            else if (rcv_number>0)
            {
                /* 正常 */
                if (psock_fd == &new_fd1)
                {
                    /* 快速socket */
                    temp_val = 1;
                    server3_OK = TRUE;
                    server_hmi_1_OK = TRUE;
                }
                else if (psock_fd == &new_fd2)
                {
                    /* 慢速socket */
                    temp_val = 2;
                    server3_OK = TRUE;
                    server_hmi_2_OK = TRUE;
                }
                else if (psock_fd == &new_fdrec)
                {
                    /* MMI召唤录波文件 */
                    temp_val = 2;
                    server3_OK = TRUE;
                    server_hmi_rec_OK = TRUE;
                }
                else if (psock_fd == &new_fd3)
                {
                    /* 连接sgView的socket，使用前面板的端口，sgView先连接到MMI，MMI再到保护CPU */
                    temp_val = 3;
                    server3_OK = TRUE;
                    server_hmi_3_OK = TRUE;
                }
                else if (psock_fd == &new_fd4)  /* sgView */
                    temp_val = 4;

#ifdef FASTER_OPR_SUPPORT
                else if (psock_fd == &sock_faster_fd)
                {
                    faster_server_OK = TRUE;
                }
#endif

#ifdef PANEL_HMI_SUPPORT
                else if (psock_fd == &ph_new_fast_fd)
                {
                    ph_server_OK = TRUE;
                }
                else if (psock_fd == &ph_new_slow_fd)
                {
                    ph_server_OK = TRUE;
                }
                else if (psock_fd == &ph_new_sockrec_fd)
                {
                    ph_server_OK = TRUE;
                }
#endif
                ReportType = rcv_buffer[4]+(rcv_buffer[5]<<8); /* report type. */

                Current_FrameSeq = U8_TO_U16(rcv_buffer[15], rcv_buffer[14])+1;
                if (Current_FrameSeq == 1) /* the first frame. */
                {
                    if (new_fileID>0)
                    {
                        close(new_fileID);
                        new_fileID = -1;
                    }

                    if (strcmp(writing_filename, ""))
                    {
                        Set_FWS(writing_filename, pfplhead, 0);	/* disable writing protecting. */
                        strcpy(writing_filename, "");
                    }
                    rcved_number = 0;
                    rcvFrameCnt = 0;
                    file_all_lenth = 0;
                }

                Framedatalen = U8_TO_U16(rcv_buffer[19], rcv_buffer[18]);  /* file head, file length, checksum */
                rcvFrameCnt++;  /* begin from 1. */

                /* 操作源 */
                usOpSrc = rcv_buffer[11];

                if (ReportType == 0x0c00)   /* 是写文件的报文 */
                {
                    LOG_Dbg_Msg("Current_FrameSeq = %x\n", Current_FrameSeq, 0, 0, 0, 0, 0);
                    LOG_Dbg_Msg("WantRcv_FrameSeq = %x\n", rcvFrameCnt, 0, 0, 0, 0, 0);
                    if (Current_FrameSeq == rcvFrameCnt)
                    {
                        rcved_number += rcv_number;  /* received bytes. */
                        if (Current_FrameSeq == 1)
                        {
                            /* the first frame. */
                            LOG_Dbg_Msg("start write a file to disk.\n", 0, 0, 0, 0, 0, 0);
                            if (strcmp(writing_filename, ""))
                            {
                                /* real file name. */
                                Set_FWS(writing_filename, pfplhead, 0);  /* disable writing protecting. */
                                strcpy(writing_filename, "");
                            }
                            LastCRC = 0;
                            strcpy(new_tmp_filename, "");
                            filenamelen = U8_TO_U16(rcv_buffer[21], rcv_buffer[20]);
                            strcpy(filename, "");
                            memcpy(filename, rcv_buffer+Frame_Head_Lenth+2, filenamelen); /* 文件名 */
                            filename[filenamelen] = '\0';

                            /* 禁止写MMI和sgView写EP_CT_RATIO_FILE文件
                             */
                            if (strcmp(filename, EP_CT_RATIO_FILE) == 0)
                            {
                                /* 错误处理，代码待填 */
                                rcved_number = 0;
                                rcvFrameCnt = 0;
                                file_all_lenth = 0;
                                /* 发写入出错报文 */
                                p = p_send_buffer;
                                *p++ = LO8(filenamelen);
                                *p++ = HI8(filenamelen);
                                memcpy(p, filename, filenamelen);
                                p += filenamelen;
                                *p++ = 0x00;
                                *p++ = 0x00;
                                *p++ = 0x04;	/* 文件写入失败 */
                                *p++ = 0x00;
                                reply(*psock_fd, p_send_buffer, rcv_buffer, 0x9400, p-p_send_buffer);
                                LOG_Dbg_Msg("can't create file %s.\n", (int)new_tmp_filename, 0, 0, 0, 0, 0);
                            }

                            strcpy(writing_filename, filename);		/* operated file name. */

                            if (!Get_FWS(filename, pfplhead))
                            {
                                /* 设置文件写保护 */
                                Set_FWS(filename, pfplhead, 1);
                                Bak_File_Name(new_tmp_filename,filename,".bak");
                                memcpy(ext_filename, rcv_buffer+Frame_Head_Lenth+2+filenamelen-3, 3);	/* 扩展名 */
                                ext_filename[3] = '\0';
                                file_head_lenth = 2+2+4+filenamelen; /* file head length. */
                                filelenth = U8_TO_U32(rcv_buffer[9], rcv_buffer[8], rcv_buffer[7],
                                                      rcv_buffer[6])-file_head_lenth-2;    /* file length. */

                                FrameNum = U8_TO_U16(rcv_buffer[13], rcv_buffer[12]); /* frame number. */

                                rpt_lenth = U8_TO_U32(rcv_buffer[9], rcv_buffer[8], rcv_buffer[7],
                                                      rcv_buffer[6]) + U8_TO_U16(rcv_buffer[13], rcv_buffer[12])*21;  /* report total length. */

                                if (rcved_number<rpt_lenth)
                                {
                                    /* more frame, not the final frame. */
                                    /* 建立一个临时文件，返回该临时文件句柄 */
                                    if ((new_fileID = creat(new_tmp_filename, 2))<0)
                                    {
                                        /* 错误处理，代码待填 */
                                        rcved_number = 0;
                                        rcvFrameCnt = 0;
                                        file_all_lenth = 0;
                                        /* 发写入出错报文 */
                                        p = p_send_buffer;
                                        *p++ = LO8(filenamelen);
                                        *p++ = HI8(filenamelen);
                                        memcpy(p, filename, filenamelen);
                                        p += filenamelen;
                                        *p++ = 0x00;
                                        *p++ = 0x00;
                                        *p++ = 0x04;	/* 文件写入失败 */
                                        *p++ = 0x00;
                                        reply(sock_tmp, p_send_buffer, rcv_buffer, 0x9400, p-p_send_buffer);
                                        LOG_Dbg_Msg("can't create file %s.\n", (int)new_tmp_filename, 0, 0, 0, 0, 0);
                                        Set_FWS(filename,pfplhead,0);
                                    }
                                    else
                                    {
                                        uint16_t fileWriteLength;  /* 要写入的文件长度 */

                                        if ((Framedatalen - file_head_lenth) > filelenth )
                                        {
                                            fileWriteLength = filelenth;
                                            fileCRC_L = rcv_buffer[20 + Framedatalen - 1]; /* the last. */
                                        }
                                        else
                                        {
                                            fileWriteLength = Framedatalen-file_head_lenth;
                                        }

                                        /* 将收到的数据追加写入临时文件 */
                                        rtcode = write(new_fileID, &rcv_buffer[Frame_Head_Lenth+file_head_lenth],
                                                       fileWriteLength);
                                        if (rtcode != fileWriteLength)
                                            LOG_Dbg_Msg("When write file an error has occurred.\n", 0, 0, 0, 0, 0, 0);
                                        file_all_lenth += fileWriteLength;
                                        LastCRC = EP_CCITT_CRC16(&rcv_buffer[20+file_head_lenth],
                                                                 fileWriteLength, LastCRC);    /* file data. */
                                    }
                                }
                                else  /* 第一帧收完，是一个短文件 */
                                {
                                    /* only one frame. */
                                    if ((new_fileID = creat(new_tmp_filename, 2))<0)
                                    {
                                        /* 错误处理，代码待填 */
                                        rcved_number = 0;
                                        rcvFrameCnt = 0;
                                        file_all_lenth = 0;
                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0x04);
                                        /* 发写入出错报文 */
                                        LOG_Dbg_Msg("can't create file %s.\n", (int)new_tmp_filename, 0, 0, 0, 0, 0);
                                        Set_FWS(filename, pfplhead, 0);
                                    }
                                    else
                                    {
                                        rtcode = write(new_fileID, &rcv_buffer[20+file_head_lenth],
                                                       Framedatalen-file_head_lenth-2);
                                        if (rtcode != (Framedatalen-file_head_lenth-2))
                                            LOG_Dbg_Msg("When write file an error has occurred.\n", 0, 0, 0, 0, 0, 0);
                                        file_all_lenth = Framedatalen-file_head_lenth-2;
                                        close(new_fileID);
                                        new_fileID = -1;
                                        LastCRC = EP_CCITT_CRC16(&rcv_buffer[20+file_head_lenth],
                                                                 Framedatalen-file_head_lenth-2, LastCRC);  /* only file. */

                                        /* get the CRC directly. */
                                        if (LastCRC != U8_TO_U16(rcv_buffer[20+Framedatalen-1],
                                                                 rcv_buffer[20+Framedatalen-2]))
                                        {
                                            LOG_Dbg_Msg("end write a file to disk but CRC error.\n", 0, 0, 0, 0, 0, 0);
                                            remove(new_tmp_filename);
                                            Set_FWS(filename, pfplhead, 0);
                                            rcved_number = 0;
                                            rcvFrameCnt = 0;
                                            /* 发写入出错报文 */
                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0x00);
                                        }
                                        else   /* 文件校验码正确 */
                                        {
                                            LOG_Dbg_Msg("end write a file to disk\n",0,0,0,0,0,0);
                                            fp=open(new_tmp_filename,0,0);

                                            /* 打开有效 */
                                            if (fp > 0)
                                            {
                                                temp_val1=lseek(fp,0,0);
                                                file_lenth=lseek(fp,0,2);
                                                file_lenth=file_lenth-temp_val1;
                                                temp_val1=lseek(fp,0,0);
                                                close(fp);
                                                if(filelenth==file_lenth)
                                                {
                                                    if(strcmp(ext_filename,"idz")==0)
                                                    {
                                                        fp=open(new_tmp_filename,0,0);
                                                        if(fp>0)
                                                        {
                                                            if((!(uiEdpStatus_g & HW_TEST_MODE))&&/* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */ (SC_Inner_Set_Is_Valid(fp)==EP_SUCCESS) )
                                                            {
                                                                /* Do not execute if fault happenning. */

                                                                close(fp);

                                                                /* 等待校验完成 */
                                                                opt_success = 1;
                                                                if (pParaCheckFun)
                                                                {
                                                                    if (pParaCheckFun() == FALSE)
                                                                    {
                                                                        SC_Reset_Inner_Set();
                                                                        opt_success = 0;
                                                                    }
                                                                }

                                                                if (opt_success == 1)
                                                                {
                                                                    if(FT_Is_File(filename))
                                                                    {
                                                                        strcpy(back_filename,"");
                                                                        Bak_File_Name(back_filename,filename,".old");
                                                                        if(FT_Is_File(back_filename))
                                                                            remove(back_filename);
                                                                        if(rename(filename,back_filename)!=OK)
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                        else
                                                                        {
                                                                            if(rename(new_tmp_filename,filename)==OK)
                                                                            {
                                                                                fp=open(filename,0,0);
                                                                                if(SC_Chg_Mem_Inner_Set(fp)==EP_SUCCESS)
                                                                                {
                                                                                    close(fp);
                                                                                    Write_Nbset_CRC();
                                                                                    Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);

                                                                                }
                                                                                else
                                                                                {
                                                                                    close(fp);

                                                                                    if (FT_Is_File(filename))
                                                                                        remove(filename);
                                                                                    rename(back_filename, filename);
                                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer, rcv_buffer, filename, filenamelen, 0xff);
                                                                                }
                                                                                if (FT_Is_File(back_filename))
                                                                                    remove(back_filename);
                                                                            }
                                                                            else
                                                                            {
                                                                                rename(back_filename,filename);
                                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                            }
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        if(rename(new_tmp_filename,filename)==OK)
                                                                        {
                                                                            fp=open(filename,0,0);
                                                                            if(SC_Chg_Mem_Inner_Set(fp)==EP_SUCCESS)
                                                                            {
                                                                                close(fp);
                                                                                Write_Nbset_CRC();
                                                                                Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);

                                                                            }
                                                                            else
                                                                                close(fp);
                                                                        }
                                                                        else
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    remove(new_tmp_filename);
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                close(fp);
                                                                remove(new_tmp_filename);
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                            }
                                                        }

                                                    }
                                                    else if(strcmp(ext_filename,"cdz")==0)
                                                    {
                                                        fp=open(new_tmp_filename,0,0);
                                                        if(fp>0)
                                                        {
                                                            if((!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */ (SC_CK_Is_Valid(fp)==EP_SUCCESS) )
                                                            {
                                                                /* Do not execute if fault happenning. */

                                                                close(fp);

                                                                /* 等待校验完成 */
                                                                opt_success = 1;

                                                                if (pParaCheckFun)
                                                                {
                                                                    if (pParaCheckFun() == FALSE)
                                                                    {
                                                                        SC_Reset_CK_Set();
                                                                        opt_success = 0;
                                                                    }
                                                                }

                                                                if (opt_success == 1)
                                                                {
                                                                    if(FT_Is_File(filename))
                                                                    {
                                                                        strcpy(back_filename,"");
                                                                        Bak_File_Name(back_filename,filename,".old");
                                                                        if(FT_Is_File(back_filename))
                                                                            remove(back_filename);
                                                                        vxsts = semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                        assert(vxsts==OK);

                                                                        Set_Ckset_Wr_Sts(1); /* 测控定值正在写入 */

                                                                        if(rename(filename,back_filename)!=OK)
                                                                        {
                                                                            vxsts=semGive(semCkCRCIni_g);
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                        }
                                                                        else
                                                                        {
                                                                            if(rename(new_tmp_filename,filename)==OK)
                                                                            {
                                                                                fp=open(filename,0,0);
                                                                                if(SC_Chg_Mem_CK_Set(fp)==EP_SUCCESS)
                                                                                {
                                                                                    close(fp);
                                                                                    if(bulRelayTaskHasAutoSet_g==TRUE)
                                                                                    {
                                                                                        SI_Wr_New_CK_Set();
                                                                                        vxsts=semGive(semCkCRCIni_g);
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        /* 原为5s */
                                                                                        /*vxsts = semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                                        assert(vxsts==OK);*/
                                                                                        Write_Ckset_CRC();
                                                                                        Set_Ckset_Wr_Sts(0); /* 测控定值写入结束 */
                                                                                        vxsts=semGive(semCkCRCIni_g);

                                                                                    }
                                                                                    RE_SetLogSetChgCnt();
                                                                                    Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                                }
                                                                                else
                                                                                {
                                                                                    close(fp);
                                                                                    if (FT_Is_File(filename))
                                                                                        remove(filename);
                                                                                    rename(back_filename, filename);
                                                                                    vxsts=semGive(semCkCRCIni_g);
                                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer, rcv_buffer, filename, filenamelen, 0xff);
                                                                                }

                                                                                if (FT_Is_File(back_filename))
                                                                                    remove(back_filename);
                                                                            }
                                                                            else
                                                                            {
                                                                                rename(back_filename,filename);
                                                                                vxsts=semGive(semCkCRCIni_g);
                                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                            }
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        vxsts = semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                        assert(vxsts==OK);

                                                                        Set_Ckset_Wr_Sts(1); /* 测控定值正在写入 */
                                                                        if(rename(new_tmp_filename,filename)==OK)
                                                                        {
                                                                            fp=open(filename,0,0);
                                                                            if(SC_Chg_Mem_CK_Set(fp)==EP_SUCCESS)
                                                                            {
                                                                                close(fp);
                                                                                if(bulRelayTaskHasAutoSet_g==TRUE)
                                                                                {
                                                                                    SI_Wr_New_CK_Set();
                                                                                    vxsts=semGive(semCkCRCIni_g);
                                                                                }
                                                                                else
                                                                                {
                                                                                    /* 原为5s */
                                                                                    /*vxsts = semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                                    assert(vxsts==OK);*/
                                                                                    Write_Ckset_CRC();
                                                                                    Set_Ckset_Wr_Sts(0); /* 测控定值写入结束 */
                                                                                    vxsts=semGive(semCkCRCIni_g);
                                                                                }
                                                                                RE_SetLogSetChgCnt();
                                                                                Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                            }
                                                                            else
                                                                            {
                                                                                close(fp);
                                                                                vxsts=semGive(semCkCRCIni_g);
                                                                            }
                                                                        }
                                                                        else
                                                                        {
                                                                            vxsts=semGive(semCkCRCIni_g);
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                        }
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    remove(new_tmp_filename);
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                close(fp);
                                                                remove(new_tmp_filename);
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                            }
                                                        }

                                                    }
                                                    else if(strcmp(ext_filename,"dza")!=0)
                                                    {
                                                        if(FT_Is_File(filename))
                                                        {
                                                            strcpy(back_filename,"");
                                                            Bak_File_Name(back_filename,filename,".old");

                                                            if(FT_Is_File(back_filename))
                                                                remove(back_filename);
                                                            if(rename(filename,back_filename)!=OK)
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);

                                                            else
                                                            {
                                                                if(rename(new_tmp_filename,filename)==OK)
                                                                {
                                                                    remove(back_filename);
                                                                    if(strcmp(filename,EP_AI_GAIN_FILE)==0)
                                                                    {
                                                                        ulCrc=0;
                                                                        ulCrc =FT_File_CRC16(EP_AI_GAIN_FILE);
                                                                        FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
                                                                    }
                                                                    if(strcmp(filename,EP_CL_GAIN_FILE)==0)
                                                                    {
                                                                        ulCrc=0;
                                                                        ulCrc =FT_File_CRC16(EP_CL_GAIN_FILE);
                                                                        FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
                                                                    }
                                                                    Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                }
                                                                else
                                                                {
                                                                    rename(back_filename,filename);
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            if(rename(new_tmp_filename,filename)==OK)
                                                            {
                                                                if(strcmp(filename,EP_AI_GAIN_FILE)==0)
                                                                {
                                                                    ulCrc=0;
                                                                    ulCrc =FT_File_CRC16(EP_AI_GAIN_FILE);
                                                                    FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
                                                                }
                                                                if(strcmp(filename,EP_CL_GAIN_FILE)==0)
                                                                {
                                                                    ulCrc=0;
                                                                    ulCrc =FT_File_CRC16(EP_CL_GAIN_FILE);
                                                                    FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
                                                                }
                                                                Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                            }
                                                            else
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        fp=open(new_tmp_filename,0,0);
                                                        if(fp>0)
                                                        {

                                                            if((!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */ (SC_Is_Valid_Set(fp)))
                                                            {
                                                                close(fp);

                                                                /* 参数校验 */
                                                                if (pParaCheckFun)
                                                                {
                                                                    if (pParaCheckFun())
                                                                    {
                                                                        Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                    }
                                                                    else
                                                                    {
                                                                        SC_Reset_Set();

                                                                        remove(new_tmp_filename);
                                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                close(fp);
                                                                remove(new_tmp_filename);
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                            }

                                                        }

                                                    }
                                                }
                                                else
                                                {
                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                }
                                            }
                                            else
                                            {
                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                            }
                                        }

                                        rcved_number=0;
                                        rcvFrameCnt=0;
                                        file_all_lenth=0;
                                        Set_FWS(filename,pfplhead,0);
                                    }
                                }
                            }
                        }
                        else                /*第一帧未收完，是一个长文件*/
                        {
                            if (rcved_number >= rpt_lenth)	/* 是最后一帧 */
                            {
                                uint16_t fileWriteLength;  /* 要写入的文件长度 */
                                uint16_t fileCRC; /* file CRC. */

                                if (Framedatalen == 1)
                                {
                                    /* only high CRC. */
                                    fileCRC_H = rcv_buffer[20 + Framedatalen - 1];
                                    fileCRC = U8_TO_U16(fileCRC_H, fileCRC_L);
                                    fileWriteLength = 0;
                                }
                                else if (Framedatalen == 2)
                                {
                                    /* all CRC. */
                                    fileCRC = U8_TO_U16(rcv_buffer[20+Framedatalen-1], rcv_buffer[20+Framedatalen-2]);
                                    fileWriteLength = 0;
                                }
                                else
                                {
                                    fileWriteLength = Framedatalen - 2;
                                    fileCRC = U8_TO_U16(rcv_buffer[20+Framedatalen-1], rcv_buffer[20+Framedatalen-2]);
                                }

                                rtcode = write(new_fileID, rcv_buffer+Frame_Head_Lenth, fileWriteLength);

                                if (rtcode != fileWriteLength)
                                    LOG_Dbg_Msg("When write file an error has occurred.\n", 0, 0, 0, 0, 0, 0);
                                file_all_lenth += fileWriteLength;
                                close(new_fileID);
                                new_fileID = -1;
                                LastCRC = EP_CCITT_CRC16(rcv_buffer+20, fileWriteLength, LastCRC);

                                if (LastCRC != fileCRC)
                                {
                                    LOG_Dbg_Msg("end write a file to disk but CRC error.\n", 0, 0, 0, 0, 0, 0);
                                    remove(new_tmp_filename);
                                    rcved_number = 0;
                                    rcvFrameCnt = 0;
                                    file_all_lenth = 0;
                                    Set_FWS(filename, pfplhead, 0);
                                    /* 发写入出错报文 */
                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0x00);
                                }
                                else    /* 文件校验码正确 */
                                {
                                    LOG_Dbg_Msg("end write a file to disk\n",0,0,0,0,0,0);
                                    fp=open(new_tmp_filename,0,0);

                                    /* 打开失败则返回 */
                                    if (fp>0)
                                    {
                                        temp_val1=lseek(fp,0,0);
                                        file_lenth=lseek(fp,0,2);
                                        file_lenth=file_lenth-temp_val1;
                                        temp_val1=lseek(fp,0,0);
                                        close(fp);
                                        if(filelenth==file_lenth)
                                        {
                                            if(strcmp(ext_filename,"idz")==0)
                                            {

                                                fp=open(new_tmp_filename,0,0);
                                                if(fp>0)
                                                {
                                                    if((!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */ (SC_Inner_Set_Is_Valid(fp)==EP_SUCCESS) )
                                                    {
                                                        /* Do not execute if fault happenning. */

                                                        close(fp);

                                                        /* 参数校验 */
                                                        opt_success = 1;

                                                        if (pParaCheckFun)
                                                        {
                                                            if (pParaCheckFun() == FALSE)
                                                            {
                                                                SC_Reset_Inner_Set();
                                                                opt_success = 0;
                                                            }
                                                        }
                                                        if (opt_success == 1)
                                                        {
                                                            if(FT_Is_File(filename))
                                                            {
                                                                strcpy(back_filename,"");
                                                                Bak_File_Name(back_filename,filename,".old");
                                                                if(FT_Is_File(back_filename))
                                                                    remove(back_filename);
                                                                if(rename(filename,back_filename)!=OK)
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                else
                                                                {
                                                                    if(rename(new_tmp_filename,filename)==OK)
                                                                    {
                                                                        fp=open(filename,0,0);
                                                                        if(SC_Chg_Mem_Inner_Set(fp)==EP_SUCCESS)
                                                                        {
                                                                            close(fp);
                                                                            Write_Nbset_CRC();
                                                                            Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);

                                                                        }
                                                                        else
                                                                        {
                                                                            close(fp);
                                                                            if (FT_Is_File(filename))
                                                                                remove(filename);
                                                                            rename(back_filename, filename);
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer, rcv_buffer, filename, filenamelen, 0xff);
                                                                        }

                                                                        if (FT_Is_File(back_filename))
                                                                            remove(back_filename);
                                                                    }
                                                                    else
                                                                    {
                                                                        rename(back_filename,filename);
                                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                    }
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if(rename(new_tmp_filename,filename)==OK)
                                                                {
                                                                    fp=open(filename,0,0);
                                                                    if(SC_Chg_Mem_Inner_Set(fp)==EP_SUCCESS)
                                                                    {
                                                                        close(fp);
                                                                        Write_Nbset_CRC();
                                                                        Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);

                                                                    }
                                                                    else
                                                                        close(fp);
                                                                }
                                                                else
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                            }
                                                        }
                                                        else
                                                        {
                                                            remove(new_tmp_filename);
                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        close(fp);
                                                        remove(new_tmp_filename);
                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                    }
                                                }

                                            }
                                            else if(strcmp(ext_filename,"cdz")==0)
                                            {
                                                fp=open(new_tmp_filename,0,0);
                                                if(fp>0)
                                                {
                                                    if((!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */ (SC_CK_Is_Valid(fp)==EP_SUCCESS) )
                                                    {
                                                        /* Do not execute if fault happenning. */

                                                        close(fp);

                                                        /* 参数校验 */
                                                        opt_success = 1;

                                                        if (pParaCheckFun)
                                                        {
                                                            if (pParaCheckFun() == FALSE)
                                                            {
                                                                SC_Reset_CK_Set();
                                                                opt_success = 0;
                                                            }
                                                        }

                                                        if (opt_success == 1)
                                                        {
                                                            if(FT_Is_File(filename))
                                                            {
                                                                strcpy(back_filename,"");
                                                                Bak_File_Name(back_filename,filename,".old");
                                                                if(FT_Is_File(back_filename))
                                                                    remove(back_filename);
                                                                vxsts=semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                assert(vxsts==OK);
                                                                if(rename(filename,back_filename)!=OK)
                                                                {
                                                                    vxsts=semGive(semCkCRCIni_g);
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                }
                                                                else
                                                                {
                                                                    Set_Ckset_Wr_Sts(1); /* 测控定值正在写入 */
                                                                    if(rename(new_tmp_filename,filename)==OK)
                                                                    {
                                                                        fp=open(filename,0,0);
                                                                        if(SC_Chg_Mem_CK_Set(fp)==EP_SUCCESS)
                                                                        {
                                                                            close(fp);
                                                                            if(bulRelayTaskHasAutoSet_g==TRUE)
                                                                            {
                                                                                SI_Wr_New_CK_Set();
                                                                                vxsts=semGive(semCkCRCIni_g);
                                                                            }
                                                                            else
                                                                            {
                                                                                /*vxsts=semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                                assert(vxsts==OK);*/
                                                                                Write_Ckset_CRC();
                                                                                Set_Ckset_Wr_Sts(0); /* 测控定值写入结束 */
                                                                                vxsts=semGive(semCkCRCIni_g);
                                                                            }
                                                                            RE_SetLogSetChgCnt();
                                                                            Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                        }
                                                                        else
                                                                        {
                                                                            close(fp);
                                                                            if (FT_Is_File(filename))
                                                                                remove(filename);
                                                                            rename(back_filename, filename);
                                                                            vxsts=semGive(semCkCRCIni_g);
                                                                            Send_Download_File_Err(sock_tmp, p_send_buffer, rcv_buffer, filename, filenamelen, 0xff);
                                                                        }
                                                                        if(FT_Is_File(back_filename))
                                                                            remove(back_filename);
                                                                    }
                                                                    else
                                                                    {
                                                                        rename(back_filename,filename);
                                                                        vxsts=semGive(semCkCRCIni_g);
                                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                    }
                                                                }
                                                            }
                                                            else
                                                            {
                                                                vxsts=semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                assert(vxsts==OK);

                                                                Set_Ckset_Wr_Sts(1); /* 测控定值正在写入 */
                                                                if(rename(new_tmp_filename,filename)==OK)
                                                                {
                                                                    fp=open(filename,0,0);
                                                                    if(SC_Chg_Mem_CK_Set(fp)==EP_SUCCESS)
                                                                    {
                                                                        close(fp);
                                                                        if(bulRelayTaskHasAutoSet_g==TRUE)
                                                                        {
                                                                            SI_Wr_New_CK_Set();
                                                                            vxsts=semGive(semCkCRCIni_g);
                                                                        }
                                                                        else
                                                                        {
                                                                            /*vxsts=semTake(semCkCRCIni_g, WAIT_FOREVER);
                                                                            assert(vxsts==OK);*/
                                                                            Write_Ckset_CRC();
                                                                            Set_Ckset_Wr_Sts(0); /* 测控定值写入结束 */
                                                                            vxsts=semGive(semCkCRCIni_g);
                                                                        }
                                                                        RE_SetLogSetChgCnt();
                                                                        Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                                    }
                                                                    else
                                                                    {
                                                                        close(fp);
                                                                        vxsts=semGive(semCkCRCIni_g);
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    vxsts=semGive(semCkCRCIni_g);
                                                                    Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            remove(new_tmp_filename);
                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        close(fp);
                                                        remove(new_tmp_filename);
                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                    }
                                                }

                                            }

                                            else if(strcmp(ext_filename,"dza")!=0)
                                            {
                                                if(FT_Is_File(filename))
                                                {
                                                    strcpy(back_filename,"");
                                                    Bak_File_Name(back_filename,filename,".old");

                                                    if(FT_Is_File(back_filename))
                                                        remove(back_filename);
                                                    if(rename(filename,back_filename)!=OK)
                                                    {
                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                    }
                                                    else
                                                    {
                                                        if(rename(new_tmp_filename,filename)==OK)
                                                        {
                                                            remove(back_filename);
                                                            if(strcmp(filename,EP_AI_GAIN_FILE)==0)
                                                            {
                                                                ulCrc=0;
                                                                ulCrc =FT_File_CRC16(EP_AI_GAIN_FILE);
                                                                FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
                                                            }
                                                            if(strcmp(filename,EP_CL_GAIN_FILE)==0)
                                                            {
                                                                ulCrc=0;
                                                                ulCrc =FT_File_CRC16(EP_CL_GAIN_FILE);
                                                                FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
                                                            }
                                                            Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                        }
                                                        else
                                                        {
                                                            rename(back_filename,filename);
                                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    if(rename(new_tmp_filename,filename)==OK)
                                                    {
                                                        if(strcmp(filename,EP_AI_GAIN_FILE)==0)
                                                        {
                                                            ulCrc=0;
                                                            ulCrc =FT_File_CRC16(EP_AI_GAIN_FILE);
                                                            FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
                                                        }
                                                        if(strcmp(filename,EP_CL_GAIN_FILE)==0)
                                                        {
                                                            ulCrc=0;
                                                            ulCrc =FT_File_CRC16(EP_CL_GAIN_FILE);
                                                            FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
                                                        }
                                                        Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                    }
                                                    else
                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                }
                                            }
                                            else
                                            {
                                                fp=open(new_tmp_filename,0,0);
                                                if(fp>0)
                                                {

                                                    if((!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g))&& */(SC_Is_Valid_Set(fp)))
                                                    {
                                                        close(fp);

                                                        /* 参数校验 */
                                                        if (pParaCheckFun)
                                                        {
                                                            if (pParaCheckFun())
                                                            {
                                                                Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                            }
                                                            else
                                                            {
                                                                SC_Reset_Set();

                                                                remove(new_tmp_filename);
                                                                Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                            }
                                                        }
                                                        else
                                                        {
                                                            Send_Download_File_OK(sock_tmp, p_send_buffer,rcv_buffer,filenamelen,filename,file_all_lenth);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        close(fp);
                                                        remove(new_tmp_filename);
                                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                                    }

                                                }

                                            }

                                        }
                                        else
                                        {
                                            LOG_Dbg_Msg("filelenth is %d,file_lenth is %d!\n",filelenth,file_lenth,0,0,0,0);
                                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                        }
                                    }
                                    else
                                    {
                                        /* 文件打不开则返回失败 */
                                        LOG_Dbg_Msg("file_lenth is %d!\n",file_lenth,0,0,0,0,0);
                                        Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                                    }
                                }
                                rcved_number=0;
                                rcvFrameCnt=0;
                                file_all_lenth=0;
                                Set_FWS(filename,pfplhead,0);
                            }
                            else if (Current_FrameSeq == FrameNum-1)
                            {
                                /* 倒数第二帧 */
                                uint16_t fileWriteLength;  /* 要写入的文件长度 */

                                if (Framedatalen > filelenth - file_all_lenth)
                                {
                                    /* 当前帧长度大于剩余文件字节数 */
                                    fileWriteLength = filelenth - file_all_lenth;

                                    if ((Framedatalen-fileWriteLength) == 1)
                                    {
                                        /* 多一个字节 */
                                        fileCRC_L = rcv_buffer[20 + Framedatalen - 1];
                                    }
                                }
                                else
                                {
                                    fileWriteLength = Framedatalen;
                                }

                                rtcode = write(new_fileID, rcv_buffer+20, fileWriteLength);
                                if (rtcode != fileWriteLength)
                                    LOG_Dbg_Msg("When write file an error has occurred.\n", 0, 0, 0, 0, 0, 0);
                                file_all_lenth += fileWriteLength;
                                LastCRC = EP_CCITT_CRC16(rcv_buffer+20, fileWriteLength, LastCRC);
                            }
                            else     /* 中间帧 */
                            {
                                rtcode = write(new_fileID, rcv_buffer+20, Framedatalen);
                                if (rtcode != Framedatalen)
                                    LOG_Dbg_Msg("When write file an error has occurred.\n", 0, 0, 0, 0, 0, 0);
                                file_all_lenth += Framedatalen;
                                LastCRC = EP_CCITT_CRC16(rcv_buffer+20, Framedatalen, LastCRC);
                            }
                        }
                    }
                    else                    /*接收与发送不同步*/
                    {
                        if(new_fileID>0)
                        {
                            close(new_fileID);
                            new_fileID=-1;
                        }
                        LOG_Dbg_Msg("rcved not syns\n",0,0,0,0,0,0);
                        rcved_number=0;
                        rcvFrameCnt=0;
                        file_all_lenth=0;
                        /*读空网络缓冲区，待定*/
                        Set_FWS(filename,pfplhead,0);
                        if(Current_FrameSeq==(U8_TO_U16(rcv_buffer[13],rcv_buffer[12])))
                        {
                            Send_Download_File_Err(sock_tmp, p_send_buffer,rcv_buffer,filename,filenamelen,0xff);
                        }
                    }
                }
                else                        /*不是写文件的报文*/
                {
                    if(new_fileID>0)
                    {
                        close(new_fileID);
                        new_fileID=-1;
                    }
                    if(strcmp(writing_filename,""))
                    {
                        Set_FWS(writing_filename,pfplhead,0);
                        strcpy(writing_filename,"");
                    }
                    rcved_number=0;
                    rcvFrameCnt=0;
                    file_all_lenth=0;
                    switch(ReportType)
                    {
                        case 0x0810:                    /*遥控命令*/

                            if(rcv_buffer[20]==0)     /*预发,但不执行*/
                            {
                                opt_success=0;
                                switch(rcv_buffer[21])
                                {
                                    case 0x00:
                                        sprintf(aucBuf,EP_SET_AREA_DIR "/area%02x.dza",rcv_buffer[22]);
                                        if(!Check_Areaset_CRC(aucBuf))
                                        {
                                            LOG_Dbg_Msg("send area error!\n",0,0,0,0,0,0);
                                            break;
                                        }
                                        opt_success=SC_Is_Valid_Area(rcv_buffer[22]);
                                        if(!opt_success/*  || (VI_Is_Fault()&&bSetIsValid_g) */)
                                        {
                                            /* Do not execute if fault happenning. .*/
                                            opt_success=FALSE; /*20131004 sdm 为了让处于故障态时后面不要多发一次成功报文*/
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x0f;
                                            *p++=0x03;
                                            strcpy(error_msg,"The new Set Area is not valid");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        break;

                                    case 0x01:  /*软压板投入退出*/
                                        /*DQ:任何压板模式下都可以进行软压板投退*/
                                        /* 获取当前状态 */
                                        plink = SC_Get_Sw_Link(rcv_buffer[22]);
                                        if(rcv_buffer[22]>=iLinkNum_g)
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x01;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x02;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if ((usOpSrc == OP_MMI_NEW)
                                                 && plink->bSwVal && (rcv_buffer[23] == 0xa5)
                                                 && (plink->bJgCurFlg /* SCI_Exsit_Current(rcv_buffer[22]) */)
                                                )
                                        {
                                            /* 源自MMI新模式, 当前为投入, 为退出操作, 同时有流 */
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x03;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else
                                        {
                                            opt_success=1;
                                        }
                                        break;

                                    case 0x02:  /*遥控点操作*/
                                        LOG_Dbg_Msg("遥控点预发操作!\n", 0, 0, 0, 0, 0, 0);
                                        if(rcv_buffer[22]>=iMeaDoNum_g)
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x01;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5)&&(rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0xaa))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x02;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            uint8_t PtNum=0; 			/* 遥控点号 */
                                            uint8_t OptNum=0; 	/* 遥控操作：0x55，短脉冲动作；0x5A，长脉冲动作；0xA5，返回；0xAA，自定义脉宽时间，且带同期方式参数 */
                                            uint32_t OptPara=0;		/* 遥控操作参数，在遥控操作类型为AA的参数脉宽时间第三个参数 */
                                            uint32_t ulTqPara=0;		/* 遥控操作参数，同期方式 */
                                            uint8_t ucExtFlag=0;  /* 扩展标志 */
                                            uint8_t ucIpAddr[4];

                                            PtNum=rcv_buffer[22];	/* 遥控点号 */
                                            OptNum=rcv_buffer[23];		/* 遥控操作代码 */
                                            /* 保留一个字节 */
                                            ucExtFlag=rcv_buffer[25];		/* 是否有扩展标志 */

                                            if(ucExtFlag)
                                            {
                                                /* 有扩展参数标志 */
                                                if(OptNum == 0xaa)
                                                {
                                                    /* 带有效参数 */
                                                    OptPara=U8_TO_U32(rcv_buffer[29], rcv_buffer[28], rcv_buffer[27], rcv_buffer[26]);
                                                }
                                                ulTqPara=U8_TO_U32(rcv_buffer[33], rcv_buffer[32], rcv_buffer[31], rcv_buffer[30]);

                                                ucIpAddr[0]=rcv_buffer[42];
                                                ucIpAddr[1]=rcv_buffer[43];
                                                ucIpAddr[2]=rcv_buffer[44];
                                                ucIpAddr[3]=rcv_buffer[45];
                                            }

//											if(!(uiEdpStatus_g & ON_FAR_STATE))
//											{	/* 处于就地态 */
//												LOG_Dbg_Msg("就地态，遥控失败!\n", 0, 0, 0, 0, 0, 0);
//                                                p_send_buffer=malloc(Max_Frame_Lenth);
//												assert(p_send_buffer);
//                                                p=p_send_buffer;
//                                                *p++=0;
//                                                *p++=0;
//                                                *p++=0x2f;
//                                                *p++=0x03;
//                                                *p++=0;
//                                                reply(*psock_fd,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
//                                                free(p_send_buffer);
//											}
//											else
                                            {

                                                if(!VI_New_MeaDo(PtNum,OptNum, 0x00, OptPara, ulTqPara)) /* Set beforehand. */
                                                {
                                                    LOG_Dbg_Msg("遥控失败!\n", 0, 0, 0, 0, 0, 0);
                                                    p=p_send_buffer;
                                                    *p++=0;
                                                    *p++=0;
                                                    *p++=0x2f;
                                                    *p++=0x03;
                                                    *p++=0;
                                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                                }
                                                else
                                                {
                                                    LOG_Dbg_Msg("遥控成功!\n", 0, 0, 0, 0, 0, 0);
                                                    opt_success=1;
                                                }
                                            }
                                        }
                                        break;

                                    case 0x04:  /*删除定值区*/
                                        strcpy(delete_filename,"");
                                        sprintf(delete_filename,EP_SET_AREA_DIR "/area%02x.dza",rcv_buffer[22]);
                                        fp=open(delete_filename,O_RDONLY,0);
                                        if(fp>0)
                                        {
                                            close(fp);
                                            strcpy(delete_filename,"");
                                            opt_success=1;
                                        }
                                        else
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x0f;
                                            *p++=0x03;
                                            strcpy(error_msg,"The Set Area you want delete not exit");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        break;

                                    case 0x03:  /*保护进入测试模式*/
                                        if(rcv_buffer[22]!=0x01)
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x01;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x02;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            opt_success=1;
                                        }
                                        break;

                                    case 0x05:  /*DQ: 压板总选择方式*/
                                        LOG_Dbg_Msg("压板总预选择! %02x\n",rcv_buffer[23],0,0,0,0,0);
                                        if(rcv_buffer[22]!=0x01)
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x01;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0x5a)&&
                                                (rcv_buffer[23]!=0xaa)&&(rcv_buffer[23]!=0xa5)&&
                                                (rcv_buffer[23]!=0x59))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x02;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            opt_success=1;
                                        }
                                        break;
                                    case 0x07:  /*DQ: 定制压板模式下,设置各压板工作方式*/
                                        LOG_Dbg_Msg("定制压板预设置! %02x %02x\n",rcv_buffer[22],rcv_buffer[23],0,0,0,0);
                                        SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
                                        if(!(ulTotalLinkMode&LINK_MODE_CUS))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x0f;
                                            *p++=0x03;
                                            strcpy(error_msg,"Current Link mode is not custom");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if(rcv_buffer[22]>=iLinkNum_g)
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x01;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0x5a)&&
                                                (rcv_buffer[23]!=0xaa)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x02;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            opt_success=1;
                                        }
                                        break;

                                    case 0x09:
                                        /* HMI设置CPU告警状态, 目前仅支持程序/配置校验错误 */
                                        if ((rcv_buffer[22] != EV_SOFTWARE_CHECK_ERR)
                                                && (rcv_buffer[22] != MMI_S_AE_MMI_ERR))
                                        {
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x01;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else if ((rcv_buffer[23] != 0x5a) && (rcv_buffer[23] != 0xa5))
                                        {
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x02;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else
                                        {
                                            if (rcv_buffer[22] == EV_SOFTWARE_CHECK_ERR)
                                            {
                                                LOG_Write(LOG_KERNEL, "CID文件错误(预发)!!\n", NULL);
                                            }
                                            else if (rcv_buffer[22] == MMI_S_AE_MMI_ERR)
                                            {
                                                LOG_Write(LOG_KERNEL, "HMI模件异常(预发)!!\n", NULL);
                                            }
                                            g_baHmiSetErr[rcv_buffer[22]] = TRUE;
                                            opt_success = 1;
                                        }
                                        break;

                                    default:
                                        opt_success=0;
                                        p=p_send_buffer;
                                        *p++=0;
                                        *p++=0;
                                        *p++=0x00;
                                        *p++=0x03;
                                        *p++=0;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        break;

                                }

                                if(opt_success)     /*预发遥控成功*/
                                {
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=rcv_buffer[20];
                                    *p++=rcv_buffer[21];
                                    remote_Type=rcv_buffer[21];
                                    *p++=rcv_buffer[22]^0xFF;
                                    remote_ObjCode=rcv_buffer[22];
                                    *p++=rcv_buffer[23];
                                    remote_OprCode=rcv_buffer[23];
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8c00,p-p_send_buffer);
                                }
                                else                    /*预发遥控失败*/
                                {
                                }
                            }
                            if(rcv_buffer[20]==1)     /*正式命令,执行*/
                            {
                                opt_success=-1;
                                switch(rcv_buffer[21])
                                {
                                        int area;
                                    case 0x00:
                                        area = SC_Work_Set_Area();

#if 0
                                        if(VI_Is_Fault()&&bSetIsValid_g)
                                            opt_success = EP_ERROR;
                                        else
#endif

                                            opt_success=SC_Chg_Work_Area(rcv_buffer[22]);
                                        if(opt_success!=EP_SUCCESS)
                                        {
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x2f;
                                            *p++=0x03;
                                            strcpy(error_msg,"When change Set Area file operation error");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                            ChangeSetAreaModifiesToLog(area,rcv_buffer[22], usOpSrc);
                                        break;

                                    case 0x01:  /*DQ:任何压板模式下都可以进行软压板投退*/
                                        if(rcv_buffer[22]>=iLinkNum_g)
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x21;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x22;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            SC_LINK_ITEM * plink = NULL;
                                            SC_LINK_ITEM *plinkWr = NULL; /* 固化压板指针 */

                                            BOOL bOldStats = FALSE;
                                            if(rcv_buffer[23]==0x5a) Opc=TRUE;
                                            else Opc=FALSE;

                                            plink = SC_Get_Sw_Link(rcv_buffer[22]);
                                            plinkWr = SC_Get_Wr_Sw_Link(rcv_buffer[22]);
                                            plinkWr->bSwVal = Opc;

                                            opt_success = EP_SUCCESS;

                                            /* 调用参数校验回调函数 */
                                            if (pParaCheckFun)
                                            {
                                                if (pParaCheckFun() == FALSE)
                                                {
                                                    SC_Reset_Link();
                                                    opt_success = EP_ERROR;
                                                }
                                            }

                                            if (opt_success == EP_SUCCESS)
                                            {
                                                bOldStats = plink->bSwVal;
                                                opt_success = SC_Chg_Sw_Link(rcv_buffer[22],Opc);
                                            }

                                            if(opt_success!=EP_SUCCESS)
                                            {
                                                p=p_send_buffer;
                                                *p++=0;
                                                *p++=0;
                                                *p++=0x2f;
                                                *p++=0x03;
                                                strcpy(error_msg,"When chg soft link file operating failure");
                                                err_msg_len=strlen(error_msg);
                                                *p++=err_msg_len;
                                                memcpy(p,error_msg,err_msg_len);
                                                p+=err_msg_len;
                                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                            }
                                            else
                                            {
                                                SybChangeToLog(plink, bOldStats, Opc, usOpSrc);
                                            }
                                        }
                                        break;

                                    case 0x02:  /*遥控执行*/
                                        LOG_Dbg_Msg("遥控点执行操作!\n", 0, 0, 0, 0, 0, 0);
                                        if(rcv_buffer[22]>=iMeaDoNum_g)
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x21;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5)&&(rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0xaa))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x22;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            uint8_t PtNum=0; 			/* 遥控点号 */
                                            uint8_t OptNum=0; 	/* 遥控操作：0x55，短脉冲动作；0x5A，长脉冲动作；0xA5，返回；0xAA，自定义脉宽时间及同期方式标志 */
                                            uint32_t OptPara=0;		/* 遥控操作参数，在遥控操作类型为AA时参数脉宽时间 */
                                            uint32_t ulTqPara=0;		/* 遥控参数，同期方式 */
                                            uint8_t ucExtFlag=0;		/* 扩展标志 */
                                            uint8_t ucIpAddr[4]= {0,0,0,0};

                                            PtNum=rcv_buffer[22];
                                            OptNum=rcv_buffer[23];		/* 遥控操作代码 */
                                            /* 保留一个字节 */
                                            ucExtFlag=rcv_buffer[25];		/* 是否有扩展标志 */

                                            if(ucExtFlag)
                                            {
                                                /* 有扩展参数标志 */
                                                if(OptNum == 0xaa)
                                                {
                                                    /* 带有效参数 */
                                                    OptPara=U8_TO_U32(rcv_buffer[29], rcv_buffer[28], rcv_buffer[27], rcv_buffer[26]);
                                                }
                                                ulTqPara=U8_TO_U32(rcv_buffer[33], rcv_buffer[32], rcv_buffer[31], rcv_buffer[30]);
                                                ucIpAddr[0]=rcv_buffer[42];
                                                ucIpAddr[1]=rcv_buffer[43];
                                                ucIpAddr[2]=rcv_buffer[44];
                                                ucIpAddr[3]=rcv_buffer[45];
                                            }

//											if(!(uiEdpStatus_g & ON_FAR_STATE))
//											{	/* 处于就地态 */
//												LOG_Dbg_Msg("就地态，遥控失败!\n", 0, 0, 0, 0, 0, 0);
//                                               p_send_buffer=malloc(Max_Frame_Lenth);
//											   assert(p_send_buffer);
//                                                p=p_send_buffer;
//                                                *p++=0;
//                                                *p++=0;
//                                                *p++=0x2f;
//                                                *p++=0x03;
//                                                *p++=0;
//                                                reply(*psock_fd,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
//                                                free(p_send_buffer);
//											}
//											else
                                            {
                                                if(!VI_New_MeaDo(PtNum,OptNum,0x01, OptPara, ulTqPara)) /* Set formally. */
                                                {
                                                    LOG_Dbg_Msg("遥控失败!\n", 0, 0, 0, 0, 0, 0);
                                                    p=p_send_buffer;
                                                    *p++=0;
                                                    *p++=0;
                                                    *p++=0x2f;
                                                    *p++=0x03;
                                                    *p++=0;
                                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                                }
                                                else
                                                {
                                                    VI_New_MeaDoToLog(PtNum,OptNum,OptPara,ulTqPara,ucIpAddr, usOpSrc);
                                                    LOG_Dbg_Msg("遥控成功!\n", 0, 0, 0, 0, 0, 0);
                                                    opt_success=EP_SUCCESS;
                                                }
                                            }
                                        }
                                        break;

                                    case 0x04:
                                        strcpy(delete_filename,"");
                                        sprintf(delete_filename,EP_SET_AREA_DIR "/area%02x.dza",rcv_buffer[22]);
                                        fp=open(delete_filename,O_RDONLY,0);
                                        if(fp>0)
                                        {
                                            close(fp);
                                            opt_success=SC_Del_Set_Area(rcv_buffer[22]);
                                            if(opt_success==EP_PARM_ERR)
                                            {
                                                p=p_send_buffer;
                                                *p++=0;
                                                *p++=0;
                                                *p++=0x22;
                                                *p++=0x03;
                                                *p++=0;
                                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                            }
                                            if(opt_success==EP_SUCCESS)
                                                DeleteSetAreaToLog(rcv_buffer[22], usOpSrc);
                                        }
                                        else
                                        {
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x2f;
                                            *p++=0x03;
                                            strcpy(error_msg,"When delete Set Area file operation error");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        break;

#if 1
                                    case 0x03:
                                        if(rcv_buffer[22]!=0x01)
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x21;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x5a)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x22;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            BOOL bTempStats;
                                            if(uiEdpStatus_g & HW_TEST_MODE)
                                                bTempStats=TRUE;
                                            else
                                                bTempStats=FALSE;
                                            if(rcv_buffer[23]==0x5a)
                                            {
                                                if(!(uiEdpStatus_g & HW_TEST_MODE))
                                                {
                                                    EP_Bgn_Hw_Test();
                                                    SIO_Disable_Alm();
                                                    bAlmDoIsForced = FALSE;
                                                }
                                            }
                                            else if(rcv_buffer[23]==0xa5)
                                            {

                                                if(uiEdpStatus_g & HW_TEST_MODE)
                                                {
                                                    LOG_Dbg_Msg("End Test Mode\n",0,0,0,0,0,0);
                                                    EP_End_Hw_Test();
                                                }
                                            }
                                            DebugRunSwitchToLog(rcv_buffer[23],bTempStats, usOpSrc);
                                            opt_success=EP_SUCCESS;
                                        }
                                        break;
#endif

                                    case 0x05:  /*DQ: 压板总选择方式*/
                                        LOG_Dbg_Msg("压板总切换!\n",0,0,0,0,0,0);
                                        if(rcv_buffer[22]!=0x01)
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x21;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0x5a)&&
                                                (rcv_buffer[23]!=0xaa)&&(rcv_buffer[23]!=0xa5)&&
                                                (rcv_buffer[23]!=0x59))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x22;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            uint16_t ulTotalLinkMode;
                                            switch(rcv_buffer[23])
                                            {
                                                case 0x5a:
                                                    OptSts=LINK_MODE_HW;
                                                    break;
                                                case 0x55:
                                                    OptSts=LINK_MODE_SW;
                                                    break;
                                                case 0xa5:
                                                    OptSts=LINK_MODE_AND;
                                                    break;
                                                case 0xaa:
                                                    OptSts=LINK_MODE_OR;
                                                    break;
                                                case 0x59:
                                                    OptSts=LINK_MODE_CUS;
                                                    break;
                                                default:
                                                    opt_success = EP_ERROR;
                                                    /* assert(0); */
                                                    break;
                                            }

#if 0
                                            if(VI_Is_Fault()&&bSetIsValid_g)
                                                opt_success = EP_ERROR;
                                            else
#endif

                                            {

                                                SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
                                                opt_success = SC_Chg_Link_Mode_File(0,OptSts,TRUE);
                                            }
                                            if(opt_success!=EP_SUCCESS)
                                            {
                                                p=p_send_buffer;
                                                *p++=0;
                                                *p++=0;
                                                *p++=0x2f;
                                                *p++=0x03;
                                                strcpy(error_msg,"When chg link mode file operating failure");
                                                err_msg_len=strlen(error_msg);
                                                *p++=err_msg_len;
                                                memcpy(p,error_msg,err_msg_len);
                                                p+=err_msg_len;
                                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                                LOG_Dbg_Msg("压板总切换失败!\n",0,0,0,0,0,0);
                                            }
                                            else
                                            {

                                                YBTotalToLog(ulTotalLinkMode,OptSts, usOpSrc);

                                            }
                                        }
                                        break;

                                    case 0x07:  /*DQ: 定制压板模式下,设置各压板工作方式*/
                                        LOG_Dbg_Msg("定制压板设置!\n",0,0,0,0,0,0);
                                        SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
                                        if(!(ulTotalLinkMode&LINK_MODE_CUS))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x2f;
                                            *p++=0x03;
                                            strcpy(error_msg,"Current Link mode is not custom");
                                            err_msg_len=strlen(error_msg);
                                            *p++=err_msg_len;
                                            memcpy(p,error_msg,err_msg_len);
                                            p+=err_msg_len;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if(rcv_buffer[22]>=iLinkNum_g)
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x21;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else if((rcv_buffer[23]!=0x55)&&(rcv_buffer[23]!=0x5a)&&
                                                (rcv_buffer[23]!=0xaa)&&(rcv_buffer[23]!=0xa5))
                                        {
                                            opt_success=EP_ERROR;
                                            p=p_send_buffer;
                                            *p++=0;
                                            *p++=0;
                                            *p++=0x22;
                                            *p++=0x03;
                                            *p++=0;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                        else
                                        {
                                            switch(rcv_buffer[23])
                                            {
                                                case 0x5a:
                                                    OptSts=LINK_MODE_HW;
                                                    break;
                                                case 0x55:
                                                    OptSts=LINK_MODE_SW;
                                                    break;
                                                case 0xa5:
                                                    OptSts=LINK_MODE_AND;
                                                    break;
                                                case 0xaa:
                                                    OptSts=LINK_MODE_OR;
                                                    break;
                                                default:
                                                    opt_success = EP_ERROR;
                                                    /* assert(0); */
                                                    break;
                                            }

#if 0
                                            if(VI_Is_Fault()&&bSetIsValid_g)
                                                opt_success = EP_ERROR;
                                            else
#endif

                                                opt_success = SC_Chg_Link_Mode_File(rcv_buffer[22],OptSts,FALSE);
                                            if(opt_success!=EP_SUCCESS)
                                            {
                                                p=p_send_buffer;
                                                *p++=0;
                                                *p++=0;
                                                *p++=0x2f;
                                                *p++=0x03;
                                                strcpy(error_msg,"When chg link mode file operating failure");
                                                err_msg_len=strlen(error_msg);
                                                *p++=err_msg_len;
                                                memcpy(p,error_msg,err_msg_len);
                                                p+=err_msg_len;
                                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                                LOG_Dbg_Msg("定制压板设置失败!\n",0,0,0,0,0,0);
                                            }

                                        }
                                        break;

                                    case 0x09:
                                        /* HMI设置CPU告警状态, 目前仅支持程序/配置校验错误 */
                                        if ((rcv_buffer[22] != EV_SOFTWARE_CHECK_ERR)
                                                && (rcv_buffer[22] != MMI_S_AE_MMI_ERR))
                                        {
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x21;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else if ((rcv_buffer[23] != 0x5a) && (rcv_buffer[23] != 0xa5))
                                        {
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x22;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else if (g_baHmiSetErr[rcv_buffer[22]] != TRUE)
                                        {
                                            opt_success = 0;
                                            p = p_send_buffer;
                                            *p++ = 0;
                                            *p++ = 0;
                                            *p++ = 0x23;
                                            *p++ = 0x03;
                                            *p++ = 0;
                                            reply(sock_tmp, p_send_buffer, rcv_buffer, 0x8e00, p-p_send_buffer);
                                        }
                                        else
                                        {
                                            g_baHmiSetErr[rcv_buffer[22]] = FALSE;

                                            if (rcv_buffer[22] == EV_SOFTWARE_CHECK_ERR)
                                            {
                                                if (rcv_buffer[23] == 0x5a)
                                                {
                                                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                                                               "CID文件错误\n",
                                                               0, 0);
                                                    LOG_Write(LOG_KERNEL, "CID文件错误(执行)!!\n", NULL);
                                                }
                                            }
                                            else if (rcv_buffer[22] == MMI_S_AE_MMI_ERR)
                                            {
                                                if (rcv_buffer[23] == 0x5a)
                                                {
                                                    ER_Set_Err_Stat(MMI_S_AE_MMI_ERR, ER_ALERT,
                                                                    "HMI模件异常\n", 0, 0, 1, 0);
                                                    LOG_Write(LOG_KERNEL, "HMI模件异常(执行, 动作)!!\n", NULL);
                                                }
                                                else if (rcv_buffer[23] == 0xa5)
                                                {
                                                    ER_Set_Err_Stat(MMI_S_AE_MMI_ERR, ER_ALERT,
                                                                    "HMI模件异常\n", 0, 0, 0, 0);
                                                    LOG_Write(LOG_KERNEL, "HMI模件异常(执行, 返回)!!\n", NULL);
                                                }
                                            }
                                            opt_success = EP_SUCCESS;
                                        }
                                        break;

                                    default:
                                        opt_success=EP_ERROR;
                                        p=p_send_buffer;
                                        *p++=0;
                                        *p++=0;
                                        *p++=0x20;
                                        *p++=0x03;
                                        *p++=0;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        break;
                                }
                                if(opt_success==EP_SUCCESS)         /*遥控成功*/
                                {
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=0x10;
                                    *p++=0x03;
                                    *p++=0;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    /*LOG_Dbg_Msg("recived order with action\n",0,0,0,0,0,0);             */
                                }
                                else                    /*遥控失败*/
                                {
                                }
                            }
                            break;
                        case 0x1393:
                            opt_success=-1;
                            if(rcv_buffer[24]==0x5a)
                            {
                                opt_success=EP_SUCCESS;
                                SIO_Enable_DO();
                            }
                            else if(rcv_buffer[24]==0xa5)
                            {
                                opt_success=EP_SUCCESS;
                                SIO_Disable_DO();
                            }
                            else
                            {
                                p=p_send_buffer;
                                *p++=0x00;
                                *p++=0x00;
                                *p++=0x97;
                                *p++=0x05;
                                *p++=0x00;
                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                            }
                            if(opt_success==EP_SUCCESS)
                            {
                                p=p_send_buffer;
                                *p++=0;
                                *p++=0;
                                *p++=0x90;
                                *p++=0x05;
                                *p++=0;
                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                            }
                            break;

                        case 0x0820:
                            if(rcv_buffer[20]==0)
                            {
                                opt_success=0;
                                kc_SN=rcv_buffer[21];
                                kc_Code=rcv_buffer[22];
                                if((kc_Code!=0xa5)&&(kc_Code!=0xa3)&&(kc_Code!=0xa8))
                                {
                                    opt_success=0;
                                    kc_SN=255;
                                    kc_Code=0;
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x41;
                                    *p++=0x03;
                                    *p++=0x00;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                                else if((kc_SN>=iLgcDoChNum_g+2)&&(kc_SN!=0xff))
                                {
                                    opt_success=0;
                                    kc_SN=255;
                                    kc_Code=0;
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x40;
                                    *p++=0x03;
                                    *p++=0x00;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                                else
                                {
                                    opt_success=1;
                                }
                                if(opt_success)
                                {
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=kc_SN^0xff;
                                    *p++=kc_Code;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8c10,p-p_send_buffer);
                                }
                            }
                            if(rcv_buffer[20]==1)
                            {
                                opt_success=0;
                                kc_SN=rcv_buffer[21];
                                kc_Code=rcv_buffer[22];
                                rtcode=EP_SUCCESS;
                                if((kc_SN==rcv_buffer[21])&&(kc_Code==rcv_buffer[22]))
                                {
                                    kc_SN=rcv_buffer[21];
                                    kc_Code=rcv_buffer[22];
                                    if((kc_Code!=0xa5)&&(kc_Code!=0xa3)&&(kc_Code!=0xa8))
                                    {
                                        opt_success=0;
                                        p=p_send_buffer;
                                        *p++=0x00;
                                        *p++=0x00;
                                        *p++=0x61;
                                        *p++=0x03;
                                        *p++=0x00;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    }
                                    else if((kc_SN>=iLgcDoChNum_g+2)&&(kc_SN!=0xff))
                                    {
                                        opt_success=0;
                                        p=p_send_buffer;
                                        *p++=0x00;
                                        *p++=0x00;
                                        *p++=0x60;
                                        *p++=0x03;
                                        *p++=0x00;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    }
                                    else
                                    {
                                        /*调用开出传动函数*/
                                        if(kc_SN!=0xff)
                                        {
                                            if(kc_SN==iLgcDoChNum_g)
                                            {
                                                switch(kc_Code)
                                                {
                                                    case 0xa5:
                                                        SIO_Enable_DO();
                                                        break;

                                                    case 0xa3:
                                                        SIO_Disable_DO();
                                                        break;

                                                    case 0xa8:
                                                        SIO_Disable_DO();
                                                        break;

                                                    default:
                                                        break;
                                                }
                                            }
                                            else if(kc_SN==(iLgcDoChNum_g+1))
                                            {
                                                switch(kc_Code)
                                                {
                                                    case 0xa5:
                                                        SIO_Enable_Alm();
                                                        bAlmDoIsForced = TRUE;
                                                        break;

                                                    case 0xa3:
                                                        SIO_Disable_Alm();
                                                        bAlmDoIsForced = FALSE;
                                                        break;

                                                    case 0xa8:
                                                        SIO_Enable_Alm();
                                                        bAlmDoIsForced = TRUE;
                                                        break;

                                                    default:
                                                        break;
                                                }
                                            }
                                            else
                                            {
                                                switch(kc_Code)
                                                {
                                                    case 0xa5:
                                                        RD_Force_DO(kc_SN,TRUE);
                                                        break;

                                                    case 0xa3:
                                                        RD_Force_DO(kc_SN,FALSE);
                                                        break;

                                                    case 0xa8:
                                                        RD_Force_DO(kc_SN,-1);
                                                        break;

                                                    default:
                                                        break;
                                                }

                                            }

                                        }
                                        else
                                        {
                                            if(kc_Code==0xa8)
                                            {
                                                for(uctemp_val=0; uctemp_val<iLgcDoChNum_g; uctemp_val++)
                                                {
                                                    RD_Force_DO(uctemp_val,-1);
                                                }
                                                SIO_Disable_DO();
                                                SIO_Disable_Alm();
                                                bAlmDoIsForced = FALSE;
                                            }
                                            else if(kc_Code==0xa5)
                                            {
                                                for(uctemp_val=0; uctemp_val<iLgcDoChNum_g; uctemp_val++)
                                                {
                                                    RD_Force_DO(uctemp_val,TRUE);
                                                }
                                                SIO_Enable_DO();
                                                SIO_Enable_Alm();
                                                bAlmDoIsForced = TRUE;
                                            }
                                            else if(kc_Code==0xa3)
                                            {
                                                for(uctemp_val=0; uctemp_val<iLgcDoChNum_g; uctemp_val++)
                                                {
                                                    RD_Force_DO(uctemp_val,FALSE);
                                                }
                                                SIO_Disable_DO();
                                                SIO_Disable_Alm();
                                                bAlmDoIsForced = FALSE;
                                            }
                                            else
                                            {
                                                rtcode=EP_ERROR;
                                            }
                                        }
                                        if(rtcode==EP_SUCCESS)
                                        {
                                            opt_success=1;
                                        }
                                        else
                                        {
                                            opt_success=0;
                                            p=p_send_buffer;
                                            *p++=0x00;
                                            *p++=0x00;
                                            *p++=0x6f;
                                            *p++=0x03;
                                            *p++=0x00;
                                            reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        }
                                    }
                                    if(opt_success)
                                    {
                                        p=p_send_buffer;
                                        *p++=0x00;
                                        *p++=0x00;
                                        *p++=0x50;
                                        *p++=0x03;
                                        *p++=0x00;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    }
                                }
                                else
                                {
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x6f;
                                    *p++=0x03;
                                    strcpy(error_msg,"DO force failure because operation code not same ");
                                    err_msg_len=strlen(error_msg);
                                    *p++=err_msg_len;
                                    memcpy(p,error_msg,err_msg_len);
                                    p+=err_msg_len;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                                kc_SN=255;
                                kc_Code=0;
                            }
                            break;

                        case 0x0818:

#if 0
                            if(VI_Is_Fault()&&bSetIsValid_g)
                            {
                                p=p_send_buffer;
                                *p++=0x00;
                                *p++=0x00;
                                *p++=0x00;
                                memcpy(p,&rcv_buffer[21],rcv_buffer[21]+1);
                                p+=rcv_buffer[21]+1;
                                *p++=rcv_buffer[22+rcv_buffer[21]]^0xFF;
                                reply(sock_tmp,p_send_buffer,rcv_buffer,0x8c08,p-p_send_buffer);
                                break;
                            }
#endif

                            if(rcv_buffer[20]==0)
                            {
                                memcpy(old_PIO_name,&rcv_buffer[22],rcv_buffer[21]);
                                old_PIO_name[rcv_buffer[21]]='\0';
                                /*查询预发投退保护的名称是否存在*/
                                opt_success=0;
                                for(i=0; i<iSubLgcNum_g; i++)
                                {
                                    pSC_SUB_LGC_ITEM=(SC_SUB_LGC_ITEM *)SC_Get_Sub_Lgc_Attr(i);
                                    if((strcmp(old_PIO_name,pSC_SUB_LGC_ITEM->aucName))==0)
                                    {
                                        opt_success=1;
                                        break;
                                    }
                                }
                                if(opt_success)
                                {

                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x00;
                                    memcpy(p,&rcv_buffer[21],rcv_buffer[21]+1);
                                    p+=rcv_buffer[21]+1;
                                    *p++=rcv_buffer[22+rcv_buffer[21]]^0xFF;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8c08,p-p_send_buffer);
                                }
                                else
                                {
                                    strcpy(old_PIO_name,"");
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x30;
                                    *p++=0x03;
                                    *p++=0x00;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                            }
                            if(rcv_buffer[20]==1)
                            {
                                memcpy(PIO_name,&rcv_buffer[22],rcv_buffer[21]);
                                PIO_name[rcv_buffer[21]]='\0';
                                if(strcmp(PIO_name,old_PIO_name)==0)
                                {
                                    /*调用保护投退命令*/
                                    if(rcv_buffer[22+rcv_buffer[21]]==0xa5) Opc=TRUE;
                                    if(rcv_buffer[22+rcv_buffer[21]]==0x5a) Opc=FALSE;
                                    bOldStats=pSC_SUB_LGC_ITEM->bRun;
                                    opt_success=SC_Chg_Prtc_Sts(PIO_name,Opc);
                                    if(opt_success==EP_SUCCESS)
                                    {
                                        ProtStatsModifiesToLog(SC_Get_Sub_Lgc_Attr(i),bOldStats,Opc, usOpSrc);
                                        p=p_send_buffer;
                                        *p++=0x00;
                                        *p++=0x00;
                                        *p++=0x39;
                                        *p++=0x03;
                                        *p++=0x00;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    }
                                    else
                                    {
                                        p=p_send_buffer;
                                        *p++=0x00;
                                        *p++=0x00;
                                        *p++=0x3f;
                                        *p++=0x03;
                                        *p++=0x00;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    }
                                }
                                else
                                {
                                    p=p_send_buffer;
                                    *p++=0x00;
                                    *p++=0x00;
                                    *p++=0x3a;
                                    *p++=0x03;
                                    *p++=0x00;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                                strcpy(old_PIO_name,"");
                            }
                            break;
                        case 0x0e02:
                            if(rcv_buffer[22]==0)     /*预发,但不执行*/
                            {
                                linkNum = U8_TO_U16(rcv_buffer[24],rcv_buffer[23]);
                                if(linkNum > iLinkNum_g)
                                {
                                    /*失败*/
                                    opt_success=0;
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=0xc0;
                                    *p++=0x05;
                                    *p++=0;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                    break;
                                }
                                for(i = 0; i < linkNum; i++)
                                {
                                    plink = SC_Get_Sw_Link(rcv_buffer[25+i*3]);
                                    if(rcv_buffer[25+i*3]>=iLinkNum_g ||
                                            ((rcv_buffer[26+i*3]!=0x5a)&&(rcv_buffer[26+i*3]!=0xa5)))
                                    {
                                        /*失败*/
                                        opt_success=0;
                                        p=p_send_buffer;
                                        *p++=0;
                                        *p++=0;
                                        *p++=0xc0;
                                        *p++=0x05;
                                        *p++=0;
                                        reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                        break;
                                    }
                                    else
                                    {
                                        opt_success=1;
                                    }
                                }
                                if(opt_success)     /*预发遥控成功*/
                                {
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=rcv_buffer[22];
                                    *p++=rcv_buffer[23];
                                    *p++=rcv_buffer[24];
                                    *p++=0;
                                    *p++=0;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0xac05,p-p_send_buffer);
                                }
                                else                    /*预发遥控失败*/
                                {
                                }
                            }
                            else if(rcv_buffer[22]==1)	/*正式执行*/
                            {
                                opt_success = SC_Chg_Sw_Multi_Link(rcv_buffer);
                                if(opt_success != EP_SUCCESS)
                                {
                                    /*失败*/
                                    opt_success=0;
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=0xc1;
                                    *p++=0x05;
                                    *p++=0;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0x8e00,p-p_send_buffer);
                                }
                                else
                                {
                                    p=p_send_buffer;
                                    *p++=0;
                                    *p++=0;
                                    *p++=rcv_buffer[22];
                                    *p++=rcv_buffer[23];
                                    *p++=rcv_buffer[24];
                                    *p++=0;
                                    *p++=0;
                                    reply(sock_tmp,p_send_buffer,rcv_buffer,0xac05,p-p_send_buffer);
                                }
                            }
                            break;
                        default:
                            explain(sock_tmp,rcv_buffer,rcv_number, psock_fd);
                    }


                }
            }
            else
            {
                /*正常关闭*/
                uint8_t s_aucPrompt[MESSAGE_MAX_LEN];
                static uint32_t ulLinkNum = 0;

                ulLinkNum++;
                if (ulLinkNum <= MAX_LINK_HALT_NUM)
                {
                    sprintf(s_aucPrompt, "%d任务退出.\n", ucServerFlag);

                    LOG_Write(LOG_KERNEL, s_aucPrompt, NULL);
                }
                ulLinkHaltTm = TM_Get_usCnt();

                LOG_Dbg_Msg("the peer port closed\n",0,0,0,0,0,0);
                if(psock_fd==&new_fd4)
                    server4_OK=FALSE;

                if(psock_fd == &sock1_fd)
                {
                    server_hmi_1_OK = FALSE;
                }
                else if(psock_fd == &sock2_fd)
                {
                    server_hmi_2_OK = FALSE;
                }
                else if(psock_fd == &sockrec_fd)
                {
                    server_hmi_rec_OK = FALSE;
                }
                else if(psock_fd == &sock3_fd)
                {
                    server_hmi_3_OK = FALSE;
                }

                close(sock_tmp);
                if (*psock_fd == sock_tmp)
                {
                    *psock_fd=-1;
                }
                if(new_fileID>0)
                {
                    close(new_fileID);
                    new_fileID=-1;
                }
                if(strcmp(writing_filename,""))
                {
                    Set_FWS(writing_filename,pfplhead,0);
                    strcpy(writing_filename,"");
                }
                data_lenth=Frame_Head_Lenth;
                rcv_cnt=0;

                return;
            }
        }
    }
}

/***********************************************************************
* Cal_CheckSum - 该函数用来计算报文的校验和
*
* RETURNS: 计算结果
*
*/
char Cal_CheckSum(
    char *p_array,
    uint32_t array_lenth
)
{
    char CheckSum=0;
    uint32_t i;

    for(i=0; i<array_lenth; i++)
    {
        CheckSum += p_array[i];
    }

    CheckSum ^= 0xFF;

    return(CheckSum);
}

/***********************************************************************
* Create_Rpt_Head - 获取报告头
*
* RETURNS: NONE
*
*/
void Create_Rpt_Head(
    uint8_t *p_sendbuff,
    uint8_t *p_rcvbuff,
    uint16_t TYPE,
    uint32_t FDTN,
    uint16_t FRCNT,
    uint16_t FRSEQ,
    uint16_t FRDN
)
{
    uint8_t *p;

    p=p_sendbuff;
    *p++=Rpt_IntelType;
    *p++=Rpt_version;
    *p++=p_rcvbuff[3];
    *p++=MasterCPU_addr;
    *p++=LO8(TYPE);
    *p++=HI8(TYPE);
    *p++=LL8(FDTN);
    *p++=LH8(FDTN);
    *p++=HL8(FDTN);
    *p++=HH8(FDTN);
    *p++=0;
    *p++=0;
    *p++=LO8(FRCNT);
    *p++=HI8(FRCNT);
    *p++=LO8(FRSEQ);
    *p++=HI8(FRSEQ);
    *p++=p_rcvbuff[16];
    *p++=p_rcvbuff[17];
    *p++=LO8(FRDN);
    *p++=HI8(FRDN);
}

/***********************************************************************
* reply - 应答
*
* RETURNS: NONE
*
*/
EP_STATUS reply(
    int sockid,
    uint8_t *p_sendbuff,
    uint8_t *p_rcvbuff,
    uint16_t TYPE,
    uint32_t FDTN
)
{
    uint8_t *p;
    uint8_t psp[Max_Frame_Lenth];
    uint16_t unAllFrames;
    uint16_t unFrameSeq;
    uint16_t len;
    uint32_t reallen;
    int retcode;

    p=psp;
    *p++=Rpt_IntelType;
    *p++=Rpt_version;
    *p++=p_rcvbuff[3];
    *p++=MasterCPU_addr;
    *p++=LO8(TYPE);
    *p++=HI8(TYPE);
    *p++=LL8(FDTN);
    *p++=LH8(FDTN);
    *p++=HL8(FDTN);
    *p++=HH8(FDTN);
    *p++=0;
    *p++=0;

    if(FDTN>0)
        unAllFrames=(FDTN+Max_Frame_Data_Lenth-1)/Max_Frame_Data_Lenth;
    else
        unAllFrames=1;

    *p++=LO8(unAllFrames);
    *p++=HI8(unAllFrames);
    p=psp+16;
    *p++=p_rcvbuff[16];
    *p++=p_rcvbuff[17];

    for(unFrameSeq=0; unFrameSeq<unAllFrames; unFrameSeq++)
    {
        p=psp+14;
        *p++=LO8(unFrameSeq);
        *p++=HI8(unFrameSeq);
        p+=2;
        if(FDTN>=Max_Frame_Data_Lenth)
            len=Max_Frame_Data_Lenth;
        else
        {
            len=FDTN;
        }

        *p++=LO8(len);
        *p++=HI8(len);
        memcpy(p,p_sendbuff,len);
        p+=len;
        FDTN-=len;
        p_sendbuff+=len;

        *p=Cal_CheckSum(psp,len+Frame_Head_Lenth);	/* 求和校验 */

        reallen=len+Frame_Head_Lenth+1;

        /* 发送报文 */
        retcode=write(sockid, psp, reallen);
        taskDelay(1);
        if(retcode<0)
        {
            char logStr[129]= {0};

            sprintf(logStr, "write is error ,sockid=%d", sockid);
            Comm_Log_Write(5, logStr);
            retcode=EP_ERROR;

            return retcode;
        }
    }

    retcode=EP_SUCCESS;

    return retcode;
}

/***********************************************************************
* compare_func - 比较函数
*
* RETURNS: NONE
*
*/
int compare_func(const void *i,const void *j)
{
    return ((File_Info *)i)->FileSN-((File_Info *)j)->FileSN;
}


BOOL replyGooseDiCfg(uint8_t *p,uint16_t *punLen)
{
    HDL_TOTAL_VT_DI_TERM_CFG *pCfg;
    int i;
    uint16_t unSum;
    uint8_t *pHead=p;

    if (HDL_Get_Vt_DI_Term_Cfg(&pCfg)!=EP_SUCCESS)
        return(FALSE);
    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x01; //type
    unSum=pCfg->iTermCnt;
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=pCfg->aTermCfgArr[i].iValType;//双点
        *p++=LO8(pCfg->aTermCfgArr[i].uiDataSetAppID);//APPID LBYTE
        *p++=HI8(pCfg->aTermCfgArr[i].uiDataSetAppID);//APPID HBYTE

        /* 区分单双点
         */
        *p = 0x00;
        if (pCfg->aTermCfgArr[i].iValType == 1)
        {
            *p++ = 0x00;
        }
        else if (pCfg->aTermCfgArr[i].iValType == 2)
        {
            *p++ |= 0x01;
        }
        else
        {
            *p++ = 0x00;
        }

        *p++=0;
        *p++=0;
        *p++=0;
        *p++=strlen(pCfg->aTermCfgArr[i].aucDescStr);//描述长度
        strcpy(p,pCfg->aTermCfgArr[i].aucDescStr);
        p+=strlen(pCfg->aTermCfgArr[i].aucDescStr);
        *p++=strlen(pCfg->aTermCfgArr[i].aucYabanIDStr);//虚端子关联的输入软压板逻辑标识长度
        strcpy(p,pCfg->aTermCfgArr[i].aucYabanIDStr);//虚端子关联的输入软压板逻辑标识
        p+=strlen(pCfg->aTermCfgArr[i].aucYabanIDStr);

        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串1
        *p++=0x00;//虚端子保留字符串2长度
        //虚端子保留字符串2
        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串2

    }
    *punLen=p-pHead;
    return(TRUE);
}

BOOL replyGooseAiCfg(uint8_t *p,uint16_t *punLen)
{
    int i;
    uint16_t unSum;
    uint8_t *pHead=p;

    return (FALSE); /*目前底层张云还未做，暂时响应否定回答*/

    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x02; //type
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=0x05;//数据类型
        *p++=i;//APPID LBYTE
        *p++=0;//APPID HBYTE

        *p++=0;
        *p++=0;
        *p++=0;
        *p++=0;
        *p++=0x01;//描述长度
        *p++='H'+i;//描述
        *p++=0x01;//虚端子关联的输入软压板逻辑标识长度
        *p++='h'+i;//虚端子关联的输入软压板逻辑标识
        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串1
        *p++=0x00;//虚端子保留字符串2长度
        //虚端子保留字符串2
        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串2

    }
    *punLen=p-pHead;
    return(TRUE);
}

BOOL replyGooseSvCfg(uint8_t *p,uint16_t *punLen)
{
    SMV_TOTAL_VT_SV_TERM_CFG *pCfg;
    int i;
    uint16_t unSum;
    uint8_t *pHead=p;

    if (SMV_Get_Vt_SV_Term_Cfg(&pCfg)!=EP_SUCCESS)
        return(FALSE);

    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x03; //type
    unSum=pCfg->iTermCnt;
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=pCfg->aTermCfgArr[i].iValType;//双点
        *p++=LO8(pCfg->aTermCfgArr[i].uiDataSetAppID);//APPID LBYTE
        *p++=HI8(pCfg->aTermCfgArr[i].uiDataSetAppID);//APPID HBYTE

        *p++=0;
        *p++=0;
        *p++=0;
        *p++=0;

        *p++=strlen(pCfg->aTermCfgArr[i].aucDescStr);//描述长度
        strcpy(p,pCfg->aTermCfgArr[i].aucDescStr);
        p+=strlen(pCfg->aTermCfgArr[i].aucDescStr);
        *p++=strlen(pCfg->aTermCfgArr[i].aucYabanIDStr);//虚端子关联的输入软压板逻辑标识长度
        strcpy(p,pCfg->aTermCfgArr[i].aucYabanIDStr);//虚端子关联的输入软压板逻辑标识
        p+=strlen(pCfg->aTermCfgArr[i].aucYabanIDStr);

        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串1
        *p++=0x00;//虚端子保留字符串2长度
        //虚端子保留字符串2
        *p++=0x00;//虚端子保留字符串1长度
        //虚端子保留字符串2

    }
    *punLen=p-pHead;
    return(TRUE);
}

BOOL replyGooseDiStatus(uint8_t *p,uint16_t *punLen)
{
    HDL_TOTAL_VT_DI_TERM_STS vtSts;
    int i;
    uint16_t unSum;
    uint8_t *pHead=p;

    if (HDL_Get_Vt_DI_Term_Sts(&vtSts)!=EP_SUCCESS)
        return(FALSE);
    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x01; //type
    unSum=vtSts.iTermCnt;
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=vtSts.aTermStsArr[i].ucTermQuality;
        *p++=vtSts.aTermStsArr[i].ucTermVal;
    }
    *punLen=p-pHead;
    return(TRUE);
}

BOOL replyGooseAiStatus(uint8_t *p,uint16_t *punLen)
{
    int i;
    uint16_t unSum=3;
    uint8_t *pHead=p;
    static uint8_t ucA[3]= {0x01,0x02,0x04};
    static uint32_t uiD[3]= {0x01,0x02,0x04};

    return (FALSE); /*目前底层张云还未做，暂时响应否定回答*/

    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x02; //type
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=ucA[i];
        ucA[i]=ucA[i]<<1;
        if (ucA[i]==0x00)
            ucA[i]=0x01;
        *p++=LL8(uiD[i]);
        *p++=LH8(uiD[i]);
        *p++=HL8(uiD[i]);
        *p++=HH8(uiD[i]++);
        *p++=0;
    }
    *punLen=p-pHead;
    return(TRUE);
}

BOOL replyGooseSvStatus(uint8_t *p,uint16_t *punLen)
{
    SMV_TOTAL_VT_SV_TERM_STS svSts;
    int i;
    uint16_t unSum;
    uint8_t *pHead=p;

    if (SMV_Get_Vt_SV_Term_Sts(&svSts)!=EP_SUCCESS)
        return(FALSE);
    //8BYTE保留
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;
    *p++=0;

    *p++=0x03; //type
    unSum=svSts.iTermCnt;
    *p++=LO8(unSum);//个数
    *p++=HI8(unSum);//个数
    for (i=0; i<unSum; i++)
    {
        *p++=svSts.aTermStsArr[i].ucTermQuality;
        FLT_TO_BYTES(p,svSts.aTermStsArr[i].fTermVal);
        p+=4;
        *p++=0;
    }
    *punLen=p-pHead;
    return(TRUE);
}


void replyOptPower(uint8_t *p,uint16_t *punLen)
{
    int i,j;
    uint16_t unOptSum;
    uint8_t *pHead=p;

    //Reserved 2BYTE
    *p++=0x00;
    *p++=0x00;

    *p++=g_ucCCNum;//个数
    *p++=0;//个数高字节
    for (i=0; i<g_ucCCNum; i++)
    {
        *p++=arrCcPortSts[i].ucCCSn; //模件编号

        *p++=strlen(arrCcPortSts[i].ucCCDesc); //CC模件描述
        strcpy(p,arrCcPortSts[i].ucCCDesc);
        p+=strlen(arrCcPortSts[i].ucCCDesc);

        *p++=LO8(arrCcPortSts[i].usFPGASwVer<<4); //FPGA程序版本号
        *p++=HI8(arrCcPortSts[i].usFPGASwVer<<4);

        *p++=LO8(arrCcPortSts[i].usNiosSwVer<<4); //Nios程序版本号
        *p++=HI8(arrCcPortSts[i].usNiosSwVer<<4);

        *p++=LO8(arrCcPortSts[i].usSvCfgVer); //SV版本号
        *p++=HI8(arrCcPortSts[i].usSvCfgVer);

        *p++=LO8(arrCcPortSts[i].usSvCfgCrc); //SV CRC
        *p++=HI8(arrCcPortSts[i].usSvCfgCrc);

        *p++=LO8(arrCcPortSts[i].usSvBayNum); //SV支持间隔数
        *p++=HI8(arrCcPortSts[i].usSvBayNum);

        *p++=LO8(arrCcPortSts[i].usGsCfgVer); //GS版本号
        *p++=HI8(arrCcPortSts[i].usGsCfgVer);

        *p++=LO8(arrCcPortSts[i].usGsCfgCrc); //GS CRC
        *p++=HI8(arrCcPortSts[i].usGsCfgCrc);

        *p++=LO8(arrCcPortSts[i].usGsBayNum); //GS支持间隔数
        *p++=HI8(arrCcPortSts[i].usGsBayNum);

        unOptSum=arrCcPortSts[i].ucPortNum;
        *p++=unOptSum;//PortNum
        for (j=0; j<unOptSum; j++)
        {

            *p++=0;
            /*供应商信息暂时不填
            *p++=strlen(arrCcPortSts[i].ucSupplierInfo); //SupplierNameLen
            strcpy(p,arrCcPortSts[i].ucSupplierInfo);
            p+=strlen(arrCcPortSts[i].ucSupplierInfo);*/

            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].StsInfo.usStsInfo); //4 //状态标 4BYTE
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].StsInfo.usStsInfo);
            *p++=0;      //4              // 2 BYTE 保留//
            *p++=0;

            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].WarmInfo.usWarmInfo);
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].WarmInfo.usWarmInfo);
            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].AlarmInfo.usAlarmInfo);
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].AlarmInfo.usAlarmInfo);

            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].usTemp);    //温度
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].usTemp);    //温度
            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].usSndWatt);    //发送光功率
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].usSndWatt);    //发送光功率
            *p++=LO8(arrCcPortSts[i].tOptPortSts[j].usRcvWatt);    //接收光功率
            *p++=HI8(arrCcPortSts[i].tOptPortSts[j].usRcvWatt);    //接收光功率
            *p++=0;      //4              // 4 BYTE 保留//
            *p++=0;
            *p++=0;
            *p++=0;
        }
    }
    *p++=0;// 2 BYTE 保留
    *p++=0;
    *punLen=p-pHead;
}
/***********************************************************************
* explain - 解析命令
*
* RETURNS: 无
*
*/
void explain(
    int new_fd,
    uint8_t *p_rcv_buffer,
    int rcv_number,
    int *psock_fd
)
{

    int iCount;
    DI_CH *pdich;
    uint8_t channelCode = 0;
    uint8_t p_send_buffer[Max_Common_Lenth];
    uint8_t p_tmp_buffer[Max_Frame_Data_Lenth];
    uint8_t *p;
    uint8_t *p1;
    uint8_t sn;
    uint16_t send_lenth;
    uint16_t ReportType;
    uint32_t rcv_data_lenth;
    int opt_success=1;                              /*测试用,假设所有自检正常*/
    EP_DATE_TIME CPU_clock;
    int retcode = 0;
    int rtcode;
    char ask_msg[]="hello!this is a test!";
    char error_msg[256]="";
    uint8_t err_msg_len;
    uint16_t filenamelen;
    uint32_t file_lenth;
    uint32_t file_head_lenth;
    uint32_t rpt_lenth = 0;
    uint16_t LastCRC=0;
    uint16_t FrameCnt = 0;
    uint32_t FrameSeq = 0;
    uint16_t file_num;
    uint16_t frame_lenth;
    uint16_t get_number;
    uint16_t start_sn;
    uint16_t want_num;
    uint16_t rpt_num;
    uint16_t sec;
    uint16_t ms;
    char     old_tmp_filename[100];
    uint32_t read_number;
    uint32_t temp_val;
    char     filename[128]; /* 100->128 */
    char     back_filename[100];
    char     dirname[100];
    DIR      *pdir = NULL;
    uint32_t Reg_Code[2];
    uint8_t  Serials_SN[8];
    struct dirent *pent;
    uint16_t dirnamelen;
    time_t   timer;
    struct   tm *ftime;
    struct   stat pStat;
    int      fp;
    float 	 *pfloat;
    float    *pf = NULL;
    uint16_t unval;
    uint16_t untemp_val;
    uint8_t  uctemp_val;
    uint8_t  ucval;
    uint16_t *punshort; /* 遥信品质位 */
    uint16_t *pus;
    BOOL *pbool;
    BOOL *pb;
    BOOL *pc;
    BOOL	bsort;
    uint8_t *ph;
    uint8_t ps[Max_Frame_Data_Lenth];
    uint8_t hNum;
    uint8_t sNum;
    int 		i,j;
    uint8_t pucValidArea[256];
    uint8_t ValidAreaNum;
    SC_SUB_LGC_ITEM *pSC_SUB_LGC_ITEM;
    RD_LGC_LED_CH *pRD_LGC_LED_CH;
    VI_MEA_AI_CFG *pVI_MEA_AI_CFG;
    RD_HW_AI_MEA  *pHW_AI;
    VI_EVT_CFG    *pVI_EVT_CFG;
    FLT_U32_UNION ulAI_Val;
    File_Info       *FileInfo;
    File_Info       *pTmpFileInfo;
    File_Info       TmpInfo;
    uint32_t StartPos;
    uint32_t ReadLenth;
    uint16_t  ulTotalLinkMode=0;
    int iLgcPoChNum=0;
    struct stat Stat;
    BOOL bFileExistFlag=FALSE;
    BOOL bAreaSetChkFlg=FALSE;

    uint8_t  * pucDevName;/*设备名称字符串地址， 2008-7-29日 张云，支持上送设备名称修改  */
    int        iDevNameLen;/*设备名称字符串长度，不包括"\0" */

    FILENODE *pFileNode;		/* 文件节点 */
    LIST *pList = NULL;		/* 列表 */
    uint8_t ucDirType;				/* 目录类型 */
    BOOL bDirExistFlag=FALSE;	/* 目录是否存在 */
    uint8_t CrcLen = 0; /* 最后一帧CRC长度 */
    int iActMeaDiNum = 0;
    STATUS vxsts;
    BOOL bCKset =FALSE;
    uint8_t ucDiskType;
    uint8_t ucFormatType;

    uint8_t aucBuf[FULL_NAME_LEN+1];

    char tmp;

    rcv_data_lenth=U8_TO_U16(p_rcv_buffer[19],p_rcv_buffer[18]);

    ReportType=U8_TO_U16(p_rcv_buffer[5],p_rcv_buffer[4]);
    switch(ReportType)                      /*报文类型*/
    {
        case 0x3535:                        /*测试用*/
            strcpy(p_send_buffer,ask_msg);
            write(new_fd,p_send_buffer,sizeof(ask_msg));
            break;

        case 0x0100:                        /*装置状态巡检*/
            opt_success=1;
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                unsigned  char   bOpeRslt;/*2007-8-13日 张云修改，支持访问运行检修状态  */
                unsigned  char   bIsRepairSts;
                p=p_send_buffer;
                *p++=TM_GetHmiTimeQFlag();
                /*闰秒标志*/
                *p = 0;
                if(!g_bLeapSecondFlagHmi)
                {
                    /*SYN_LOG("####  巡检应答报文: 无B码.时分秒毫秒: %d-%d-%d-%d\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,0,0);*/
                    *p = 0;
                }
                else
                {
                    if((g_ucPorNLeapSecondHmi & IRIGB_PLS) == IRIGB_PLS)
                    {
                        /*SYN_LOG("####  巡检应答报文: 有正B码.时分秒毫秒: %d-%d-%d-%d  flag: 0x%x\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,g_ucPorNLeapSecondHmi,0);*/
                        *p |= 0x01;
                    }
                    else if((g_ucPorNLeapSecondHmi & IRIGB_NLS) == IRIGB_NLS)
                    {
                        /*SYN_LOG("####  巡检应答报文: 有负B码.时分秒毫秒: %d-%d-%d-%d  flag: 0x%x\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,g_ucPorNLeapSecondHmi,0);*/
                        *p |= 0x02;
                    }
                }
                p++;
                uctemp_val=0x00;

                if (EP_Is_Lock_DO())
                {
                    uctemp_val |= 0x01;
                }

                if(uiEdpStatus_g&LOCK_EVT) uctemp_val|=0x02;
                if(uiEdpStatus_g&NO_LICENSE) uctemp_val|=0x04;
                if(uiEdpStatus_g&ON_FAR_STATE) uctemp_val|=0x08;	/*2007-8-13日 张云修改，以前这里忘记处理了*/
                if(uiEdpStatus_g&SYS_ENG_MODE) uctemp_val|=0x40;
                if(ER_IsSetAlertFlag())
                {
                    /*2007-8-13日 张云修改，支持访问呼唤状态  */
                    uctemp_val|=0x10;
                }
                bOpeRslt=EP_Get_Repair_Sts(&bIsRepairSts);
                if(bOpeRslt&&bIsRepairSts)
                {
                    /*2007-8-13日 张云修改，支持访问运行检修状态  */
                    uctemp_val|=0x20;
                }
                *p++=uctemp_val;
                uctemp_val=0x00;
                if(uiEdpStatus_g&EVT_NOT_CLR) uctemp_val|=0x02;
                if(uiEdpStatus_g&HW_TEST_MODE) uctemp_val|=0x04;
                if(GetAdjustTimeSuccessFlag())
                {
                    uctemp_val|=0x08;
                }
                if(App_GetAcMdType() == 1)
                {
                    uctemp_val|=0x10;
                }
                if(uiPwrFreq_g == 60)
                {
                    uctemp_val|=0x20;
                }
                if(GetRecWrSts() || GetEvtWrSts())
                {
                    /* 是否处于录波/事件态 */
                    uctemp_val|=0x40;
                }

                if (VI_Is_Fault())
                {
                    /* 是否处于启动态 */
                    uctemp_val |= 0x80;
                }

                *p++=uctemp_val;
                *p++=SC_Work_Set_Area();
                SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
                if(ulTotalLinkMode&LINK_MODE_HW)
                    *p++=0x01;
                else if(ulTotalLinkMode&LINK_MODE_SW)
                    *p++=0x02;
                else if(ulTotalLinkMode&LINK_MODE_AND)
                    *p++=0x03;
                else if(ulTotalLinkMode&LINK_MODE_OR)
                    *p++=0x04;
                else if(ulTotalLinkMode&LINK_MODE_CUS)
                    *p++=0x05;
                else
                    *p++=0x00;

                uctemp_val = 0;
                if (bFstOrSecFlag)
                {
                    uctemp_val |= 0x01; /* 显示一次值 */
                }
                if(g_bChkStatus)
                {
                    uctemp_val |= 0x02; /* 检测中 */
                }
                if(g_bChkResult)
                {
                    uctemp_val |= 0x04; /* 检测异常 */
                }
                if(g_bFormatStatus)
                {
                    uctemp_val |= 0x08; /* 格式化中 */
                }
                if(!g_bMMITimeValid)
                {
                    uctemp_val |= 0x10; /* 对时有效应答 */
                }

                /* 时间管理状态反馈 */
                if (g_TimeSynIntSts)
                {
                    uctemp_val |= 0x20; /* 接口状态 */
                }

                if (g_TimeServeSts)
                {
                    uctemp_val |= 0x40; /* 服务状态 */
                }

                if (g_TimeLeapSts)
                {
                    uctemp_val |= 0x80; /* 跳变状态 */
                }

                *p++ = uctemp_val;

                /*支持上送装置设备名称，2008-7-29日 张云  */
                if(EP_GetDevName(&pucDevName,&iDevNameLen)==EP_SUCCESS)
                {

                    *p++=0x01;
                    if(iDevNameLen>=MAX_ID_LEN)
                    {
                        iDevNameLen=MAX_ID_LEN;
                    }
                    *p++=iDevNameLen;
                    strncpy(p,pucDevName,iDevNameLen);
                    p=p+iDevNameLen;
                }
                else
                {
                    *p++=0x00;
                }

                *p++=0x00;

                /* CPU新增状态信息 */
                tmp = 0;
                if(DP_DI_VAL_MODE_POSITIVE == n_ucDpDiValMode)
                {
                    tmp |= HMI_DP_DI_VAL_POSITIVE;
                }
                *p++=tmp;

                /* 4.03规约中该字节为：BSP操作申请类型 */
                *p++=0x00;

                *p++=SC_Get_Range_ChgCnt();   /* 量程调整计数（用于HMI和sgView更新） */

                tmp = 0;
                if(g_bCcdCrcErr)
                {
                    tmp |= CCD_CRC_ERR;
                }
                if(g_bCcdFileErr)
                {
                    tmp |= CCD_FILE_ERR;
                }
                *p++=tmp;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9680,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x0f;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

            break;

        case 0x0810:                    /*遥控命令*/

            if(p_rcv_buffer[20]==0)     /*预发,但不执行*/
            {
                if(opt_success)     /*预发遥控成功*/
                {
                    p=p_send_buffer;
                    *p++=0;
                    *p++=0;
                    *p++=p_rcv_buffer[20];
                    *p++=p_rcv_buffer[21];
                    *p++=p_rcv_buffer[22]^0xFF;
                    *p++=p_rcv_buffer[23];
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8c00,send_lenth);
                }
                else                    /*预发遥控失败*/
                {

                }
            }
            if(p_rcv_buffer[20]==1)     /*正式命令,执行*/
            {
                if(opt_success)         /*遥控成功*/
                {
                    send_lenth=26;
                    p=p_send_buffer;

                    *p++=0;
                    *p++=0;
                    *p++=0x10;
                    *p++=0x03;
                    *p++=0;
                    send_lenth=p-p_send_buffer;

                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
                else                    /*遥控失败*/
                {

                }
            }
            break;

        case 0x0200:                /*读取装置当前运行模式*/
            opt_success=1;

            if(opt_success)         /*读取成功*/
            {
                unsigned  char   bOpeRslt;/* 支持访问运行检修状态 */
                unsigned  char   bIsRepairSts;

                p=p_send_buffer;
                *p++=0;
                *p++=0;
                uctemp_val=0x00;

                if (EP_Is_Lock_DO())
                {
                    uctemp_val |= 0x01;
                }

                if(uiEdpStatus_g&LOCK_EVT) uctemp_val|=0x02;
                if(uiEdpStatus_g&NO_LICENSE) uctemp_val|=0x04;
                if(uiEdpStatus_g&ON_FAR_STATE) uctemp_val|=0x08;	/*20050225*/
                if(uiEdpStatus_g&SYS_ENG_MODE) uctemp_val|=0x40;
                if(uiEdpStatus_g&SYS_GPS_ERR)  uctemp_val|=0x80;
                if(ER_IsSetAlertFlag())
                {
                    /* 支持访问呼唤状态 */
                    uctemp_val|=0x10;
                }
                bOpeRslt=EP_Get_Repair_Sts(&bIsRepairSts);
                if(bOpeRslt&&bIsRepairSts)
                {
                    /* 支持访问运行检修状态 */
                    uctemp_val|=0x20;
                }
                *p++=uctemp_val;
                *p++=0x00;

                uctemp_val=0x00;
                if(uiEdpStatus_g&JGS_STATE) uctemp_val|=0x01;		/* 解挂锁 */

                if (bFstOrSecFlag)
                {
                    uctemp_val |= 0x02; /* 显示一次值 */
                }
                if (!g_bMMITimeValid)
                {
                    uctemp_val |= 0x04; /* 对时异常 */
                }

                if(g_TimeSynIntSts)
                {
                    uctemp_val |= 0x08; /* 对时信号异常 */
                }

                if(g_TimeServeSts)
                {
                    uctemp_val |= 0x10; /* 对时服务异常 */
                }

                if(g_TimeLeapSts)
                {
                    uctemp_val |= 0x20; /* 时间跳变 */
                }

                *p++=uctemp_val;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8200,send_lenth);
            }
            else                        /*读取失败*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0xbf;
                *p++=0x03;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

            break;

        case 0x0210:                    /*设置装置当前运行模式*/
            opt_success=1;

            /* 不允许HMI设置 */
#if 0
            if(p_rcv_buffer[20]&0x01)
                EP_Set_Sts_Bit(LOCK_DO);
            else
                EP_Clr_Sts_Bit(LOCK_DO);
#endif

            if(p_rcv_buffer[20]&0x02)
                EP_Set_Sts_Bit(LOCK_EVT);
            else
                EP_Clr_Sts_Bit(LOCK_EVT);
            if(p_rcv_buffer[20]&0x04)
                EP_Set_Sts_Bit(NO_LICENSE);
            else
                EP_Clr_Sts_Bit(NO_LICENSE);

            if(p_rcv_buffer[20]&0x08)	/*add 20050225*/
            {
                EP_Set_Sts_Bit(ON_FAR_STATE);
                VI_New_FarSts(0x01);
            }
            else
            {
                EP_Clr_Sts_Bit(ON_FAR_STATE);
                VI_New_FarSts(0x00);
            }

            if(p_rcv_buffer[20]&0x20)	/*张云 2007-8-13日，设置运行检修状态，满足goose的检修状态要求*/
            {
                EP_Set_Sts_Bit(ON_EXAM_STATE);
                VI_New_RepairSts(0x01);
            }
            else
            {
                EP_Clr_Sts_Bit(ON_EXAM_STATE);
                VI_New_RepairSts(0x00);
            }

            if(p_rcv_buffer[20]&0x40)
                EP_Set_Sts_Bit(SYS_ENG_MODE);
            else
                EP_Clr_Sts_Bit(SYS_ENG_MODE);

            if(p_rcv_buffer[20]&0x80)
                EP_Set_Sts_Bit(SYS_GPS_ERR);
            else
                EP_Clr_Sts_Bit(SYS_GPS_ERR);

            if(p_rcv_buffer[22]&0x01)
            {
                /* 不支持MMI设置 */
                /* EP_Set_Sts_Bit(JGS_STATE); */
            }
            else
            {
                /* EP_Clr_Sts_Bit(JGS_STATE); */
            }

            /* 一次/二次显示选择 */
            if (p_rcv_buffer[22]&0x02)
            {
                bFstOrSecFlag = TRUE;
            }
            else
            {
                bFstOrSecFlag = FALSE;
            }

            if (p_rcv_buffer[22]&0x04)
            {
                g_bMMITimeValid= FALSE;
            }
            else
            {
                g_bMMITimeValid= TRUE;
            }

            if (p_rcv_buffer[22] & 0x10)
            {
                g_TimeServeSts = TRUE;
            }
            else
            {
                g_TimeServeSts = FALSE;
            }

            if (p_rcv_buffer[22] & 0x20)
            {
                g_TimeLeapSts = TRUE;
            }
            else
            {
                g_TimeLeapSts = FALSE;
            }

            if(opt_success)             /*设置成功*/
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0xC0;
                *p++=0x03;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            else                        /*设置失败*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0xdf;
                *p++=0x03;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;
        case 0x0D00:                   /*读取保护CPU的TCP/IP网络配置信息*/
        {
            int i,j ;
            EDP_NET_CFG_INFO RtNetInfo;
            if(NT_GetNetRunCfg(&RtNetInfo)==EP_SUCCESS)
            {
                p=p_send_buffer;
                for(j=0; j<4; j++)
                    *p++=0x00;/*保留4个字节*/
                *p++=RtNetInfo.iValidNetNum; /*以太网数目*/
                for(j=0; j<4; j++)
                    *p++=0x00;/*保留4个字节*/
                for(i=0; i<RtNetInfo.iValidNetNum; i++)
                {
                    memcpy(p,RtNetInfo.NetInfArr[i].aucMacAddr,6);/*mac地址*/
                    p+=6;
                    for(j=0; j<4; j++)
                        *p++=0x00;/*保留4个字节*/
                    memcpy(p,RtNetInfo.NetInfArr[i].aucIpAddr,4);/*IP地址*/
                    p+=4;
                    for(j=0; j<4; j++)
                        *p++=0x00;/*保留4个字节*/
                    memcpy(p,RtNetInfo.NetInfArr[i].aucIpMsk,4);/*子网掩码*/
                    p+=4;
                    for(j=0; j<4; j++)
                        *p++=0x00;/*保留4个字节*/
                }
                for(j=0; j<2; j++)
                    *p++=0x00;/*保留2个字节*/
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9500,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x0F;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x0D10: /*设置保护CPU的TCP/IP网络配置信息*/
        {
            int i;
            EDP_NET_CFG_INFO RtNetInfo;
            EDP_NET_CFG_INFO RtOldNetInfo;
            NT_GetNetRunCfg(&RtOldNetInfo);

            opt_success=EP_SUCCESS;
            p=&p_rcv_buffer[20];
            p+=4; /*4个保留字节*/
            RtNetInfo.iValidNetNum = *p++; /*保护CPU的以太网口数目NC*/
            if(RtNetInfo.iValidNetNum>RtOldNetInfo.iValidNetNum)
            {
                for(i=RtOldNetInfo.iValidNetNum; i<RtNetInfo.iValidNetNum; i++)
                {
                    RtOldNetInfo.NetInfArr[i].iNetSeqNo=i;
                    RtOldNetInfo.NetInfArr[i].aucIpAddr[0]=192;
                    RtOldNetInfo.NetInfArr[i].aucIpAddr[1]=168;
                    RtOldNetInfo.NetInfArr[i].aucIpAddr[2]=0;
                    RtOldNetInfo.NetInfArr[i].aucIpAddr[3]=123;

                    RtOldNetInfo.NetInfArr[i].aucIpMsk[0]=255;
                    RtOldNetInfo.NetInfArr[i].aucIpMsk[1]=0;
                    RtOldNetInfo.NetInfArr[i].aucIpMsk[2]=0;
                    RtOldNetInfo.NetInfArr[i].aucIpMsk[3]=0;
                }
            }

            if(RtNetInfo.iValidNetNum>MAX_EDP_NET_NUM)
            {
                /* 防止异常，数组越界   张云2007-9-6日 */
                opt_success = EP_ERROR;
            }
            else
            {
                p+=4; /*4个保留字节*/
                for(i=0; i<RtNetInfo.iValidNetNum; i++)
                {
                    memcpy(RtNetInfo.NetInfArr[i].aucIpAddr,p,4);/*IP地址*/
                    p+=4;
                    p+=14; /*14个保留字节*/
                    memcpy(RtNetInfo.NetInfArr[i].aucIpMsk,p,4);/*子网掩码*/
                    p+=4;
                    p+=4; /*4个保留字节*/
                    if(NT_SetOneNetIpAddr(i,RtNetInfo.NetInfArr[i].aucIpAddr)!=EP_SUCCESS)
                    {
                        opt_success = EP_ERROR;
                        break;
                    }
                }
            }
            p+=2; /*2个保留字节*/
            if(opt_success==EP_SUCCESS)
            {
                IPAdressModifiesToLog(RtOldNetInfo,RtNetInfo);
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x10;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x1F;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x0300:                    /*读取CPU时钟*/
            opt_success=TM_Get_Sys_Time(&CPU_clock);
            if((opt_success==EP_SUCCESS)||(opt_success==EP_LOCAL_MSG))             /*读取成功*/
            {
                /*send_lenth=34;*/
                p=p_send_buffer;
                /*Create_Rpt_Head(p_send_buffer,p_rcv_buffer,0x8210,13,1,0,13);*/
                *p++=0;
                *p++=0;
                *p++=(uint8_t)(CPU_clock.unYear&0xff);
                *p++=(uint8_t)(CPU_clock.unYear>>8);
                *p++=CPU_clock.ucMonth;
                *p++=CPU_clock.ucDate;
                *p++=CPU_clock.ucHour;
                *p++=CPU_clock.ucMinute;
                *p++=CPU_clock.ucSec;
                *p++=(uint8_t)(CPU_clock.unMSEL&0xff);
                *p++=(uint8_t)(CPU_clock.unMSEL>>8);
                *p++=(uint8_t)(CPU_clock.unMicroSec&0xff);
                *p++=(uint8_t)(CPU_clock.unMicroSec>>8);
                send_lenth=p-p_send_buffer;
                /*SYN_LOG("kevin 0x0300:读取时钟:时分秒毫秒:%d-%d-%d-%d, us:%d \n ",
                    CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL, TM_Get_usCnt(),0);*/
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8210,send_lenth);

            }
            else                        /*读取失败*/
            {
                /*send_lenth=26;*/
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x40;
                *p++=0x02;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

            break;

        case 0x0310:					/*设置日历时钟*/
            p=&p_rcv_buffer[20];
            CPU_clock.unYear=U8_TO_U16(p[1],p[0]);
            CPU_clock.ucMonth=p[2];
            CPU_clock.ucDate=p[3];
            CPU_clock.ucHour=p[4];
            CPU_clock.ucMinute=p[5];
            CPU_clock.ucSec=p[6];
            CPU_clock.unMSEL=U8_TO_U16(p[8],p[7]);
            CPU_clock.unMicroSec=U8_TO_U16(p[10],p[9]);

            /* HMI保证在传递对时品质时永远非0
             */
            if (p[11] != 0)
            {
                ucHmiTmQflag = p[11];
            }

            /*HMI传递的闰秒标志*/
            g_ucHmiLsFlag = p[12];
            /*清标志时是否也要绝对时间的有效性?? 答:不需要*/
            /*清闰秒*/
            if(((g_ucHmiLsFlag & 0x01) == 0x01) && ((g_ucHmiLsFlag & 0x02) == 0x0) && ((g_ucHmiLsFlag & 0x04) == 0x0))
            {
                SYN_ClearLsFlag();
                SYN_LOG("####  0x0310 设置报文: 清除闰秒标志. 闰秒.时分秒毫秒: %d-%d-%d-%d  us:%d \n",
                        CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
            }
            /*设置闰秒*/
            if((g_ucHmiLsFlag & 0x01) == 0x01 && CPU_clock.ucMinute == 59)
            {
                /*设置闰秒无效*/
                if(((g_ucHmiLsFlag & 0x02) == 0x02) && ((g_ucHmiLsFlag & 0x04) == 0x04))
                {
                    SYN_LOG("####  0x0310 设置报文: 设置报文非法, 正负闰秒同时置上了,.时分秒毫秒: %d-%d-%d-%d us:%d\n",
                            CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
                }
                /*设置正闰秒*/
                else if(((g_ucHmiLsFlag & 0x02) == 0x02) && ((g_ucHmiLsFlag & 0x04) == 0x0))
                {
                    if(SYN_IsLsFlagClear())
                    {
                        SYN_SetLsSpecialFlag();
                        SYN_LOG("####  0x0310 设置报文: 将有正闰秒.时分秒毫秒: %d-%d-%d-%d us:%d\n",
                                CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
                        g_bLeapSecondFlagHmi = TRUE;
                        g_ucPorNLeapSecondHmi = IRIGB_PLS;
                    }
                    else
                    {
                        SYN_LOG("####  0x0310 设置报文:润秒标志还未清除,设置正闰秒无效.时分秒毫秒: %d-%d-%d-%d us:%d\n",
                                CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
                    }
                }
                /*设置负闰秒*/
                else if(((g_ucHmiLsFlag & 0x02) == 0x0) && ((g_ucHmiLsFlag & 0x04) == 0x04 ))
                {
                    if(SYN_IsLsFlagClear())
                    {
                        SYN_SetLsSpecialFlag();
                        SYN_LOG("####  0x0310 设置报文: 将有负闰秒.时分秒毫秒: %d-%d-%d-%d us:%d\n",
                                CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
                        g_bLeapSecondFlagHmi = TRUE;
                        g_ucPorNLeapSecondHmi = IRIGB_NLS;
                    }
                    else
                    {
                        SYN_LOG("####  0x0310 设置报文:润秒标志还未清除,设置负闰秒无效.时分秒毫秒: %d-%d-%d-%d us:%d\n",
                                CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,TM_Get_usCnt(),0);
                    }
                }
            }

            if((sXmlCfg.smvCfg.synMode == INTERNAL_DEFAULT)||(sXmlCfg.smvCfg.synMode==EXTERNAL_HMI_ADJ))
            {
                uint32_t ulSec = 0;
                /*闰秒前的55秒至下一分的45秒，不更新基准，误差大概2ms*/
                ulSec=TM_Time_To_Long(&CPU_clock);
                /*if(!((SYN_IsLsFlagClear() && CPU_clock.ucSec > 55)
                    ||(g_bLeapSecondFlagHmi && CPU_clock.ucSec < 5)
                    || (ulSec - g_ulLSDataTimeSecCnt) < IRIGB_SPECIAL_SEC))*/
                /*闰秒标记清除，如果未清除则增加判断在闰秒所在分的56s前是允许对时的,56开始才不允许*/
                if(SYN_IsLsFlagClear() || (CPU_clock.ucSec < 56 && CPU_clock.ucMinute == 59))
                {
                    SYN_LOG("####  0310报文设置时间生效: %d-%d-%d-%d  us:%d\n",
                            CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,
                            CPU_clock.unMSEL,TM_Get_usCnt(),0);
                    opt_success=TM_Set_Sys_Time(&CPU_clock,1);
                }
                else
                {
                    opt_success = EP_SUCCESS;
                    SYN_LOG("####  0310报文: CPU在闰秒特殊处理时间内取消设置时间报文: %d-%d-%d-%d  us:%d\n",
                            CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,
                            CPU_clock.unMSEL,TM_Get_usCnt(),0);
                }

                if(opt_success==EP_SUCCESS)
                {
                    AdjustTimeSuccessFlag =TRUE ;
                    GetAbsTimeInterval=tickGet();
                }
            }
            else
            {
                opt_success=EP_SUCCESS;
            }
            if(opt_success==EP_SUCCESS)
            {
                retcode=TM_Get_Sys_Time(&CPU_clock);
                if((retcode==EP_SUCCESS)||(retcode==EP_LOCAL_MSG))
                {

                    /*2006-7-30日，张云修改  */
                    set_date(CPU_clock.unYear, CPU_clock.ucMonth, CPU_clock.ucDate,
                             CPU_clock.ucHour, CPU_clock.ucMinute, CPU_clock.ucSec);

                    p=p_send_buffer;
                    *p++=0;
                    *p = 0; /*先清零*/
                    if(!g_bLeapSecondFlagHmi)
                    {
                        /*SYN_LOG("####  0x0310 报文应答: 无闰秒标志.时分秒毫秒: %d-%d-%d-%d\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,0,0);*/
                        *p=0;
                    }
                    else
                    {
                        if((g_ucPorNLeapSecondHmi & IRIGB_PLS) == IRIGB_PLS)
                        {
                            SYN_LOG("####  0x0310 报文应答: 将有正闰秒.时分秒毫秒: %d-%d-%d-%d  flag: 0x%x\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,g_ucPorNLeapSecondHmi,0);
                            *p |= 0x01;
                        }
                        else if((g_ucPorNLeapSecondHmi & IRIGB_NLS) == IRIGB_NLS)
                        {
                            SYN_LOG("####  0x0310 报文应答: 将有负闰秒.时分秒毫秒: %d-%d-%d-%d  flag: 0x%x\n",CPU_clock.ucHour,CPU_clock.ucMinute,CPU_clock.ucSec,CPU_clock.unMSEL,g_ucPorNLeapSecondHmi,0);
                            *p |= 0x02;
                        }
                    }
                    p++;
                    *p++=LO8(CPU_clock.unYear);
                    *p++=HI8(CPU_clock.unYear);
                    *p++=CPU_clock.ucMonth;
                    *p++=CPU_clock.ucDate;
                    *p++=CPU_clock.ucHour;
                    *p++=CPU_clock.ucMinute;
                    *p++=CPU_clock.ucSec;
                    *p++=LO8(CPU_clock.unMSEL);
                    *p++=HI8(CPU_clock.unMSEL);
                    *p++=LO8(CPU_clock.unMicroSec);
                    *p++=HI8(CPU_clock.unMicroSec);
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8210,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x5f;
                    *p++=0x02;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x5f;
                *p++=0x02;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0600:					/*读取压板有效状态*/
            pbool=malloc(Max_Frame_Data_Lenth);
            {
                static BOOL bLog0600=TRUE;
                if (pbool==NULL)
                {
                    if (bLog0600)
                    {
                        bLog0600=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0600 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x0600报文时申请内存失败.\n", NULL);
                        }
                    }
                    break;
                }
            }
            pb=pbool;
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                for(uctemp_val=0; uctemp_val<iLinkNum_g; uctemp_val++)
                {
                    retcode=SC_Get_Link_Now_Sts(uctemp_val, pb);
                    pb++;
                    if(retcode!=EP_SUCCESS) break;

                }
            }
            else						/*add 20050513*/
                retcode=EP_ERROR;
            if(retcode==EP_SUCCESS)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=iLinkNum_g;
                pb=pbool;
                for(temp_val=0; temp_val<iLinkNum_g; temp_val++)
                {
                    *p++=temp_val;
                    *p++=*pb++;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8b10,send_lenth);
                free(pbool);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x8f;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pbool);
            }
            break;
        case 0x0604:					/*读取软硬压板状态*/
            pbool=malloc(Max_Frame_Data_Lenth);
            {
                static BOOL bLog0604=TRUE;
                if (pbool==NULL)
                {
                    if (bLog0604)
                    {
                        bLog0604=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0604 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x0604报文时申请内存失败.\n", NULL);
                        }
                    }
                    break;
                }
            }
            pb=pbool;
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                if(p_rcv_buffer[20])    /*Soft link*/
                    for(uctemp_val=0; uctemp_val<iLinkNum_g; uctemp_val++)
                    {
                        retcode=SC_Get_Link_SW_Sts(uctemp_val, pb);
                        pb++;
                        if(retcode!=EP_SUCCESS) break;
                    }
                else                    /*Hard link*/
                    for(uctemp_val=0; uctemp_val<iLinkNum_g; uctemp_val++)
                    {
                        retcode=SC_Get_Link_HW_Sts(uctemp_val, pb);
                        pb++;
                        if(retcode!=EP_SUCCESS) break;
                    }
            }
            else						/*add 20050513*/
                retcode=EP_ERROR;
            if(retcode==EP_SUCCESS)
            {
                p=p_send_buffer;
                *p++=p_rcv_buffer[20];
                *p++=0x00;
                *p++=iLinkNum_g;
                pb=pbool;
                for(temp_val=0; temp_val<iLinkNum_g; temp_val++)
                {
                    *p++=temp_val;
                    *p++=*pb++;
                }
                send_lenth=p-p_send_buffer;

                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8b14,send_lenth);
                free(pbool);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0xdf;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pbool);
            }
            break;
        case 0x0608:					/*定制压板模式下读取压板模式*/
        {

            BOOL pchar[Max_Frame_Data_Lenth];
            SC_LINK_ITEM *pLinkAttr;

            SC_Get_Link_Mode_Sts(&ulTotalLinkMode);
            pc=pchar;
            retcode=EP_SUCCESS;
            if(!(ulTotalLinkMode&LINK_MODE_CUS))    /*非定制压板模式情况下不处理该0x0608命令*/
            {
                retcode=EP_ERROR;
            }
            else if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                for(uctemp_val=0; uctemp_val<iLinkNum_g; uctemp_val++)
                {
                    pLinkAttr=(SC_LINK_ITEM *)SC_Get_Link_Attr(uctemp_val);
                    if((pLinkAttr==NULL)||(pLinkAttr->aucMode!=LINK_MODE_HW&&pLinkAttr->aucMode!=LINK_MODE_SW&&
                                           pLinkAttr->aucMode!=LINK_MODE_AND&&pLinkAttr->aucMode!=LINK_MODE_OR))
                    {
                        retcode=EP_ERROR;
                        break;
                    }
                    *pc=pLinkAttr->aucMode;
                    pc++;
                }
            }
            else						/*add 20050513*/
                retcode=EP_ERROR;
            if(retcode==EP_SUCCESS)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=iLinkNum_g;
                pc=pchar;
                for(temp_val=0; temp_val<iLinkNum_g; temp_val++)
                {
                    *p++=temp_val;
                    switch(*pc++)
                    {
                        case LINK_MODE_HW:
                            *p++=0x01;
                            break;
                        case LINK_MODE_SW:
                            *p++=0x02;
                            break;
                        case LINK_MODE_AND:
                            *p++=0x03;
                            break;
                        case LINK_MODE_OR:
                            *p++=0x04;
                            break;
                        default:
                            /* assert(0); */
                            *p++=0x00;
                            break;
                    }
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8b18,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0xef;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x0F80:                        /*读取光纵通道信息*/  /* hchj */
            if(EP_Get_04CPU_Init_End_Flag())
            {
                for(uctemp_val=0; uctemp_val<OPT_GetOptChTotalNum(); uctemp_val++)
                {
                    retcode = OPT_GetOptChStsRpt(uctemp_val,&chStsReportArray[uctemp_val]);
                    if(retcode!=EP_SUCCESS)
                        break ;
                }
            }
            else
                retcode = EP_ERROR;
            if(retcode==EP_SUCCESS)
            {
                int iTotalNum;

                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=(uint8_t )OPT_GetOptChTotalNum(); /*通道个数低位*/
                *p++=0x00;   /*通道个数高位*/

                iTotalNum = OPT_GetOptChTotalNum();
                for(uctemp_val=0; uctemp_val<iTotalNum; uctemp_val++)
                {
                    *p++= chStsReportArray[uctemp_val].ucOptChNum ; /*序号*/
                    *p++= chStsReportArray[uctemp_val].bChInitFlag ; /* 通道是否配置标志*/
                    *p++= chStsReportArray[uctemp_val].bChComStableFlag ; /* 通道通信稳定标志*/
                    *p++= chStsReportArray[uctemp_val].bChSamSynFlag ; /* 通道采样同步标志*/
                    *p++= chStsReportArray[uctemp_val].bLocalIsMaster ;/*本机同步主从标志*/
                    *p++= chStsReportArray[uctemp_val].bPeerIsMaster ;/*该通道对侧的同步主从标志*/
                    *p++=0x00;/* 保留2个字节*/
                    *p++=0x00;
                    *p++=LL8(chStsReportArray[uctemp_val].ulFrComTime); 	/*该通道报文通信时间，单位US*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulFrComTime);
                    *p++=HL8(chStsReportArray[uctemp_val].ulFrComTime);
                    *p++=HH8(chStsReportArray[uctemp_val].ulFrComTime);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalComInstableNum); 	/*该通道总的通信失稳次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalComInstableNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalComInstableNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalComInstableNum);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalFrLostNum); 	/*该通道总的丢帧次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalFrLostNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalFrLostNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalFrLostNum);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalFrDelayNum); 	/*该通道总的帧延迟次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalFrDelayNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalFrDelayNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalFrDelayNum);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalFrErrNum); 	/*该通道总的帧错误次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalFrErrNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalFrErrNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalFrErrNum);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalChComeTimeChangeNum); 	/*该通道总的通信时间变化次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalChComeTimeChangeNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalChComeTimeChangeNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalChComeTimeChangeNum);
                    *p++=LL8(chStsReportArray[uctemp_val].ulTotalSamMissSynNum); 	/*该通道总的采样失步次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulTotalSamMissSynNum);
                    *p++=HL8(chStsReportArray[uctemp_val].ulTotalSamMissSynNum);
                    *p++=HH8(chStsReportArray[uctemp_val].ulTotalSamMissSynNum);

                    *p++=LL8(chStsReportArray[uctemp_val].ulHdlcCrcErr); 	/*该通道hdlc总的CRC校验错误次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulHdlcCrcErr);
                    *p++=HL8(chStsReportArray[uctemp_val].ulHdlcCrcErr);
                    *p++=HH8(chStsReportArray[uctemp_val].ulHdlcCrcErr);

                    *p++=LL8(chStsReportArray[uctemp_val].ulHdlcDpllErr); 	/*该通道hdlc总的锁相环错误次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulHdlcDpllErr);
                    *p++=HL8(chStsReportArray[uctemp_val].ulHdlcDpllErr);
                    *p++=HH8(chStsReportArray[uctemp_val].ulHdlcDpllErr);

                    *p++=LL8(chStsReportArray[uctemp_val].ulHdlcAddrErr);       /*该通道hdlc总的接收地址串扰错误次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulHdlcAddrErr);
                    *p++=HL8(chStsReportArray[uctemp_val].ulHdlcAddrErr);
                    *p++=HH8(chStsReportArray[uctemp_val].ulHdlcAddrErr);

                    *p++=LL8(chStsReportArray[uctemp_val].ulHdlcCpmBusy); 	/*该通道hdlc总的因CPM忙导致的错误*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulHdlcCpmBusy);
                    *p++=HL8(chStsReportArray[uctemp_val].ulHdlcCpmBusy);
                    *p++=HH8(chStsReportArray[uctemp_val].ulHdlcCpmBusy);

                    *p++=LL8(chStsReportArray[uctemp_val].ulHdlcRcvBusy); 	/*该通道hdlc总的因接收忙导致的错误*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulHdlcRcvBusy);
                    *p++=HL8(chStsReportArray[uctemp_val].ulHdlcRcvBusy);
                    *p++=HH8(chStsReportArray[uctemp_val].ulHdlcRcvBusy);

                    *p++=LL8(chStsReportArray[uctemp_val].ulFrCRCErrNumPerSec); 	/*该通道每秒CRC帧错误次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulFrCRCErrNumPerSec);
                    *p++=HL8(chStsReportArray[uctemp_val].ulFrCRCErrNumPerSec);
                    *p++=HH8(chStsReportArray[uctemp_val].ulFrCRCErrNumPerSec);

                    *p++=LL8(chStsReportArray[uctemp_val].ulFrLostNumPerSec); 	/*该通道每秒丢帧次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulFrLostNumPerSec);
                    *p++=HL8(chStsReportArray[uctemp_val].ulFrLostNumPerSec);
                    *p++=HH8(chStsReportArray[uctemp_val].ulFrLostNumPerSec);

                    *p++=LL8(chStsReportArray[uctemp_val].ulFrDelayNumPerSec); 	/*该通道每秒帧延迟次数*/
                    *p++=LH8(chStsReportArray[uctemp_val].ulFrDelayNumPerSec);
                    *p++=HL8(chStsReportArray[uctemp_val].ulFrDelayNumPerSec);
                    *p++=HH8(chStsReportArray[uctemp_val].ulFrDelayNumPerSec);
                    *p++=LO8(chStsReportArray[uctemp_val].uiLocalRandCode);  /* 本侧随机编码 */
                    *p++=HI8(chStsReportArray[uctemp_val].uiLocalRandCode);
                    *p++=LO8(chStsReportArray[uctemp_val].uiPeerRandCode); /* 该通道对侧随机编码 */
                    *p++=HI8(chStsReportArray[uctemp_val].uiPeerRandCode);
                    *p++=LO8(arrCcPortSts[0].tOptPortSts[2+iTotalNum-1-uctemp_val].usSndWatt);  /* 本侧随机编码 */
                    *p++=HI8(arrCcPortSts[0].tOptPortSts[2+iTotalNum-1-uctemp_val].usSndWatt);
                    *p++=LO8(arrCcPortSts[0].tOptPortSts[2+iTotalNum-1-uctemp_val].usRcvWatt); /* 该通道对侧随机编码 */
                    *p++=HI8(arrCcPortSts[0].tOptPortSts[2+iTotalNum-1-uctemp_val].usRcvWatt);

                    for(i=0; i<24; i++)
                        *p++=0x00 ;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xa400,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x58;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;
        case 0x0FC0: 						/*清除光纵通道状态*/
            if(EP_Get_04CPU_Init_End_Flag())
            {
                p=&p_rcv_buffer[20];
                channelCode =p[2]; /*清除的光纵通道号，序号从1开始*/
                /*assert(channelCode>0);*/
                OPT_ClearOptChStsRpt(channelCode);
                retcode = EP_SUCCESS;
            }
            else
                retcode = EP_ERROR;
            if(retcode==EP_SUCCESS)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=channelCode; /* 清除的光纵通道号 ，序号从0开始 */
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xa800,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x5c;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x1040:                    /*读取测量量命令*/
            if(p_rcv_buffer[20]==0)     /*读取全部测量量*/
            {
                uint8_t *pTmpSndBuf=NULL;
                uint8_t *pStart;
                int iMeaValueNum=0;
                static BOOL bLog1040=TRUE;
                RD_AI_MEA *pmeaRslt;
                uint32_t ulCalcTm = 0;
                int retcode;
                EP_DATE_TIME dttm;

                iMeaValueNum=ME_Get_Msu_Num();
                pmeaRslt=(RD_AI_MEA *)malloc(sizeof(RD_AI_MEA)*iMeaValueNum);
                if (pmeaRslt==NULL)
                {
                    if (bLog1040)
                    {
                        bLog1040=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x1040 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x1040报文时申请内存失败.\n", NULL);
                        }
                    }
                    break;
                }

                /* 获取遥测时标和值 */
                taskLock();
                ulCalcTm = ME_GetCalcTm();
                RD_Mea_AI(pmeaRslt);
                taskUnlock();

                send_lenth=iMeaValueNum*10+20;

                if(send_lenth>Max_Frame_Data_Lenth)
                {
                    pTmpSndBuf=malloc(send_lenth);
                    if (pTmpSndBuf==NULL)
                    {
                        if (bLog1040)
                        {
                            bLog1040=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x1040 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x1040报文时申请内存失败.\n", NULL);
                            }
                        }
                        free(pmeaRslt);
                        break;
                    }
                    pStart=pTmpSndBuf;
                }
                else
                {
                    pStart=p_send_buffer;
                }

                p=pStart;
                *p++=0x00;

                /* 遥测时标填写 */
                retcode = TM_To_Dttm(ulCalcTm, &dttm);

                if (retcode == EP_SUCCESS)
                {
                    *p++ = dttm.ucHour;
                }
                else
                {
                    *p = dttm.ucHour;
                    *p |= 0x80;
                    p++;
                }
                *p++ = dttm.ucMinute;
                *p++ = dttm.ucSec;
                *p++ = LO8(dttm.unMSEL);
                *p++ = HI8(dttm.unMSEL);
                *p++ = LO8(dttm.unMicroSec);
                *p++ = HI8(dttm.unMicroSec);

                /* 时标品质 */
                *p++ = GetSysTimeQFlag();

                /* 保护测控一体化装置 */
                if (uiAppType_g == APP_PROT_MEA_MERGE)
                {
                    *p++ = 0x01; /* 是否包含品质位, bit0, 1: 包含; 0: 不包含 */
                    for (i = 0; i<7; i++)
                        *p++ = 0x00;
                }
                else
                {
                    for(i=0; i<8; i++)
                        *p++=0x00;
                }

                *p++=128;				/*发送原因：被动读取*/
                *p++=LO8(iMeaValueNum); /*张云:iMeaValueNum最大256*/
                *p++=HI8(iMeaValueNum);
                for(i=0; i<iMeaValueNum; i++)
                {
                    FLT_U32_UNION ulAI_Val;
                    *p++=(uint8_t)LO8(i);
                    *p++=(uint8_t)HI8(i);

                    /* 保测一体化装置 */
                    if (uiAppType_g == APP_PROT_MEA_MERGE)
                    {
                        *p++ = LO8((pmeaRslt+i)->usQuality);
                        *p++ = HI8((pmeaRslt+i)->usQuality);
                    }
                    else
                    {
                        *p++=0x00;
                        *p++=0x00;
                    }

                    *p++=(pmeaRslt+i)->ucUnit;
                    *p++=(pmeaRslt+i)->ucAttr;
                    ulAI_Val.fVal=(pmeaRslt+i)->fVal;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                }

                send_lenth=p-pStart;
                reply(new_fd,pStart,p_rcv_buffer,0xb000,send_lenth);
                if (pTmpSndBuf != NULL)
                    free(pTmpSndBuf);
                free(pmeaRslt);
            }
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                if(p_rcv_buffer[20]==1)  /*只读取发生越限的测量量*/
                    *p++=0x64;
                else
                    *p++=0x6f;
                *p++=0x04;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x1500:
        {
            BOOL btn=FALSE;
            switch (p_rcv_buffer[24])
            {
                case 0x01:
                    btn=replyGooseDiCfg(p_send_buffer,&send_lenth);
                    break;
                case 0x02:
                    btn=replyGooseAiCfg(p_send_buffer,&send_lenth);
                    break;
                case 0x03:
                    btn=replyGooseSvCfg(p_send_buffer,&send_lenth);
                    break;
            }

            if (btn==TRUE)
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xB600,send_lenth);
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x4F;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x1501:
        {
            BOOL btn=FALSE;
            switch (p_rcv_buffer[24])
            {
                case 0x01:
                    btn=replyGooseDiStatus(p_send_buffer,&send_lenth);
                    if (btn==TRUE)
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0xB602,send_lenth);
                    break;
                case 0x02:
                    btn=replyGooseAiStatus(p_send_buffer,&send_lenth);
                    if (btn==TRUE)
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0xB601,send_lenth);
                    break;
                case 0x03:
                    btn=replyGooseSvStatus(p_send_buffer,&send_lenth);
                    if (btn==TRUE)
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0xB601,send_lenth);
                    break;
            }
            if (btn==FALSE)
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x57;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x1510:
        {
            replyOptPower(p_send_buffer,&send_lenth);
            reply(new_fd,p_send_buffer,p_rcv_buffer,0xB610,send_lenth);
        }
        break;

        case 0x1520:		/*格式化盘符命令*/
            opt_success=ERROR;
            ucDiskType = p_rcv_buffer[22];
            ucFormatType = p_rcv_buffer[23];
            /*收到命令先回复，避免MMI 死等*/
            if((ucDiskType != FORMATTFFS && ucDiskType != FORMATSET && ucDiskType != FORMATDATA)
                    || (ucFormatType != CHECKNANDFLASH && ucFormatType != FORMATNANDFLASH))
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=ucFormatType;
                *p++=0;
                *p++=0;
                *p++=0;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac04,p-p_send_buffer);
            }
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=ucFormatType;
                *p++=1;
                *p++=0;
                *p++=0;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac04,p-p_send_buffer);
            }
            if(ucDiskType == FORMATDATA)
            {
                if(ucFormatType == CHECKNANDFLASH)
                {
                    g_bChkStatus = TRUE;
                    // opt_success = chkdsk(EP_DATA,0,0);
                    opt_success=OK;
                    g_bChkStatus = FALSE;
                    if(opt_success != OK)
                    {
                        LOG_Write(LOG_OPRATE, "DATA盘经检测存在异常.\n", NULL);
                        g_bChkResult = TRUE;
                    }
                    else
                    {
                        LOG_Write(LOG_OPRATE, "DATA盘经检测正常.\n", NULL);
                        g_bChkResult = FALSE;
                    }
                }
                else if(ucFormatType == FORMATNANDFLASH)
                {
                    if(EP_Get_04CPU_Init_End_Flag())
                    {
                        g_bFormatStatus = TRUE;
                        // opt_success = LowFormat(EP_DATA);
                        opt_success=OK;
                        g_bFormatStatus = FALSE;
                    }
                }
                if(ucFormatType == FORMATNANDFLASH && opt_success == OK)
                {
                    LOG_Write(LOG_OPRATE, "DATA盘格式化成功.\n", NULL);
                    taskDelay(100);
                    EDPreboot(REBOOT_ACTIVE);
                }
                else if(ucFormatType == FORMATNANDFLASH && opt_success  != OK)
                {
                    LOG_Write(LOG_OPRATE, "DATA盘格式化失败.\n", NULL);
                    taskDelay(100);
                    EDPreboot(REBOOT_ACTIVE);
                }
            }
            else if((ucDiskType == FORMATTFFS || ucDiskType == FORMATSET)&&ucFormatType == CHECKNANDFLASH)
            {
                g_bChkStatus = TRUE;
                if(ucDiskType == FORMATTFFS)
                {
                    // opt_success = chkdsk(EP_ROOT,0,0);
                                        opt_success=OK;
                    if(opt_success != OK)
                    {
                        LOG_Write(LOG_OPRATE, "TFFS盘经检测存在异常.\n", NULL);
                        g_bChkResult = TRUE;
                    }
                    else
                    {
                        LOG_Write(LOG_OPRATE, "TFFS盘经检测正常.\n", NULL);
                        g_bChkResult = FALSE;
                    }
                }
                else
                {
                    // opt_success = chkdsk(EP_SET,0,0);
                                        opt_success=OK;
                    if(opt_success != OK)
                    {
                        LOG_Write(LOG_OPRATE, "SET盘经检测存在异常.\n", NULL);
                        g_bChkResult = TRUE;
                    }
                    else
                    {
                        LOG_Write(LOG_OPRATE, "SET盘经检测正常.\n", NULL);
                        g_bChkResult = FALSE;
                    }
                }
                g_bChkStatus = FALSE;
            }
            else
            {
            }
            break;
        case 0x1140:                    /*测量电能清零命令*/
            if(p_rcv_buffer[20]==0)     /*预发,但不执行*/
            {
                opt_success=0;
                if(p_rcv_buffer[25]<GetPoCfgNum()||p_rcv_buffer[25]==0xff)
                    opt_success=1;
                if(!opt_success)
                {
                    /* Do not execute if fault happenning. .*/
                    p=p_send_buffer;
                    *p++=0;
                    *p++=0;
                    *p++=0xa7;
                    *p++=0x04;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    for(i=0; i<18; i++)
                        *p++=p_rcv_buffer[20+i];
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0xb100,send_lenth);
                }
            }
            else if(p_rcv_buffer[20]==1)
            {
                opt_success=0;
                if(p_rcv_buffer[25]<GetPoCfgNum()||p_rcv_buffer[25]==0xff)
                    opt_success=1;
                if(opt_success&&VI_New_PoClear(p_rcv_buffer[25]))     /* 执行成功 */
                {
                    PoClearAdjustToLog(p_rcv_buffer[25]);
                    p=p_send_buffer;
                    *p++=0;
                    *p++=0;
                    *p++=0xa8;
                    *p++=0x04;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    *p++=0;
                    *p++=0;
                    *p++=0xa9;
                    *p++=0x04;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
            }
            break;

        case 0x1350:         /* 铁电相关命令 */
            opt_success=0;
            if (p_rcv_buffer[24] == 0x11)
            {
                LOG_Dbg_Msg("铁电切换主变命令!\n", 0, 0, 0, 0, 0, 0);

                if (VI_New_TdSwitch(p_rcv_buffer[24]) == TRUE)
                {
                    opt_success = 1;
                }
            }
            else if (p_rcv_buffer[24] == 0x33)
            {
                LOG_Dbg_Msg("铁电切换进线命令!\n", 0, 0, 0, 0, 0, 0);

                if (VI_New_TdSwitch(p_rcv_buffer[24]) == TRUE)
                {
                    opt_success = 1;
                }
            }

            if (!opt_success)
            {
                /* Do not execute if fault happenning. */
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x2f;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
            else
            {
                p=p_send_buffer;

                for (i=0; i<21; i++)
                    *p++=p_rcv_buffer[20+i];
                send_lenth=p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0xb390, send_lenth);
            }
            break;

        case 0x1240: /*校准物理AIAO通道命令,保护校准*/
        {
            uint8_t ucCmd=0,ucPnl=0;
            uint8_t ucObjType=0,    /* 校准对象类型，0: ai物理通道，1: 测量量*/
                    ucOrdType=0;    /* 校准命令类型，0: 增益校准，1: 偏置校准*/
            ucCmd=p_rcv_buffer[24];
            ucPnl=p_rcv_buffer[25];

            p=p_send_buffer;
            if((ucCmd==0x11||ucCmd==0x33)&&(ucPnl==0xff))   /*目前只支持ai通道的全部通道校准*/
            {
                ucObjType=0;
                ucOrdType=(ucCmd==0x11)? 1:0;
                if(VI_New_Adjust(ucObjType, ucOrdType))     /*success calibrate*/
                {
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=p_rcv_buffer[24];
                    *p++=p_rcv_buffer[25];
                    *p++=0;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0xb240,send_lenth);
                }
                else
                {
                    *p++=0;
                    *p++=0;
                    *p++=0xbf;
                    *p++=0x04;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
            }
            else
            {
                *p++=0;
                *p++=0;
                *p++=0xb9;
                *p++=0x04;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;
        case 0x1340: /*获得某系统组件的CRC版本信息*/
        {
            uint16_t unMouduleType=0;
            uint32_t ulCcdCrc=0;
            uint8_t  ucInfoSrc=0;
            uint8_t *aucVer=NULL;
            uint8_t *pTimeStamp = NULL;
            ucInfoSrc=p_rcv_buffer[21]; /* 0表示上送组件信息为装置当前运行实际程序 1表示上送组件信息为装置上电时读取的version.ini的组件信息*/
            unMouduleType=U8_TO_U16(p_rcv_buffer[23],p_rcv_buffer[22]);
            p=p_send_buffer;
            if(unMouduleType>=1&&unMouduleType<=4&&(UnVerInfo_g.unPlatCrc)&&(UnVerInfo_g.unRelayCrc)&&(ucInfoSrc==0||ucInfoSrc==1))   /*1平台支撑组件 2保护应用组件 3保护配置组件 4MMI组件*/
            {
                *p++=0;
                *p++=0;
                *p++=p_rcv_buffer[22];
                *p++=p_rcv_buffer[23];
                *p++=0;
                *p++=0;
                if(unMouduleType==1)
                {
                    if(ucInfoSrc)
                    {
                        *p++=LO8(UnVerInfoRd_g.unPlatCrc);
                        *p++=HI8(UnVerInfoRd_g.unPlatCrc);
                        aucVer=UnVerInfoRd_g.aucPlatVer;
                    }
                    else
                    {
                        *p++=LO8(UnVerInfo_g.unPlatCrc);
                        *p++=HI8(UnVerInfo_g.unPlatCrc);
                        aucVer=UnVerInfo_g.aucPlatVer;
                    }
                }
                else if(unMouduleType==2)
                {
                    if(ucInfoSrc)
                    {
                        *p++=LO8(UnVerInfoRd_g.unRelayCrc);
                        *p++=HI8(UnVerInfoRd_g.unRelayCrc);
                        aucVer=UnVerInfoRd_g.aucRelayVer;
                    }
                    else
                    {
                        *p++=LO8(UnVerInfo_g.unRelayCrc);
                        *p++=HI8(UnVerInfo_g.unRelayCrc);
                        aucVer=UnVerInfo_g.aucRelayVer;
                    }
                }
                else if(unMouduleType==3)
                {
                    if(VER_UN_RelayCRCIsCoverCfg())
                    {
                        *p++=0;
                        *p++=0;
                        //aucVer='\0';
                    }
                    else
                    {
                        if(ucInfoSrc)
                        {
                            *p++=LO8(UnVerInfoRd_g.unCfgCrc);
                            *p++=HI8(UnVerInfoRd_g.unCfgCrc);
                            aucVer=UnVerInfoRd_g.aucCfgVer;
                        }
                        else
                        {
                            *p++=LO8(UnVerInfo_g.unCfgCrc);
                            *p++=HI8(UnVerInfo_g.unCfgCrc);
                            aucVer=UnVerInfo_g.aucCfgVer;
                        }
                    }
                }
                else if(unMouduleType==4)
                {
                    if(ucInfoSrc)
                    {
                        *p++=LO8(UnVerInfoRd_g.unMmiCrc);
                        *p++=HI8(UnVerInfoRd_g.unMmiCrc);
                    }
                    else
                    {
                        *p++=LO8(UnVerInfo_g.unMmiCrc);
                        *p++=HI8(UnVerInfo_g.unMmiCrc);
                    }
                    //aucVer='\0';
                }
                *p++=0;
                *p++=0;
                if(unMouduleType==1)
                {
                    *p++=LO8(VER_UN_GetPlatLabel());
                    *p++=HI8(VER_UN_GetPlatLabel());
                }
                else
                {
                    *p++=0;
                    *p++=0;
                }

                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                if(aucVer==NULL)
                    *p++=0;
                else
                {
                    *p++=strlen(aucVer);
                    for(i=0; i<strlen(aucVer); i++)
                        *p++=aucVer[i];
                }

                pTimeStamp = EDP_GetCcdCreatTime();
                if(pTimeStamp==NULL)
                {
                    *p++=0;
                }
                else
                {
                    *p++=strlen(pTimeStamp);
                    for(i=0; i<strlen(pTimeStamp); i++)
                        *p++=pTimeStamp[i];
                }

                ulCcdCrc = g_ulCcdFileCheckCrc;
                *p++=LL8(ulCcdCrc);
                *p++=LH8(ulCcdCrc);
                *p++=HL8(ulCcdCrc);
                *p++=HH8(ulCcdCrc);
                /**p++=0; *p++=0; *p++=0; *p++=0;*/
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xb380,send_lenth);
            }
            else
            {
                *p++=0;
                *p++=0;
                *p++=0x10;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

        }
        break;

        case 0x1341: /*获得某系统组件的CRC版本信息*/
        {
            p=p_send_buffer;
            if(SI_Rst_Ver_INI()==EP_SUCCESS)
            {
                *p++=0;
                *p++=0;
                *p++=0;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xb381,send_lenth);
            }
            else
            {
                *p++=0;
                *p++=0;
                *p++=0x11;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;

        case 0x1380:
        {
            assert (FALSE);
        }

        case 0x1381:
        {
            assert (FALSE);
        }

        case 0x1382:
        {
            assert (FALSE);
        }

        case 0x1388: /* 查询CPU配置的所有模件信息 */
        {
            LOG_Dbg_Msg("查询CPU配置的所有模件信息.\n", 0, 0, 0, 0, 0, 0);
            p = p_send_buffer;

            if (RD_GetModInfo(&p) == EP_SUCCESS)
            {
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0xb440, send_lenth);
            }
            else
            {
                *p++ = 0;
                *p++ = 0;
                *p++ = 0x60;
                *p++ = 0x05;
                *p++ = 0;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
        }
        break;

        case 0x1389: 	/* 查询CPU的某模件的物理通道配置信息 */
        {
            uint8_t aucName[256];

            p = p_send_buffer;

            EP_ID_Copy(aucName, &p_rcv_buffer[25], p_rcv_buffer[24]);

            if (RD_GetChnInfo(aucName, &p) == EP_SUCCESS)
            {
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0xb441, send_lenth);
            }
            else
            {
                *p++ = 0;
                *p++ = 0;
                *p++ = 0x68;
                *p++ = 0x05;
                *p++ = 0;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
        }
        break;

        case 0x138A: 	/* 校准CPU某模件的物理通道 */
        {
            uint8_t *pRcv;
            uint8_t aucmodName[256];
            uint8_t aucchnName[256];
            uint8_t uccmdtype;
            uint32_t ulRst;

            p = p_send_buffer;

            pRcv = &p_rcv_buffer[20];
            EP_ID_Copy(aucmodName, &pRcv[5], pRcv[4]);
            pRcv += 4+1+pRcv[4];
            uccmdtype = *pRcv++;
            pRcv += 16;

            if (pRcv[0])
            {
                EP_ID_Copy(aucchnName, &pRcv[1], pRcv[0]);
            }
            else
            {
                aucchnName[0] = '\0';
            }

            if ((uccmdtype == 0x22) || (uccmdtype == 0x55))
            {
                ulRst = RD_AdjMod(aucmodName, aucchnName, uccmdtype);

                if(ulRst == 0)
                {
                    *p++ = 0;
                    *p++ = 0;
                    *p++ = 0x70;
                    *p++ = 0x05;
                    *p++ = 0;
                    send_lenth = p-p_send_buffer;
                    reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
                }
                else if (ulRst == 1)
                {
                    *p++ = 0;
                    *p++ = 0;
                    *p++ = 0x75;
                    *p++ = 0x05;
                    *p++ = 0;
                    send_lenth = p-p_send_buffer;
                    reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
                }
                else if (ulRst == 2)
                {
                    *p++ = 0;
                    *p++ = 0;
                    *p++ = 0x74;
                    *p++ = 0x05;
                    *p++ = 0;
                    send_lenth = p-p_send_buffer;
                    reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
                }
            }
            else
            {
                *p++ = 0;
                *p++ = 0;
                *p++ = 0x76;
                *p++ = 0x05;
                *p++ = 0;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
        }
        break;

        case 0x0830: /*硬件物理通道测量校准操作,测量校准*/
        {
            uint8_t ucObjType=0,    /* 校准对象类型，0: ai物理通道，1: 测量量*/
                    ucOrdType=0,    /* 校准命令类型，0: 增益校准，1: 偏置校准*/
                    ucPnlIdx=0;     /* 校准通道号,目前只支持0XFF*/
            ucObjType=p_rcv_buffer[21];
            ucOrdType=p_rcv_buffer[22];
            ucPnlIdx=p_rcv_buffer[23];

            p=p_send_buffer;
            if((ucOrdType==0x22||ucOrdType==0x55)&&(ucPnlIdx==0xff)&&(ucObjType==0||ucObjType==1))   /*目前只支持ai通道的全部通道校准*/
            {
                ucOrdType=(ucOrdType==0x55)? 1:0;
                if(VI_New_Adjust(ucObjType, ucOrdType))     /*success calibrate*/
                {
                    ChannelAdjustToLog();
                    *p++=0;
                    *p++=0;
                    *p++=0xb0;
                    *p++=0x02;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
                else
                {
                    *p++=0;
                    *p++=0;
                    *p++=0xcf;
                    *p++=0x02;
                    *p++=0;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
            }
            else
            {
                *p++=0;
                *p++=0;
                *p++=0xc2;
                *p++=0x02;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
        }
        break;


        case 0x1200: /*读取保护任务资源消耗*/
#if 1
            if(EP_Get_04CPU_Init_End_Flag())
            {
                int i,j;
                RE_TASK_COMSUME_RESOURCE_TYPE taskSource ; /*每个特定任务的状态属性*/
                int taskArray[50]= {0};
                int taskCount =0;
                taskCount = RE_GetRunTaskCnt(&taskArray[0]);
                taskCount = 0;  /* 界面上不显示资源消耗 */
                p=p_send_buffer;
                *p++=0x00; /*保留*/
                *p++=0x00;	/*保留*/
                *p++=LO8(taskCount);/*投入运行的保护任务个数*/
                *p++=HI8(taskCount);
                for(i=0; i<taskCount; i++)
                {
                    RE_GetRunTaskConsumeResource(taskArray[i],&taskSource);
                    *p++=taskArray[i];/*任务号*/
                    *p++=LL8(taskSource.ulCurTimePerPeriod); 	/*该任务的最新周期运行时间*/
                    *p++=LH8(taskSource.ulCurTimePerPeriod);
                    *p++=HL8(taskSource.ulCurTimePerPeriod);
                    *p++=HH8(taskSource.ulCurTimePerPeriod);
                    *p++=LL8(taskSource.ulMinTimePerPeriod); 	/*该任务的最小周期运行时间*/
                    *p++=LH8(taskSource.ulMinTimePerPeriod);
                    *p++=HL8(taskSource.ulMinTimePerPeriod);
                    *p++=HH8(taskSource.ulMinTimePerPeriod);
                    *p++=LL8(taskSource.ulMaxTimePerPeriod); 	/*该任务的最大周期运行时间*/
                    *p++=LH8(taskSource.ulMaxTimePerPeriod);
                    *p++=HL8(taskSource.ulMaxTimePerPeriod);
                    *p++=HH8(taskSource.ulMaxTimePerPeriod);
                    *p++=LL8(taskSource.ulAverageTimePerPeriod); 	/*该任务的平均周期运行时间*/
                    *p++=LH8(taskSource.ulAverageTimePerPeriod);
                    *p++=HL8(taskSource.ulAverageTimePerPeriod);
                    *p++=HH8(taskSource.ulAverageTimePerPeriod);
                    for(j=0; j<64; j++)
                        *p++=0x00 ;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xB200,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x64;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
#endif
            break;
        case 0x1000: /* 读取电压监测信息 */
            if(EP_Get_04CPU_Init_End_Flag())
            {
                char tempWatchValue[5];
                WT_SYSINFO_TYPE pWt[1];
                WT_VoltWatchStsRpt(pWt);
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=pWt->bVoltWatchInitFlag ; /*电压监测功能初始化标志*/
                *p++=pWt->ucVoltWatchCh; /*电压监视路数*/
                *p++=LO8(pWt->uiSoftVer);/*电压监测软件版本号 小数*/
                *p++=HI8(pWt->uiSoftVer);/*电压监测软件版本号 整数*/
                FLT_TO_BYTES(tempWatchValue,pWt->f10VoltWatchVal);
                *p++=tempWatchValue[0];/*正10伏电压监视值*/
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];
                FLT_TO_BYTES(tempWatchValue,pWt->fNeg10VoltWatchVal);
                *p++=tempWatchValue[0];/*负10伏电压监视值*/
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->f5VoltWatchVal);
                *p++=tempWatchValue[0];/*正5伏电压监视值*/
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                for(i=0; i<4; i++)		/* 保留4个字节 */
                    *p++=0x00 ;

                FLT_TO_BYTES(tempWatchValue,pWt->f24VoltWatchVal);
                *p++=tempWatchValue[0];/* 24V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->f3Dot3VoltWatchVal);
                *p++=tempWatchValue[0];/* 3.3V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->f1Dot5VoltWatchVal);
                *p++=tempWatchValue[0];/* 1.5V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->f12VoltWatchVal);
                *p++=tempWatchValue[0];/* 12V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->fTemptWatchVal);
                *p++=tempWatchValue[0];/*温度监视值*/
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->fNeg12VoltWatchVal);
                *p++=tempWatchValue[0];/* -12V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->f2Dot5VoltWatchVal);
                *p++=tempWatchValue[0];/* 2.5V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->fHdlc1VoltWatchVal);
                *p++=tempWatchValue[0];/* hdlc1电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->fHdlc2VoltWatchVal);
                *p++=tempWatchValue[0];/* hdlc2电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->fFpga2Dot5VoltWatchVal);
                *p++=tempWatchValue[0];/* fpga使用的2.5V电压监视值 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                for(i=0; i<8; i++)
                    *p++=0x00 ;

                *p++=pWt->ucQDSts;

                for(i=0; i<3; i++)
                    *p++=0x00 ;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x60;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break ;

        case 0x1001: /* 读取电压异常信息 */
            if(EP_Get_04CPU_Init_End_Flag())
            {
                char tempWatchValue[5];
                WT_EXC_INFO_TYPE pWt[1];
                WT_VoltExcStsRpt(pWt);
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=pWt->bVoltWatchInitFlag ; /*电压监测功能初始化标志*/

                for(i=0; i<3; i++)
                    *p++=0;

                FLT_TO_BYTES(tempWatchValue,pWt->inf10VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*正10伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf10VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*正10伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf10VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*正10伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf10VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*正10伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infNeg10VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*负10伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infNeg10VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*负10伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infNeg10VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*负10伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infNeg10VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*负10伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf5VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*5伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf5VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*5伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf5VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf5VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf24VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*24伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf24VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*24伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf24VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*24伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf24VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*24伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf3Dot3VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*3.3伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf3Dot3VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*3.3伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf3Dot3VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*3.3伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf3Dot3VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*3.3伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf1Dot5VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*1.5伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf1Dot5VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*1.5伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf1Dot5VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*1.5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf1Dot5VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*1.5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf12VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*12伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf12VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*12伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf12VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*12伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf12VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*12伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infNeg12VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*-12伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infNeg12VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*-12伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infNeg12VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*-12伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infNeg12VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*-12伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf2Dot5VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*2.5伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->inf2Dot5VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*2.5伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf2Dot5VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*2.5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->inf2Dot5VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*2.5伏电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infHdlc1VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*hdlc1电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infHdlc1VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*hdlc1电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infHdlc1VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*hdlc1电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infHdlc1VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*hdlc1电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infHdlc2VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*hdlc2电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infHdlc2VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*hdlc2电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infHdlc2VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*hdlc2电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infHdlc2VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*hdlc2电压溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infFpga2Dot5VoltExc.fOvrflwVal);
                *p++=tempWatchValue[0];		/*fpga使用2.5伏电压上溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                FLT_TO_BYTES(tempWatchValue,pWt->infFpga2Dot5VoltExc.fUdrflwVal);
                *p++=tempWatchValue[0];		/*fpga使用2.5伏电压下溢出信息 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infFpga2Dot5VoltExc.ulOvrflwCnt);
                *p++=tempWatchValue[0];		/*fpga使用2.5伏电压上溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->infFpga2Dot5VoltExc.ulUdrflwCnt);
                *p++=tempWatchValue[0];		/*fpga使用2.5伏电压下溢出计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                for(i=0; i<24; i++)
                    *p++=0x00 ;

                U32_TO_BYTES(tempWatchValue,pWt->ulCpuRebootCnt);
                *p++=tempWatchValue[0];		/*主CPU连续异常复位次数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->ulDownHaltCnt);
                *p++=tempWatchValue[0];		/*下行帧中断异常计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->ulDownChkErrCnt);
                *p++=tempWatchValue[0];		/*下行帧连续校验和错误异常计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->ulDownFmtErrCnt);
                *p++=tempWatchValue[0];		/*下行帧格式错误异常计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->ulUnSupportBoardErrCnt);
                *p++=tempWatchValue[0];		/*硬件版本未支持错误异常计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                U32_TO_BYTES(tempWatchValue,pWt->ulUnKownErrCnt);
                *p++=tempWatchValue[0];		/*硬件版本未支持错误异常计数 */
                *p++=tempWatchValue[1];
                *p++=tempWatchValue[2];
                *p++=tempWatchValue[3];

                for(i=0; i<12; i++)
                    *p++=0x00 ;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac01,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x61;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break ;

        case 0x1002: /* 清除电压异常记录 */
            if(EP_Get_04CPU_Init_End_Flag())
            {
                WT_MegaClrErrRec();
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac02,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x62;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break ;

        case 0x1600: /* 读取定值量程信息 */
            p = p_send_buffer;
            if(SC_Get_SetRange(&p)==EP_SUCCESS)  /* 读取成功 */
            {
                send_lenth = p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xad00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0xF0;
                *p++=0x05;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break ;

        case 0x1003: /* MEGA16启动测试命令 */
            if(EP_Get_04CPU_Init_End_Flag())
            {
                WT_QD_TST_RSLT_TYPE Wt;
                taskDelay(10);
                Wt=WT_MegaQDTst();
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;

                if(Wt == WT_QD_TST_NORMAL)
                {
                    *p++=0x00;
                }
                else if(Wt == WT_QD_TST_ABNORMAL)
                {
                    *p++=0x01;
                }
                else if(Wt == WT_QD_TST_UNKOWN)
                {
                    *p++=0x02;
                }

                for(i=0; i<2; i++)
                    *p++=0x00 ;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0xac03,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x63;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break ;

        case 0x0610:                    	/*读取开入量状态*/
            opt_success=1;
            /*if(opt_success)*/				/*del 20050513*/
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=HI8(iLgcDiChNum_g);
                *p++=LO8(iLgcDiChNum_g);
                untemp_val=(iLgcDiChNum_g+7)/8;
                for(temp_val=0; temp_val<untemp_val; temp_val++)
                {
                    uctemp_val=0x00;
                    sn=0x00;
                    ucval=1;
                    if(temp_val!=(untemp_val-1)) j=8;
                    else j=iLgcDiChNum_g-(untemp_val-1)*8;
                    for(i=0; i<j; i++)
                    {
                        retcode=RD_Mea_Hw_DI(temp_val*8+i);
                        if(retcode==DP_TRUE||retcode==DP_FALSE||retcode==DP_INVALID_00||retcode==DP_INVALID_11)
                        {
                            if(DP_DI_VAL_MODE_POSITIVE == n_ucDpDiValMode)
                            {
                                if(retcode==DP_TRUE||retcode==DP_INVALID_11)
                                {
                                    sn|=ucval;
                                }
                                if((retcode&0x8000)&&(retcode==DP_TRUE||retcode==DP_INVALID_00))
                                {
                                    uctemp_val|=ucval;
                                }
                            }
                            else if(DP_DI_VAL_MODE_COUNTER == n_ucDpDiValMode)
                            {
                                if(retcode==DP_FALSE||retcode==DP_INVALID_11)
                                {
                                    sn|=ucval;
                                }
                                if((retcode&0x8000)&&(retcode==DP_FALSE||retcode==DP_INVALID_11))
                                {
                                    uctemp_val|=ucval;
                                }
                            }
                            else
                            {
                                /*if (bdType_g == BOARD_TYPE_E02)*/
                                if((uiAppType_g != APP_BUS))
                                {
                                    /*edp 02装置双点目前都是开关状态使用，而保护都是用TWJ做反逻辑显示,这里把分位作为双点状态显示到界面上*/
                                    if(retcode==DP_FALSE||retcode==DP_INVALID_11)
                                    {
                                        sn|=ucval;
                                    }
                                }
                                else
                                {
                                    if(retcode==DP_TRUE||retcode==DP_INVALID_11)
                                    {
                                        sn|=ucval;
                                    }
                                }

                                /*if (bdType_g == BOARD_TYPE_E02)*/
                                if(uiAppType_g != APP_BUS)
                                {
                                    if((retcode&0x8000)&&(retcode==DP_FALSE||retcode==DP_INVALID_11))
                                    {
                                        uctemp_val|=ucval;
                                    }
                                }
                                else
                                {
                                    if((retcode&0x8000)&&(retcode==DP_TRUE||retcode==DP_INVALID_00))
                                    {
                                        uctemp_val|=ucval;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(retcode&0x7fff)
                            {
                                sn|=ucval;
                            }
                            if(retcode&0x8000)
                            {
                                uctemp_val|=ucval;
                            }
                        }
                        ucval*=2;
                    }
                    *p++=sn;
                    *p++=uctemp_val;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8b00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x9f;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

            break;

        case 0x0e00:                    /*强制开入量*/

            pdich = calloc(p_rcv_buffer[22],sizeof(DI_CH));
            if (pdich==NULL)
            {
                static BOOL bLog0e00=TRUE;
                if (bLog0e00)
                {
                    bLog0e00=FALSE;
                    if(ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "Fail to calloc In explain() when deal 0x0e00 msg.\n", NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "解释0x0e00报文时申请内存失败.\n", NULL);
                    }
                }
                break;
            }

            iCount =  p_rcv_buffer[22];
            p=&p_rcv_buffer[23];
            opt_success=EP_SUCCESS;

            for(untemp_val=0; untemp_val<p_rcv_buffer[22]; untemp_val++)
            {


                sn=*p++;
                uctemp_val=*p++;

                sprintf(pdich[untemp_val].aucName,(plgcdich_g+sn)->aucName);
                pdich[untemp_val].PreStats=(plgcdich_g+sn)->iForceSts;
                switch(uctemp_val)
                {
                    case 0x55:
                        pdich[untemp_val].NewStats=1;
                        break;
                    case 0xaa:
                        pdich[untemp_val].NewStats=0;
                        break;
                    case 0x5a:
                        pdich[untemp_val].NewStats=-1;
                        break;
                    default:
                        break;
                }

#if 0
                if(sn<iLgcDiChNum_g)
                {
                    switch(uctemp_val)
                    {
                        case 0x55:
                            retcode=RD_Chg_Force_DI(sn,TRUE);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;
                            break;

                        case 0xaa:
                            retcode=RD_Chg_Force_DI(sn,FALSE);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;

                            break;

                        case 0x5a:
                            retcode=RD_Chg_Force_DI(sn,-1);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;
                            break;

                        default:
                            opt_success=EP_ERROR;
                            break;
                    }
                }
                else
                {
                    opt_success=EP_PARM_ERR;
                }
#endif
            }

#if 1
            opt_success = RD_Chg_Force_Multi_Di(p_rcv_buffer);
#endif

            if(opt_success==EP_SUCCESS)
            {

                DiForceToLog(pdich,iCount);
                p=p_send_buffer;
                p1=&p_rcv_buffer[23];
                *p++=0x00;
                *p++=0x00;
                *p++=p_rcv_buffer[22];
                for(untemp_val=0; untemp_val<p_rcv_buffer[22]; untemp_val++)
                {
                    *p++=*p1;
                    retcode=RD_Mea_Hw_DI(*p1);
                    if(retcode&0x8000)
                    {
                        if(retcode&0x7fff)
                        {
                            *p++=0xaa;
                        }
                        else
                        {
                            *p++=0x55;
                        }
                    }
                    else
                    {
                        *p++=0xa5;
                    }
                    p1+=2;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9600,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_FILE_ERR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x98;
                *p++=0x00;
                strcpy(error_msg,"when force DI file operating failure");
                err_msg_len=strlen(error_msg);
                *p++=err_msg_len;
                memcpy(p,error_msg,err_msg_len);
                p+=err_msg_len;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_ERROR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x98;
                *p++=0x00;
                strcpy(error_msg,"when force DI the operation code error");
                err_msg_len=strlen(error_msg);
                *p++=err_msg_len;
                memcpy(p,error_msg,err_msg_len);
                p+=err_msg_len;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_PARM_ERR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x90;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }

            break;

        case 0x0e01:
        {
            uint16_t DI_Force_Num=0;
            uint16_t DI_Sn=0;
            DI_Force_Num=U8_TO_U16(p_rcv_buffer[23],p_rcv_buffer[22]);
            pdich = calloc(DI_Force_Num,sizeof(DI_CH));
            if (pdich==NULL)
            {
                static BOOL bLog0e00=TRUE;
                if (bLog0e00)
                {
                    bLog0e00=FALSE;
                    if(ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "Fail to calloc In explain() when deal 0x0e01 msg.\n", NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "解释0x0e01报文时申请内存失败.\n", NULL);
                    }
                }
                break;
            }

            iCount =  (int)DI_Force_Num;
            p=&p_rcv_buffer[24];
            opt_success=EP_SUCCESS;

            for(untemp_val=0; untemp_val<DI_Force_Num; untemp_val++)
            {
                DI_Sn = U8_TO_U16(*(p+1),*p);
                p+=2;
                uctemp_val=*p++;

                sprintf(pdich[untemp_val].aucName,(plgcdich_g+DI_Sn)->aucName);
                pdich[untemp_val].PreStats=(plgcdich_g+DI_Sn)->iForceSts;
                switch(uctemp_val)
                {
                    case 0x55:
                        pdich[untemp_val].NewStats=1;
                        break;
                    case 0xaa:
                        pdich[untemp_val].NewStats=0;
                        break;
                    case 0x5a:
                        pdich[untemp_val].NewStats=-1;
                        break;
                    default:
                        break;
                }
                if(DI_Sn<iLgcDiChNum_g)
                {
                    switch(uctemp_val)
                    {
                        case 0x55:
                            retcode=RD_Chg_Force_DI(DI_Sn,TRUE);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;
                            break;

                        case 0xaa:
                            retcode=RD_Chg_Force_DI(DI_Sn,FALSE);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;

                            break;

                        case 0x5a:
                            retcode=RD_Chg_Force_DI(DI_Sn,-1);
                            if(retcode!=EP_SUCCESS)
                                opt_success=EP_FILE_ERR;
                            break;

                        default:
                            opt_success=EP_ERROR;
                            break;
                    }
                }
                else
                {
                    opt_success=EP_PARM_ERR;
                }
                p++;/*保留*/
            }

            if(opt_success==EP_SUCCESS)
            {

                DiForceToLog(pdich,iCount);
                p=p_send_buffer;
                p1=&p_rcv_buffer[24];
                *p++=0x00;
                *p++=0x00;
                *p++=p_rcv_buffer[22];
                *p++=p_rcv_buffer[23];
                for(untemp_val=0; untemp_val<DI_Force_Num; untemp_val++)
                {
                    *p++=*p1;
                    *p++=*(p1+1);
                    retcode=RD_Mea_Hw_DI(U8_TO_U16(*(p1+1),*p1));
                    if(retcode&0x8000)
                    {
                        if(retcode&0x7fff)
                        {
                            *p++=0xaa;
                        }
                        else
                        {
                            *p++=0x55;
                        }
                    }
                    else
                    {
                        *p++=0xa5;
                    }
                    p1+=4;
                    *p++=0;/*保留*/
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9601,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_FILE_ERR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x98;
                *p++=0x00;
                strcpy(error_msg,"when force DI file operating failure");
                err_msg_len=strlen(error_msg);
                *p++=err_msg_len;
                memcpy(p,error_msg,err_msg_len);
                p+=err_msg_len;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_ERROR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x98;
                *p++=0x00;
                strcpy(error_msg,"when force DI the operation code error");
                err_msg_len=strlen(error_msg);
                *p++=err_msg_len;
                memcpy(p,error_msg,err_msg_len);
                p+=err_msg_len;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }
            else if(opt_success==EP_PARM_ERR)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x90;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pdich);
            }
        }
        break;

        case 0x0800:                    /*装置复归*/
            VI_Clear_Evt();
            opt_success=1;
            if(opt_success)
            {
                DeviceResetToLog();
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x80;
                *p++=0x02;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x9f;
                *p++=0x02;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0F40:  /* 取得IO板信息 */
            if (EP_Get_04CPU_Init_End_Flag())
            {
                for (untemp_val = 0; untemp_val<MAX_MOD_NUM; untemp_val++)
                    SIO_Mod_Info(untemp_val, &subModInfo[untemp_val]);
                opt_success = 1;
            }
            else
                opt_success = 0;

            if (opt_success)
            {
                extern uint8_t ucExtIoSum;    /*IO模块数量*/
                extern uint8_t aExtSubModInfo[MAX_MOD_NUM*5];/* 最多支持MAX_MOD_NUM个模件 */
                p = p_send_buffer;
                *p++ = 0x00;
                *p++ = 0x00;
                temp_Io_count = 0;
                for (untemp_val = 0; untemp_val<MAX_MOD_NUM-1; untemp_val++)
                {
                    if (subModInfo[untemp_val].type == IDLE_MODULE)
                        continue;
                    temp_Io_count++;
                }

                *p++ = LL8(temp_Io_count+ucExtIoSum);	/* 有效模件个数 */
                *p++ = LH8(temp_Io_count+ucExtIoSum);
                for (untemp_val = 0; untemp_val<MAX_MOD_NUM-1; untemp_val++)
                {
                    if (subModInfo[untemp_val].type == IDLE_MODULE)
                        continue;
                    *p++ = subModInfo[untemp_val].type ;  	/* 类型码 */
                    *p++ = subModInfo[untemp_val].ucHardAddr;/* 模件地址 */
                    *p++ = 0;   /* 模件地址 */
                    *p++ = subModInfo[untemp_val].ucDesignSN; /* 序列号 */
                    *p++ = LL8(subModInfo[untemp_val].unVer); /* 版本号小数位 */
                    *p++ = LH8(subModInfo[untemp_val].unVer); /* 版本号整数位 */
                    *p++ = 0x00;
                    *p++ = 0x00;
                }
                for (untemp_val = 0; untemp_val<ucExtIoSum; untemp_val++)
                {
                    *p++ = aExtSubModInfo[untemp_val*5] ;  	/* 类型码 */
                    *p++ = aExtSubModInfo[untemp_val*5+1] ;/* 模件地址 */
                    *p++ = 0;   /* 模件地址 */
                    *p++ = aExtSubModInfo[untemp_val*5+2] ; /* 序列号 */
                    *p++ = aExtSubModInfo[untemp_val*5+3] ; /* 版本号小数位 */
                    *p++ = aExtSubModInfo[untemp_val*5+4] ; /* 版本号整数位 */
                    *p++ = 0x01;    /*bit0=1表示扩展机箱IO模块*/
                    *p++ = 0x00;
                }
                *p++ = 0x00;
                *p++ = 0x00;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0xA000, send_lenth);
            }
            else
            {
                p = p_send_buffer;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x50;
                *p++ = 0x04;
                *p++ = 0x00;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
            break;

        case 0x0b00:					/*取得遥测量*/
        {
            static BOOL bLog0b00=TRUE;
            pfloat=malloc(iMeaAiNum_g*sizeof(float)+100);
            if (pfloat==NULL)
            {
                if (bLog0b00)
                {
                    bLog0b00=FALSE;
                    if(ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0b00 msg.\n", NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "解释0x0b00报文时申请内存失败.\n", NULL);
                    }
                }
                break;
            }

            if(EP_Get_04CPU_Init_End_Flag())   /*20041224*/
            {
                static BOOL bFst=TRUE;

                if(bFst)
                {
                    bFst=FALSE;
                    LOG_Dbg_Msg("开始获取遥测量!\n", 0, 0, 0, 0, 0, 0);
                }

                VI_Rd_Mea_AI_Val(pfloat);
                pf=pfloat;
                opt_success=1;
            }
            else
            {
                opt_success=0;
            }
            if(opt_success)
            {
                uint8_t *pTmpSndBuf=NULL;
                uint8_t *pStart;
                int lenth;
                lenth=10+iMeaAiNum_g*10+100;
                if(lenth>Max_Frame_Data_Lenth)
                {
                    pTmpSndBuf=malloc(lenth);
                    pStart=pTmpSndBuf;
                    if (pTmpSndBuf==NULL)
                    {
                        if (bLog0b00)
                        {
                            bLog0b00=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0b00 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x0b00报文时申请内存失败.\n", NULL);
                            }
                        }
                        free(pfloat);
                        break;
                    }
                }
                else
                {
                    pStart=p_send_buffer;
                }
                p=pStart;

                *p++=0x00;
                *p++=0x00;
                *p++=128;				/*被动读取*/
                *p++=LL8(iMeaAiNum_g);
                *p++=LH8(iMeaAiNum_g);
                for(untemp_val=0; untemp_val<iMeaAiNum_g; untemp_val++)
                {
                    *p++=LO8(untemp_val);
                    *p++=HI8(untemp_val);
                    pVI_MEA_AI_CFG=(VI_MEA_AI_CFG *)VI_Get_Mea_AI_Attr(untemp_val);
                    *p++=0x00;
                    *p++=0x00;
                    *p++=pVI_MEA_AI_CFG->ucUnit;
                    *p++=0x00;
                    ulAI_Val.fVal=*pf++;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                }/*正式代码，测试时暂时屏蔽*/

                free(pfloat);
                send_lenth=p-pStart;
                reply(new_fd,pStart,p_rcv_buffer,0x8700,send_lenth);
                if (pTmpSndBuf != NULL)
                    free(pTmpSndBuf);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x0f;
                *p++=0x01;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                free(pfloat);
            }
            break;
        }

        case 0x0a10:                    /*读取物理通道有效值*/
        {
            RD_HW_AI_MEA  pRD_HW_AI_MEA[MAXHCHNNUM];
            pHW_AI=pRD_HW_AI_MEA;
            RD_Mea_Hw_AI(pRD_HW_AI_MEA, TRUE);
            if(p_rcv_buffer[20]!=0xff)
            {
                if(p_rcv_buffer[20]<iHwAiChNum_g)
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x01;
                    *p++=0x01;
                    *p++=p_rcv_buffer[20];
                    pHW_AI+=p_rcv_buffer[20];
                    ulAI_Val.fVal=pHW_AI->fRmsVal;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fAngle;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fMean;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fGain;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    *p++=pHW_AI->ucType;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8910,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0xb0;
                    *p++=0x00;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x01;
                *p++=iHwAiChNum_g;
                for(uctemp_val=0; uctemp_val<iHwAiChNum_g; uctemp_val++)
                {
                    *p++=uctemp_val;
                    ulAI_Val.fVal=pHW_AI->fRmsVal;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fAngle;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fMean;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    ulAI_Val.fVal=pHW_AI->fGain;
                    *p++=LL8(ulAI_Val.ulVal);
                    *p++=LH8(ulAI_Val.ulVal);
                    *p++=HL8(ulAI_Val.ulVal);
                    *p++=HH8(ulAI_Val.ulVal);
                    *p++=pHW_AI->ucType;
                    pHW_AI++;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8910,send_lenth);
            }
            break;
        }

        case 0x0b10:					/*取得遥信量*/
            /*调用取得遥信量的函数*/
            if(p_rcv_buffer[20]==2)/*内部规约版本号大于等于3.80时读取全遥信*/
            {
                pbool=malloc(Max_Common_Lenth);
                punshort = malloc(Max_Common_Lenth);
                {
                    static BOOL bLog0b10=TRUE;
                    if ((pbool==NULL) || (punshort==NULL))
                    {
                        if (bLog0b10)
                        {
                            bLog0b10=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0b10 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x0b10报文时申请内存失败.\n", NULL);
                            }
                        }
                        break;
                    }
                }
                pb=pbool;
                pus=punshort;
                VI_Rd_Mea_DI_Val(pbool,punshort);
                opt_success=1;
                if(opt_success)
                {
                    iActMeaDiNum = iMeaDiNum_g;

                    p=p_send_buffer;
                    *p++=0x02;
                    *p++=0x00;
                    *p++=128;				/*被动读取*/

                    *p++=LO8(iActMeaDiNum);
                    *p++=HI8(iActMeaDiNum);

                    for(untemp_val=0; untemp_val<iActMeaDiNum; untemp_val++)
                    {
                        *p++=LO8(untemp_val);
                        *p++=HI8(untemp_val);

                        *p=*pb++;
                        if((*pus++)&0x0010)
                            *p|=0x80;
                        else
                            *p&=(~0x80);
                        p++;

                        *p++=0x00;/*保留*/
                    }
                    free(pbool);/*正式代码，测试时暂时屏蔽*/
                    free(punshort);
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8801,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x1f;
                    *p++=0x01;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                    free(pbool);
                    free(punshort);
                }
            }
            else
            {
                pbool=malloc(Max_Common_Lenth);
                punshort = malloc(Max_Common_Lenth);
                {
                    static BOOL bLog0b10=TRUE;
                    if ((pbool==NULL) || (punshort==NULL))
                    {
                        if (bLog0b10)
                        {
                            bLog0b10=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0b10 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x0b10报文时申请内存失败.\n", NULL);
                            }
                        }
                        break;
                    }
                }
                pb=pbool;
                pus=punshort;
                VI_Rd_Mea_DI_Val(pbool,punshort);
                opt_success=1;
                if(opt_success)
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=128;				/*被动读取*/

                    /* 遥信个数限制,最多传递255个
                     */
                    if (iMeaDiNum_g >= 256)
                    {
                        iActMeaDiNum = 255;
                    }
                    else
                    {
                        iActMeaDiNum = iMeaDiNum_g;
                    }

                    *p++=iActMeaDiNum;
                    for(untemp_val=0; untemp_val<iActMeaDiNum; untemp_val++)
                    {
                        *p++=untemp_val;
                        *p=*pb++;
                        if((*pus++)&0x0010)
                            *p|=0x80;
                        else
                            *p&=(~0x80);
                        p++;
                    }
                    free(pbool);/*正式代码，测试时暂时屏蔽*/
                    free(punshort);
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8800,send_lenth);
                }
                else
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x1f;
                    *p++=0x01;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                    free(pbool);
                    free(punshort);
                }
            }
            break;

        case 0x0c20:                    /*从保护CPU获得文件名*/
            opt_success=0;
            switch(p_rcv_buffer[20])
            {
                case 0x40:              /*系统配置信息文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_SYS_INFO_FILE);
                    opt_success=1;
                    break;

                case 0x4c:              /*内部定值文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_INNER_SET_FILE);
                    opt_success=1;
                    break;

                case 0x50:              /*增益系数文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_AI_GAIN_FILE);
                    opt_success=1;
                    break;

                case 0x80:              /*逻辑图顺序化文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_LGC_CFG_FILE);
                    opt_success=1;
                    break;

                case 0x84:              /*保护硬件配置文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_HW_CFG_FILE);
                    opt_success=1;
                    break;

                case 0x85:              /*保护软件配置文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_SW_CFG_FILE);
                    opt_success=1;
                    break;

                case 0x48:              /*定值区文件*/
                    strcpy(filename,"");
                    sprintf(filename,EP_SET_AREA_DIR "/area%02x.dza",p_rcv_buffer[21]);
                    opt_success=1;
                    break;

                default:
                    opt_success=0;
                    break;

                    if(opt_success)
                    {
                        if((fp=open(filename,0,0))>0)
                        {
                            close(fp);
                            p=p_send_buffer;
                            filenamelen=strlen(filename);
                            memcpy(p,p_rcv_buffer+20,17);
                            p+=17;
                            *p++=LO8(filenamelen);
                            *p++=HI8(filenamelen);
                            memcpy(p,filename,filenamelen);
                            p+=filenamelen;
                            *p++=0;
                            *p++=0;
                            send_lenth=p-p_send_buffer;
                            reply(new_fd,p_send_buffer,p_rcv_buffer,0x9130,send_lenth);
                        }
                        else
                        {
                            p=p_send_buffer;
                            memcpy(p,p_rcv_buffer+20,17);
                            p+=17;
                            *p++=0;
                            *p++=0;
                            *p++=0x04;
                            *p++=0;
                            send_lenth=p-p_send_buffer;
                            reply(new_fd,p_send_buffer,p_rcv_buffer,0x9420,send_lenth);
                        }
                    }
                    else
                    {
                        p=p_send_buffer;
                        memcpy(p,p_rcv_buffer+20,17);
                        p+=17;
                        *p++=0;
                        *p++=0;
                        *p++=0x00;
                        *p++=0;
                        send_lenth=p-p_send_buffer;
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0x9420,send_lenth);
                    }
            }
            break;

        case 0x0c10:   /* 从保护CPU读取信息文件 */
            filenamelen = U8_TO_U16(p_rcv_buffer[21], p_rcv_buffer[20]);
            strcpy(filename, "/root");
            memcpy(&filename[5], &p_rcv_buffer[22], filenamelen);
            filename[5+filenamelen] = '\0';
            if(strcmp(filename,EP_INNER_SET_FILE)==0)
            {
                uint16_t usAttr = 0;

                usAttr = U8_TO_U16(p_rcv_buffer[22+filenamelen+1], p_rcv_buffer[22+filenamelen]);
                if ((usAttr != PASSWORD_FOR_INNER_SET_READ)
                        && ((psock_fd == &new_fd3) || (psock_fd == &new_fd4)))
                {
                    return;
                }

                if(Check_Nbset_CRC()!=TRUE)
                    return;
            }
            if(strcmp(filename,EP_CK_SET_FILE)==0)
            {
                bCKset =TRUE;
                vxsts = semTake(semCkCRCIni_g, WAIT_FOREVER);
                if (vxsts != OK)
                {
                    return;
                }

                if(Check_Ckset_CRC()!=TRUE)
                {
                    vxsts=semGive(semCkCRCIni_g);
                    assert(vxsts==OK);
                    return;
                }
            }

            /* 读文件时校验功能投退状态文件有效性 */
            if (strcmp(filename, EP_FUNC_STS_FILE) == 0)
            {
                if (Check_FunSts_CRC() != TRUE)
                {
                    return;
                }
            }

            /* 读取过程层状态文件 */
            if (strcmp(filename, SMV_GO_COMM_STAT_FILE) == 0)
            {
                Smv_Go_CommStat_Chg();
            }

            strncpy(aucBuf, filename,13);
            aucBuf[13]='\0';
            if(strcmp(aucBuf,(EP_SET_AREA_DIR "/area"))==0)
            {
                if(Check_Areaset_CRC(filename)!=TRUE)
                {
                    return;
                }
            }

            if ((fp = open(filename, 0, 0644))>0)
                opt_success = 1;
            else
            {
                opt_success = 0;
            }

            if (!EP_Get_04CPU_Init_End_Flag())
            {
                if(bCKset)
                {
                    vxsts=semGive(semCkCRCIni_g);
                    assert(vxsts==OK);
                }
                break;
            }
            if (opt_success)
            {
                LastCRC = 0;

                temp_val = lseek(fp, 0, 0);
                if (temp_val == ERROR)
                {
                    LOG_Dbg_Msg("set a file read/write pointer failed.\n", 0, 0, 0, 0, 0, 0);
                }

                file_lenth = lseek(fp, 0, 2);
                if (file_lenth == ERROR)
                {
                    LOG_Dbg_Msg("set a file read/write pointer failed.\n", 0, 0, 0, 0, 0, 0);
                }

                file_lenth = file_lenth-temp_val;  /* file length. */

                temp_val = lseek(fp, 0, 0);
                if (temp_val == ERROR)
                {
                    LOG_Dbg_Msg("set a file read/write pointer failed.\n", 0, 0, 0, 0, 0, 0);
                }

                file_head_lenth = 2+2+4+filenamelen;    /* file head length. */

                rpt_lenth = file_lenth+file_head_lenth+2;		/* valid report length, 最后两字节为校验码 */

                FrameCnt = rpt_lenth/Max_Frame_Data_Lenth;
                if (rpt_lenth%Max_Frame_Data_Lenth)
                    FrameCnt++;

                read_number = 0;
                FrameSeq = 0;
                for (temp_val = 0; temp_val<FrameCnt; temp_val++)
                {
                    if (FrameSeq == 0)
                    {

                        memcpy(p_send_buffer+20, p_rcv_buffer+20, filenamelen+2);
                        p_send_buffer[20+filenamelen+2] = 0;
                        p_send_buffer[20+filenamelen+3] = 0;
                        p_send_buffer[20+filenamelen+4] = LL8(file_lenth);
                        p_send_buffer[20+filenamelen+5] = LH8(file_lenth);
                        p_send_buffer[20+filenamelen+6] = HL8(file_lenth);
                        p_send_buffer[20+filenamelen+7] = HH8(file_lenth);

                        if (FrameCnt != 1)
                        {
                            /* 多帧处理 */
                            get_number = read(fp, p_send_buffer+Frame_Head_Lenth+file_head_lenth,
                                              Max_Frame_Data_Lenth-file_head_lenth);

                            Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt,
                                            FrameSeq, Max_Frame_Data_Lenth);  /* 根据帧中数据长度计算，包括校验码 */

                            LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth+file_head_lenth,
                                                     get_number, 0);  /* 根据实际数据长度计算 */

                            if (Max_Frame_Data_Lenth-file_head_lenth > get_number)
                            {
                                /* One bytes more, Has CRC Low byte */
                                p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth-1] = LO8(LastCRC);
                            }

                            /* 校验码放最后一个字节 */
                            p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth] = Cal_CheckSum(p_send_buffer,
                                    Frame_Head_Lenth+Max_Frame_Data_Lenth);

                            retcode = write(new_fd, p_send_buffer, Max_Frame_Lenth);

                            if (retcode != Max_Frame_Lenth)
                            {
                                close(fp);
                                opt_success = 0;
                                LOG_Dbg_Msg("file transmit not complete.\n", 0, 0, 0, 0, 0, 0);
                                break;
                            }

                            if (new_fd != new_fd4)
                                taskDelay(2);
                            FrameSeq++;
                        }
                        else
                        {
                            /* 单帧处理 */
                            get_number = read(fp, p_send_buffer+Frame_Head_Lenth+file_head_lenth, file_lenth);
                            Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt,
                                            FrameSeq, rpt_lenth);

                            LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth+file_head_lenth, file_lenth, 0);
                            p_send_buffer[Frame_Head_Lenth+file_head_lenth+file_lenth] = LO8(LastCRC);
                            p_send_buffer[Frame_Head_Lenth+file_head_lenth+file_lenth+1] = HI8(LastCRC);
                            p_send_buffer[Frame_Head_Lenth+file_head_lenth+file_lenth+2]
                                = Cal_CheckSum(p_send_buffer, Frame_Head_Lenth+file_head_lenth+file_lenth+2);

                            retcode = write(new_fd, p_send_buffer, Frame_Head_Lenth+file_head_lenth+file_lenth+3);
                            if (retcode != (Frame_Head_Lenth+file_head_lenth+file_lenth+3) )
                            {
                                /* 增加错误判断 */
                                close(fp);
                                opt_success = 0;
                                LOG_Dbg_Msg("file transmit not complete.\n", 0, 0, 0, 0, 0, 0);
                            }
                            else
                            {
                                close(fp);
                                opt_success = 1;
                            }
                        }
                        read_number += get_number;

                    }
                    else
                    {
                        if (FrameSeq == FrameCnt-2)
                        {
                            /* 倒数第二帧 */
                            get_number = read(fp, p_send_buffer+Frame_Head_Lenth, Max_Frame_Data_Lenth);
                            if (get_number == Max_Frame_Data_Lenth - 1)
                            {
                                /* One byte more. */
                                Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt, FrameSeq, Max_Frame_Data_Lenth);
                                LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth, get_number, LastCRC);
                                p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth - 1] = LO8(LastCRC);
                                p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth] = Cal_CheckSum(p_send_buffer,
                                        Frame_Head_Lenth+Max_Frame_Data_Lenth);
                            }
                            else
                            {
                                Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt, FrameSeq, Max_Frame_Data_Lenth);
                                LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth, Max_Frame_Data_Lenth, LastCRC);
                                p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth] = Cal_CheckSum(p_send_buffer,
                                        Frame_Head_Lenth+Max_Frame_Data_Lenth);
                            }

                            retcode = write(new_fd, p_send_buffer, Max_Frame_Lenth);
                            if (retcode != Max_Frame_Lenth )
                            {
                                close(fp);
                                opt_success = 0;
                                LOG_Dbg_Msg("file transmit not complete.\n", 0, 0, 0, 0, 0, 0);
                                break;
                            }

                            if (new_fd != new_fd4)
                                taskDelay(2);
                            FrameSeq++;
                        }
                        else if (FrameSeq == FrameCnt-1)
                        {
                            /* 最后一帧 */
                            get_number = read(fp, p_send_buffer+Frame_Head_Lenth, Max_Frame_Data_Lenth);

                            if (get_number>0)
                            {
                                /* have bytes. */
                                Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt, FrameSeq, get_number+2);

                                LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth, get_number, LastCRC);
                                p_send_buffer[Frame_Head_Lenth+get_number] = LO8(LastCRC);
                                p_send_buffer[Frame_Head_Lenth+get_number+1] = HI8(LastCRC);
                                p_send_buffer[Frame_Head_Lenth+get_number+2] = Cal_CheckSum(p_send_buffer, Frame_Head_Lenth+get_number+2);
                                CrcLen = 2;
                            }
                            else
                            {
                                /* get_number = 0; */
                                if ((rpt_lenth % Max_Frame_Data_Lenth) == 1)
                                {
                                    /* One byte more. */
                                    CrcLen = 1;
                                    p_send_buffer[Frame_Head_Lenth] = HI8(LastCRC);
                                }
                                else
                                {
                                    p_send_buffer[Frame_Head_Lenth] = LO8(LastCRC);
                                    p_send_buffer[Frame_Head_Lenth+1] = HI8(LastCRC);
                                    CrcLen = 2;
                                }

                                Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt, FrameSeq, CrcLen);
                                p_send_buffer[Frame_Head_Lenth+CrcLen] = Cal_CheckSum(p_send_buffer,
                                        Frame_Head_Lenth+CrcLen);
                            }

                            retcode = write(new_fd, p_send_buffer, Frame_Head_Lenth + get_number + CrcLen + 1);
                            if (retcode != (Frame_Head_Lenth + get_number + CrcLen + 1) )
                            {
                                close(fp);
                                opt_success = 0;
                                LOG_Dbg_Msg("file transmit not complete.\n", 0, 0, 0, 0, 0, 0);
                            }
                            else
                            {
                                close(fp);
                                opt_success = 1;
                            }
                        }
                        else
                        {
                            /* 中间帧 */
                            get_number = read(fp, p_send_buffer+Frame_Head_Lenth, Max_Frame_Data_Lenth);
                            Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9120, rpt_lenth, FrameCnt, FrameSeq, Max_Frame_Data_Lenth);
                            LastCRC = EP_CCITT_CRC16(p_send_buffer+Frame_Head_Lenth, Max_Frame_Data_Lenth, LastCRC);
                            p_send_buffer[Frame_Head_Lenth+Max_Frame_Data_Lenth] = Cal_CheckSum(p_send_buffer,
                                    Frame_Head_Lenth+Max_Frame_Data_Lenth);

                            retcode = write(new_fd, p_send_buffer, Max_Frame_Lenth);
                            if (retcode<0)
                            {
                                close(fp);
                                LOG_Dbg_Msg("file transmit not complete.\n", 0, 0, 0, 0, 0, 0);
                                break;
                            }
                            if (new_fd != new_fd4)
                                taskDelay(2);
                            FrameSeq++;
                        }
                        read_number += get_number;
                    }
                }
                if(bCKset)
                {
                    vxsts=semGive(semCkCRCIni_g);
                    assert(vxsts==OK);
                }
            }
            else
            {
                if(bCKset)
                {
                    vxsts=semGive(semCkCRCIni_g);
                    assert(vxsts==OK);
                }
                p = p_send_buffer;
                *p++ = LO8(filenamelen);
                *p++ = HI8(filenamelen);
                memcpy(p, filename, filenamelen);
                p += filenamelen;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x00;
                send_lenth = p-p_send_buffer;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x9410, send_lenth);
            }
            break;

        case 0x0960:                    /*固化定值区文件*/
            /*LOG_Dbg_Msg("rcved report for solidify set\n",0,0,0,0,0,0);*/
            strcpy(old_tmp_filename,"");
            strcpy(back_filename,"");
            filenamelen=U8_TO_U16(p_rcv_buffer[24],p_rcv_buffer[23]);
            strcpy(filename,"");
            memcpy(filename,&p_rcv_buffer[25],filenamelen);
            filename[filenamelen]='\0';
            Bak_File_Name(old_tmp_filename,filename,".bak");
            bAreaSetChkFlg = SC_Get_AreaSet_ChkFlg();
            /*2008-2-21 DQ 当处于启动态，若无有效定值区，也开放可以进行定值整定*/
            if( (!(uiEdpStatus_g & HW_TEST_MODE))&& /* (!VI_Is_Fault()||(VI_Is_Fault()&&!bSetIsValid_g)) && */ (fp=open(old_tmp_filename,0,0))>0 && (bAreaSetChkFlg==FALSE) )
            {
                if(SC_Is_Valid_Set(fp))		/* SET_AREA_START_NO目前值为0，不需要判断 */
                {

                    close(fp);

                    /* 参数校验 */
                    opt_success = 1;

                    if (pParaCheckFun)
                    {
                        if (pParaCheckFun() == FALSE)
                        {
                            SC_Reset_Set();
                            opt_success = 0;
                        }
                    }

                    if (opt_success == 1)
                    {
                        if(FT_Is_File(filename))
                        {
                            strcpy(back_filename,"");
                            Bak_File_Name(back_filename,filename,".old");
                            if(FT_Is_File(back_filename))
                                remove(back_filename);

                            Set_Areaset_Wr_Sts(p_rcv_buffer[20], 1); /* 定值写入 */

                            if(rename(filename,back_filename)!=OK)
                            {
                                Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                            }
                            else
                            {
                                if(rename(old_tmp_filename,filename)==OK)
                                {
                                    /* 判断更新定值是否正常 */
                                    if (SC_End_Wr_Set(p_rcv_buffer[20],back_filename) == EP_SUCCESS)
                                    {
                                        Write_Areaset_CRC(filename,p_rcv_buffer[20]);
                                        Set_Areaset_Wr_Sts(p_rcv_buffer[20], 0); /* 定值写入结束 */
                                        RE_SetLogSetChgCnt();/* 文件操作结束后更新计数 */
                                        Solid_Set_OK(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                                    }
                                    else
                                    {
                                        /* 更新失败则删除已更换名称文件 */
                                        if (FT_Is_File(filename))
                                            remove(filename);
                                        rename(back_filename, filename);

                                        Solid_Set_Err(new_fd, p_send_buffer, p_rcv_buffer, send_lenth);
                                    }

                                    /* 删除临时文件 */
                                    if(FT_Is_File(back_filename))
                                        remove(back_filename);
                                }
                                else
                                {
                                    rename(back_filename,filename);
                                    Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                                }
                            }
                        }
                        else
                        {
                            Set_Areaset_Wr_Sts(p_rcv_buffer[20], 1);  /* 定值写入 */
                            if(rename(old_tmp_filename,filename)==OK)
                            {
                                /* 判断更新定值是否正常 */
                                if (SC_End_Wr_Set(p_rcv_buffer[20],SET_BACK_FILE_NONE) == EP_SUCCESS)
                                {
                                    Write_Areaset_CRC(filename,p_rcv_buffer[20]);
                                    Set_Areaset_Wr_Sts(p_rcv_buffer[20], 0); /* 写入结束 */
                                    RE_SetLogSetChgCnt();/* 文件操作结束后更新计数 */
                                    Solid_Set_OK(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                                }
                                else
                                {
                                    Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                                }

                            }
                            else
                            {
                                Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                            }
                        }
                    }
                    else
                    {
                        Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                    }
                }
                else
                {
                    close(fp);
                    Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                }
            }
            else
            {
                if(bAreaSetChkFlg==TRUE )   /* 正在校验定值区文件，不接受整定 */
                {
                    LOG_Write(LOG_KERNEL, "正在校验保护定值区文件，禁止整定定值.\n", NULL);
                    Solid_Set_Unknown_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                }
                else
                {
                    Solid_Set_Err(new_fd, p_send_buffer,p_rcv_buffer,send_lenth);
                }
            }
            break;

        case 0x0900:                    	/*读取运行定值区号*/
            opt_success=1;
            /*if(opt_success)*/				/*del 20050513*/
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                /*LOG_Dbg_Msg(" read working set area num success \n",0,0,0,0,0,0);*/
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=SC_Work_Set_Area();
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8a10,send_lenth);

            }
            else
            {
                LOG_Dbg_Msg(" read working set area num error \n",0,0,0,0,0,0);
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x9f;
                *p++=0x01;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0c11:
            p=p_rcv_buffer+20;
            filenamelen=U8_TO_U16(p[1],p[0]);
            p+=2;
            memcpy(filename,p,filenamelen);
            filename[filenamelen]='\0';
            p+=filenamelen;
            StartPos=U8_TO_U32(p[3],p[2],p[1],p[0]);
            p+=4;
            ReadLenth=U8_TO_U32(p[3],p[2],p[1],p[0]);


            if((fp=open(filename,0,0644))>0) opt_success=1;
            else opt_success=0;
            if(opt_success)
            {
                LastCRC=0;
                temp_val=lseek(fp,StartPos,0);
                file_lenth=lseek(fp,0,2);
                file_lenth=file_lenth-temp_val;
                if(file_lenth>=ReadLenth)
                {
                    uint8_t *psendTmp;
                    psendTmp=malloc(ReadLenth+200);
                    if (psendTmp==NULL)
                    {
                        static BOOL bLog0c11=TRUE;
                        if (bLog0c11)
                        {
                            bLog0c11=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0c11 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x0c11报文时申请内存失败.\n", NULL);
                            }
                        }
                        break;
                    }
                    p=psendTmp;

                    temp_val=lseek(fp,StartPos,0);
                    *p++=LO8(filenamelen);
                    *p++=HI8(filenamelen);
                    memcpy(p,filename,filenamelen);
                    p+=filenamelen;
                    *p++=LL8(StartPos);
                    *p++=LH8(StartPos);
                    *p++=HL8(StartPos);
                    *p++=HH8(StartPos);
                    *p++=LL8(ReadLenth);
                    *p++=LH8(ReadLenth);
                    *p++=HL8(ReadLenth);
                    *p++=HH8(ReadLenth);
                    *p++=0x00;
                    *p++=0x00;
                    retcode=read(fp,p,ReadLenth);
                    if(retcode!=ReadLenth)
                    {
                        p=psendTmp;
                        memcpy(p,p_rcv_buffer+20,filenamelen+12);
                        p+=filenamelen+12;
                        *p++=0x04;
                        *p++=0x00;
                        send_lenth=p-psendTmp;
                        reply(new_fd,psendTmp,p_rcv_buffer,0x9411,send_lenth);
                    }
                    else
                    {
                        LastCRC=EP_CCITT_CRC16(p,ReadLenth,0);
                        p+=ReadLenth;
                        *p++=LO8(LastCRC);
                        *p++=HI8(LastCRC);
                        send_lenth=p-psendTmp;
                        reply(new_fd,psendTmp,p_rcv_buffer,0x9121,send_lenth);
                    }
                    free(psendTmp);
                }
                else
                {
                    p=p_send_buffer;
                    memcpy(p,p_rcv_buffer+20,filenamelen+12);
                    p+=filenamelen+12;
                    *p++=0x08;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x9411,send_lenth);
                }
                close(fp);
            }
            else
            {
                p=p_send_buffer;
                memcpy(p,p_rcv_buffer+20,filenamelen+12);
                p+=filenamelen+12;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9411,send_lenth);
            }
            break;

        case 0x0c40:                    /* 从保护CPU根据文件名获取文件详细信息 */
            p=p_rcv_buffer+20;
            filenamelen=U8_TO_U16(p[1],p[0]);
            if(filenamelen>1024)
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x3f;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                break;
            }
            p+=2;
            memcpy(filename,p,filenamelen);
            filename[filenamelen]='\0';		/* 添加结束字符 */
            p+=filenamelen;

            if(stat((uint8_t*)filename, &Stat) == OK)
            {
                /* 该文件存在 */
                opt_success=1;
            }
            else
            {
                opt_success=0;
            }

            if(opt_success)
            {
                p=p_send_buffer;
                *p++=0;
                if((Stat.st_mode & S_IFMT) == S_IFREG)
                {
                    *p++=1;
                }
                else
                {
                    *p++=0;
                }
                *p++=LO8(filenamelen);
                *p++=HI8(filenamelen);
                memcpy(p,filename,filenamelen);
                p+=filenamelen;

                timer=Stat.st_mtime;
                ftime=gmtime(&timer);
                *p++=LO8((uint16_t)(ftime->tm_year+1900));
                *p++=HI8((uint16_t)(ftime->tm_year+1900));
                *p++=ftime->tm_mon+1;
                *p++=ftime->tm_mday;
                *p++=ftime->tm_hour;
                *p++=ftime->tm_min;
                *p++=ftime->tm_sec;
                if((Stat.st_mode & S_IFMT) == S_IFREG)
                {
                    *p++=LL8(Stat.st_size);
                    *p++=LH8(Stat.st_size);
                    *p++=HL8(Stat.st_size);
                    *p++=HH8(Stat.st_size);
                }
                else
                {
                    *p++=LL8(0);
                    *p++=LH8(0);
                    *p++=HL8(0);
                    *p++=HH8(0);
                }

                for(i=0; i<32; i++)
                    *p++=0;

                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9150,send_lenth);

            }
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x30;
                *p++=0x05;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0910:                    /*读取所有有效的定值区号*/
            opt_success=1;
            if(opt_success)
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                ValidAreaNum=SC_Get_Valid_Area(pucValidArea);
                *p++=ValidAreaNum;      /**p++=有效定值区个数*/
                for(temp_val=0; temp_val<ValidAreaNum; temp_val++)
                {
                    *p++=pucValidArea[temp_val];
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8a00,send_lenth);

            }
            else
            {
                p=p_send_buffer;
                *p++=0;
                *p++=0;
                *p++=0x8f;
                *p++=0x01;
                *p++=0;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0a00:
            p=p_send_buffer;
            retcode=RC_Start_Lubo_Sample(filename,&sn);
            if(retcode==EP_SUCCESS)
            {
                filenamelen=strlen(filename);
                *p++=0x00;
                *p++=0x00;
                *p++=LO8(filenamelen);
                *p++=HI8(filenamelen);
                memcpy(p,filename,filenamelen);
                p+=filenamelen;
                *p++=sn;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8900,send_lenth);
            }
            else
            {
                *p++=0x00;
                *p++=0x00;
                *p++=0xaf;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0b20:					/* 读取脉冲量/电度量 */
            opt_success = 1;
            iLgcPoChNum = GetPoCfgNum(); /*一般iLgcPoChNum为几十，不可能超过400*/
            if (iLgcPoChNum >=256)
            {
                static BOOL bLog0b20=TRUE;
                if (bLog0b20)
                {
                    bLog0b20=FALSE;
                    if(ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "error:LgcPoChNum must be less than 256 when deal 0x0b20 msg!\n", NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "脉冲量个数不能超过255.\n", NULL);
                    }
                }
                break;
            }
            send_lenth = iLgcPoChNum*14+5;
            if (opt_success)
            {
                RD_PO_MEA ppomeaRslt[256];

                p = p_send_buffer;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 128;		 /* 发送原因：被动读取 */
                *p++ = LO8(iLgcPoChNum);
                *p++ = HI8(iLgcPoChNum);
                RD_Mea_Po(ppomeaRslt);
                for (i = 0; i<iLgcPoChNum; i++)
                {
                    FLT_U32_UNION ulAI_Val;

                    *p++ = (ppomeaRslt+i)->ucType;
                    *p++ = (uint8_t)i;
                    ulAI_Val.fVal = (ppomeaRslt+i)->fVal;
                    *p++ = LL8(ulAI_Val.ulVal);
                    *p++ = LH8(ulAI_Val.ulVal);
                    *p++ = HL8(ulAI_Val.ulVal);
                    *p++ = HH8(ulAI_Val.ulVal);
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                    *p++=0;
                }
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8500, send_lenth);
            }
            else
            {
                send_lenth = 5;
                p = p_send_buffer;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x2f;
                *p++ = 0x01;
                *p++ = 0x00;
                reply(new_fd, p_send_buffer, p_rcv_buffer, 0x8e00, send_lenth);
            }
            break;

        case 0x0c80:                    /*读取保护功能当前投退状态*/
            if(EP_Get_04CPU_Init_End_Flag())/*add 20050513*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=(uint8_t)(iSubLgcNum_g);
                for(i=0; i<iSubLgcNum_g; i++)
                {
                    pSC_SUB_LGC_ITEM=(SC_SUB_LGC_ITEM *)SC_Get_Sub_Lgc_Attr(i);
                    *p++=strlen(pSC_SUB_LGC_ITEM->aucName);
                    memcpy(p,pSC_SUB_LGC_ITEM->aucName,strlen(pSC_SUB_LGC_ITEM->aucName));
                    p+=strlen(pSC_SUB_LGC_ITEM->aucName);
                    *p++=pSC_SUB_LGC_ITEM->bRun;
                    *p++=0x00;
                    *p++=0x00;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9480,send_lenth);
            }
            else						/*add 20050513*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x4f;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }

            break;

        case 0x0d80:                    /*读取指示灯状态*/
            pbool=malloc(Max_Common_Lenth);
            {
                static BOOL bLog0d80=TRUE;
                if (pbool==NULL)
                {
                    if (bLog0d80)
                    {
                        bLog0d80=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0d80 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x0d80报文时申请内存失败.\n", NULL);
                        }
                    }
                    break;
                }
            }
            RD_Rd_Hw_Led_Val(pbool);
            pb=pbool;
            opt_success=1;
            if(opt_success)
            {
                ph=p_send_buffer;
                p=ps;
                hNum=0;
                sNum=0;
                for(untemp_val=0; untemp_val<iHwLedChNum_g; untemp_val++)
                {
                    pRD_LGC_LED_CH=(RD_LGC_LED_CH *)RD_Get_Hw_Led_Attr(untemp_val);
                    if(pRD_LGC_LED_CH->bIsHwLED)
                    {
                        hNum++;
                        *ph++=untemp_val;
                        *ph++=0x00;
                        *ph++=0x00;
                        *ph++=0x00;
                        *ph++=0x00;
                        *ph++=*pb++;

                    }
                    else
                    {
                        sNum++;
                        *p++=untemp_val;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=*pb++;
                    }
                }
                p=ph;
                *p++=0x00;
                *p++=0x00;
                *p++=128;
                *p++=hNum;
                memcpy(p,p_send_buffer,hNum*6);
                p+=hNum*6;
                *p++=sNum;
                memcpy(p,ps,sNum*6);
                p+=sNum*6;
                send_lenth=p-ph;
                reply(new_fd,ph,p_rcv_buffer,0x8780,send_lenth);
            }
            free(pbool);
            break;

        case 0x0d40:                    /* 读取某目录下的所有文件列表 */
            untemp_val=0;
            file_num=0;
            strcpy(dirname, "");
            dirnamelen=U8_TO_U16(p_rcv_buffer[23], p_rcv_buffer[22]);
            memcpy(dirname, p_rcv_buffer+24, dirnamelen);
            dirname[dirnamelen]='\0';

            LOG_Dbg_Msg("Begin to read directory %s!\n", (int)dirname, 0, 0, 0, 0, 0);

            ucDirType=GetDirType(dirname);		/* 获取目录类型  */

            if(ucDirType == 1)
            {
                /* 录波目录  */
                bDirExistFlag=TRUE;
            }
            else if(ucDirType == 2)
            {
                /* 事件目录 */
                bDirExistFlag=TRUE;
            }
            else
            {
                /* 其他目录 */

                pdir=opendir(dirname);		/* 打开目录 */
                if(pdir)
                {
                    bDirExistFlag=TRUE;				/* 目录存在 */
                }
                else
                {
                    bDirExistFlag=FALSE;			/* 目录不存在 */
                }
            }

            if(bDirExistFlag)
            {
                /* 目录存在，或链表有数据 */
                char *pAllDataBuffer = NULL;		/* 分配所有数据存储空间 */
                char *pAll = NULL;
                static BOOL bLog0d40=TRUE;

                if(ucDirType == 0)
                {
                    /* 查找具体目录  */
                    rpt_lenth=0;		/* 报告长度  */
                    rpt_lenth += (6+dirnamelen);			/* 报告头长度 */
                    while ((pent=readdir(pdir)) != NULL)
                    {
                        file_num++; 	/* 总文件数 */
                        temp_val=strlen(pent->d_name);
                        rpt_lenth += (15+temp_val);    /* 每一个文件报告长度，得此报告总字节数  */
                    }

                    closedir(pdir);
                    FrameCnt=rpt_lenth/Max_Frame_Data_Lenth;		/* 帧数 */
                    if(rpt_lenth%Max_Frame_Data_Lenth)
                    {
                        /* 增加一帧 */
                        FrameCnt++;
                    }

                    FrameSeq=0;		/* 第一帧 */
                    pdir=opendir(dirname);
                    if(pdir)
                    {
                        /* 打开目录名填充 */
                        untemp_val=0;
                        get_number=0;

                        pAllDataBuffer=malloc(rpt_lenth+100);
                        if (pAllDataBuffer==NULL)
                        {
                            if (bLog0d40)
                            {
                                bLog0d40=FALSE;
                                if(ENG_MODE == 1)
                                {
                                    LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0d40 msg.\n", NULL);
                                }
                                else if(ENG_MODE == 0)
                                {
                                    LOG_Write(LOG_KERNEL, "解释0x0d40报文时申请内存失败.\n", NULL);
                                }
                            }
                            closedir(pdir);
                            break;
                        }
                        pAll=pAllDataBuffer;

                        *pAll++=p_rcv_buffer[20];		/* 是否是详细文件信息 */
                        *pAll++=0x00 ; 		/* 保留 */
                        memcpy(pAll, p_rcv_buffer+22, dirnamelen+2);		/* 长度和文件名 */
                        pAll += dirnamelen+2;
                        *pAll++=LO8(file_num);			/* 文件和子目录个数 */
                        *pAll++=HI8(file_num);

                        while((pent=readdir(pdir)) != NULL)
                        {
                            untemp_val++;
                            get_number++;
                            memcpy(filename, dirname, dirnamelen);
                            filename[dirnamelen]='/';
                            temp_val=strlen(pent->d_name);             /* 文件名长度 */
                            memcpy(filename+dirnamelen+1, pent->d_name, temp_val);
                            filename[dirnamelen+1+temp_val]='\0';
                            if((p_rcv_buffer[20] == 0) || (p_rcv_buffer[20] == 1)) 		/* 文件详细信息，临时，本来为0 */
                            {
                                /* 详细信息和部分详细信息 */
                                if(stat(filename, &pStat) == OK)
                                {
                                    bFileExistFlag=TRUE;
                                }
                                else
                                {
                                    bFileExistFlag=FALSE;
                                }

                                if(((pStat.st_mode & S_IFMT) == S_IFREG) && bFileExistFlag)
                                    *pAll++=0x01;
                                else
                                    *pAll++=0x00; 		/* 目录 */
                            }
                            else
                            {
                                *pAll++=1;		/* 默认为文件 */
                            }

                            *pAll++=temp_val;		/* 只使用一个字节 */
                            memcpy(pAll, pent->d_name, temp_val);
                            pAll += temp_val;
                            if(p_rcv_buffer[20] == 0) 		/* 文件详细信息，临时，本来为0 */
                            {
                                if(bFileExistFlag)
                                {
                                    timer=pStat.st_mtime;
                                    ftime=gmtime(&timer);
                                    *pAll++=LO8((uint16_t)(ftime->tm_year+1900));
                                    *pAll++=HI8((uint16_t)(ftime->tm_year+1900));
                                    *pAll++=ftime->tm_mon+1;
                                    *pAll++=ftime->tm_mday;
                                    *pAll++=ftime->tm_hour;
                                    *pAll++=ftime->tm_min;
                                    *pAll++=ftime->tm_sec;
                                    *pAll++=LL8(pStat.st_size);
                                    *pAll++=LH8(pStat.st_size);
                                    *pAll++=HL8(pStat.st_size);
                                    *pAll++=HH8(pStat.st_size);
                                    *pAll++=0x00;
                                    *pAll++=0x00;
                                }
                                else
                                {
                                    for(i=0; i<13; i++)
                                        *pAll++=0;
                                }
                            }
                            else  /* 只读文件名称 */
                            {
                                for(i=0; i<13; i++)
                                    *pAll++=0;
                            }
                        }

                        closedir(pdir);
                    }
                    else    /* 目录打不开 */
                    {
                        LOG_Dbg_Msg("Can not open the directory %s!\n", (int)dirname, 0, 0, 0, 0, 0);
                        p=p_send_buffer;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x30;		/* 出错 */
                        *p++=0x04;
                        *p++=0x00;
                        send_lenth=p-p_send_buffer;
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                    }
                }
                else if((ucDirType == REC_DIR) || (ucDirType == EVT_DIR))
                {
                    STATUS vxsts;

                    if(ucDirType == REC_DIR)
                    {
                        /* 录波目录  */
                        vxsts=semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
                        assert(vxsts == OK);

                        pList=pmRecFileList_g;
                    }
                    else if(ucDirType == EVT_DIR)
                    {
                        /* 事件目录 */
                        vxsts=semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
                        assert(vxsts == OK);

                        pList=pmEvtFileList_g;
                    }

                    assert(pList);

                    rpt_lenth=0;		/* 报告长度  */
                    rpt_lenth += (6+dirnamelen);			/* 报告头长度 */

                    file_num = 0;
                    for (pFileNode = (FILENODE *)lstFirst(pList);
                            pFileNode != NULL; pFileNode = (FILENODE *)lstNext((NODE*)pFileNode))
                    {
                        if ((pFileNode->uiSwCfgFileCRC != VER_ExtGetSwCfgCRC())
                                && (ucDirType == REC_DIR))
                        {
                            continue;
                        }

                        file_num++;
                        rpt_lenth += (15+pFileNode->ulNameLength);    /* 每一个文件报告长度，得此报告总字节数  */
                    }

                    FrameCnt=rpt_lenth/Max_Frame_Data_Lenth;		/* 帧数 */
                    if(rpt_lenth%Max_Frame_Data_Lenth)
                    {
                        /* 增加一帧  */
                        FrameCnt++;
                    }

                    FrameSeq=0;		/* 第一帧 */

                    untemp_val=0;
                    get_number=0;

                    pAllDataBuffer=malloc(rpt_lenth+100);
                    if (pAllDataBuffer==NULL)
                    {
                        if (bLog0d40)
                        {
                            bLog0d40=FALSE;
                            if(ENG_MODE == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0d40 msg.\n", NULL);
                            }
                            else if(ENG_MODE == 0)
                            {
                                LOG_Write(LOG_KERNEL, "解释0x0d40报文时申请内存失败.\n", NULL);
                            }
                        }
                        if(ucDirType == REC_DIR)
                        {
                            /* 录波目录  */
                            vxsts=semGive(semRecFileListWR_g);
                            assert(vxsts == OK);
                        }
                        else if(ucDirType == EVT_DIR)
                        {
                            /* 事件目录 */
                            vxsts=semGive(semEvtFileListWR_g);
                            assert(vxsts == OK);
                        }
                        break;
                    }
                    pAll=pAllDataBuffer;

                    /* 开始填写报文 */
                    *pAll++=p_rcv_buffer[20];		/* 是否是详细文件信息 */
                    *pAll++=0x00 ; 		/* 保留 */
                    memcpy(pAll, p_rcv_buffer+22, dirnamelen+2);		/* 长度和文件名 */
                    pAll += dirnamelen+2;
                    *pAll++=LO8(file_num);			/* 文件和子目录个数 */
                    *pAll++=HI8(file_num);
                    for (pFileNode = (FILENODE *)lstFirst(pList);
                            pFileNode != NULL; pFileNode = (FILENODE *)lstNext((NODE*)pFileNode))
                    {
                        if ((pFileNode->uiSwCfgFileCRC != VER_ExtGetSwCfgCRC())
                                && (ucDirType == REC_DIR))
                        {
                            continue;
                        }

                        untemp_val++;
                        get_number++;

                        *pAll++=pFileNode->ucFileType;		/* 文件类型 */
                        *pAll++=pFileNode->ulNameLength;			/* 文件长度，只一个字节 */
                        memcpy(pAll, pFileNode->ucFileName, pFileNode->ulNameLength);			/* 使用不带目录的名称 */
                        pAll += pFileNode->ulNameLength;

                        if(p_rcv_buffer[20] == 0) 		/* 文件详细信息，临时，本来为0 */
                        {
                            timer=pFileNode->timer;
                            ftime=gmtime(&timer);
                            *pAll++=LO8((uint16_t)(ftime->tm_year+1900));
                            *pAll++=HI8((uint16_t)(ftime->tm_year+1900));
                            *pAll++=ftime->tm_mon+1;
                            *pAll++=ftime->tm_mday;
                            *pAll++=ftime->tm_hour;
                            *pAll++=ftime->tm_min;
                            *pAll++=ftime->tm_sec;
                            *pAll++=LL8(pFileNode->ulSize);
                            *pAll++=LH8(pFileNode->ulSize);
                            *pAll++=HL8(pFileNode->ulSize);
                            *pAll++=HH8(pFileNode->ulSize);
                            *pAll++=0x00;
                            *pAll++=0x00;
                        }
                        else  /* 只读文件名称 */
                        {
                            for(i=0; i<13; i++)
                                *pAll++=0;
                        }
                    }
                    if(ucDirType == REC_DIR)
                    {
                        vxsts=semGive(semRecFileListWR_g);
                        assert(vxsts == OK);
                    }
                    else if(ucDirType == EVT_DIR)
                    {
                        vxsts=semGive(semEvtFileListWR_g);
                        assert(vxsts == OK);
                    }
                }

                if(rpt_lenth != (pAll-pAllDataBuffer))
                {
                    LOG_Dbg_Msg("Read directory %s fail!\n", (int)dirname, 0, 0, 0, 0, 0);
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x30;		/* 失败 */
                    *p++=0x04;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                    free(pAllDataBuffer);
                    break;
                }
                else
                {
                    pAll=pAllDataBuffer;		/* 开始发送 */
                    for(i=0; i<FrameCnt; i++)
                    {
                        /* 按帧发 */
                        if(i != (FrameCnt-1))
                            frame_lenth=Max_Frame_Data_Lenth;
                        else
                        {
                            uint16_t rpt_lenth_res;

                            rpt_lenth_res = rpt_lenth % Max_Frame_Data_Lenth;
                            frame_lenth = rpt_lenth_res ? rpt_lenth_res : Max_Frame_Data_Lenth;
                        }

                        Create_Rpt_Head(p_send_buffer, p_rcv_buffer, 0x9540, rpt_lenth, FrameCnt,
                                        FrameSeq, frame_lenth);	/* 产生帧头 */

                        memcpy(p_send_buffer+20, pAll, frame_lenth);		/* 帧数据 */
                        pAll += frame_lenth;

                        p_send_buffer[Frame_Head_Lenth+frame_lenth]=Cal_CheckSum(p_send_buffer,
                                Frame_Head_Lenth+frame_lenth);

                        retcode=write(new_fd, p_send_buffer, Frame_Head_Lenth+frame_lenth+1);
                        taskDelay(2);
                        if(retcode<0)
                        {
                            /* free(pAllDataBuffer); */ /* hchj delete 20080229 dy */
                            LOG_Dbg_Msg("file list %stransmit not complete\n",(int)dirname,0,0,0,0,0);

                            break;
                        }

                        FrameSeq++;
                    }
                    free(pAllDataBuffer);
                    LOG_Dbg_Msg("Read directory success!\n", 0, 0, 0, 0, 0, 0);
                }
            }
            else
            {
                /* 如果目录不存在 */
                LOG_Dbg_Msg("The directory %s do not exist!\n", (int)dirname, 0, 0, 0, 0, 0);
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x30;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0e40:                    /*设置装置注册码*/
            p=&p_rcv_buffer[22];
            Reg_Code[0]=U8_TO_U32(p[0],p[1],p[2],p[3]); /*此处顺序特殊*/
            Reg_Code[1]=U8_TO_U32(p[4],p[5],p[6],p[7]); /*此处顺序特殊*/
            retcode=1;
            if(retcode>=0)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x40;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x44;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0e80:                    /*读取装置序列号*/
            retcode=EP_SUCCESS;
            if(retcode==EP_SUCCESS)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                memcpy(p,Serials_SN,8);
                p+=8;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9640,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x48;
                *p++=0x04;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0ec0:                    /*读取所有事件状态*/
            p=p_send_buffer;
            *p++=0x00;
            *p++=0x00;
            *p++=LO8(SysMaxErrNum_g+iEvtNum_g);
            *p++=HI8(SysMaxErrNum_g+iEvtNum_g);
            for(untemp_val=0; untemp_val<MAX_SYS_ERR_NUM; untemp_val++)
            {
                if (SysErrEnableFlag_g & (1LL << untemp_val))
                {
                    *p++=LO8(untemp_val);
                    *p++=HI8(untemp_val);
                    *p++=ER_Sys_Err_Sts(untemp_val)?0:1;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x00;
                }
            }
            for(untemp_val=0; untemp_val<iEvtNum_g; untemp_val++)
            {
                pVI_EVT_CFG=(VI_EVT_CFG *)VI_Get_Evt_Attr(untemp_val);
                *p++=LO8(pVI_EVT_CFG->unCode);
                *p++=HI8(pVI_EVT_CFG->unCode);
                *p++=pVI_EVT_CFG->bStsNow?0:1;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;
            }
            send_lenth=p-p_send_buffer;
            reply(new_fd,p_send_buffer,p_rcv_buffer,0x96c0,send_lenth);
            break;

        case 0x0c30:   /* 删除文件 */

            p = p_send_buffer;
            strcpy(filename, "");
            filenamelen = U8_TO_U16(p_rcv_buffer[21], p_rcv_buffer[20]);
            memcpy(filename, p_rcv_buffer+22, filenamelen);
            filename[filenamelen] = '\0';
            fp = open(filename, 0, 0);	/* 打开文件 */
            if (fp>0)
            {
                close(fp);

                ucDirType = GetFileType(filename);	/* 文件类型 */

                if ((ucDirType == REC_DIR) || (ucDirType == EVT_DIR))
                {
                    retcode = FS_RemoveFile(filename, ucDirType);
                }
                else
                {
                    retcode = remove(filename);	 /* 删除文件 */
                }

                if (retcode == OK)
                {
                    memcpy(p, p_rcv_buffer+20, filenamelen+2);
                    p+=filenamelen+2;
                    *p++=0x00;
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd, p_send_buffer, p_rcv_buffer, 0x9140, send_lenth);
                }
                else
                {
                    memcpy(p, p_rcv_buffer+20, filenamelen+2);
                    p+=filenamelen+2;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x04;		/* 失败 */
                    *p++=0x00;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd, p_send_buffer, p_rcv_buffer, 0x9430, send_lenth);
                }
            }
            else
            {
                memcpy(p,p_rcv_buffer+20, filenamelen+2);
                p+=filenamelen+2;
                *p++=0x00;
                *p++=0x00;
                *p++=0x00;	/* 失败 */
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9430,send_lenth);
            }
            break;

        case 0x0f00:                    /*读取开出状态*/
            opt_success=1;
            if(opt_success)
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=iLgcDoChNum_g+2;
                untemp_val=(iLgcDoChNum_g+2+7)/8; /*增加一个启动测试和一个CPU告警*/
                for(temp_val=0; temp_val<untemp_val; temp_val++)
                {
                    uctemp_val=0x00;
                    sn=0x00;
                    ucval=1;
                    if(temp_val!=(untemp_val-1)) j=8;
                    else j=iLgcDoChNum_g+2-(untemp_val-1)*8;
                    for(i=0; i<j; i++)
                    {
                        if((i==j-2)&&(temp_val==untemp_val-1))
                        {
                            /* 倒数第二个为启动测试 */
                            if(SIO_Is_Open_QD())
                            {
                                uctemp_val|=ucval;
                                sn|=ucval;
                            }
                            ucval*=2;
                        }
                        else if((i==j-1)&&(temp_val==untemp_val-1))
                        {
                            /* 倒数第一个为CPU告警 */
                            if(bAlmDoIsForced)
                            {
                                uctemp_val|=ucval;
                                sn|=ucval;
                            }
                            ucval*=2;
                        }
                        else
                        {
                            retcode=RD_Mea_Hw_DO(temp_val*8+i);
                            if(retcode&0x7fff)
                            {
                                sn|=ucval;
                            }
                            if(retcode&0x8000)
                            {
                                uctemp_val|=ucval;
                            }
                            ucval*=2;
                        }
                    }
                    *p++=sn;
                    *p++=uctemp_val;
                }
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x9740,send_lenth);
            }
            else
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x7f;
                *p++=0x03;
                *p++=0x00;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        case 0x0700:                    /*读取报告列表*/
            strcpy(dirname,"");
            sprintf(dirname,EP_EVT_RPT_DIR);
            dirnamelen=strlen(dirname);
            pdir=opendir(dirname);
            if(pdir)
            {
                static BOOL bLog0700=TRUE;
                rpt_num=0;
                FileInfo=malloc(sizeof(File_Info)*MAX_ALLOW_RPT_NUM);
                if(FileInfo==NULL)
                {
                    close((int)pdir);
                    if (bLog0700)
                    {
                        bLog0700=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0700 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x报文时申请内存失败.\n", NULL);
                        }
                    }
                    return;
                }
                while ((pent=readdir(pdir))!=NULL)
                {
                    char *p;
                    p=pent->d_name+strlen(pent->d_name)-4;
                    if((pent->d_name[0]!='$')&&(pent->d_name[0]!='#')&&(strcmp(p,".bak")==0)&&(strcmp(p,".old")==0))
                    {
                        memcpy(filename,dirname,dirnamelen);
                        filename[dirnamelen]='/';
                        temp_val=strlen(pent->d_name);
                        memcpy(filename+dirnamelen+1,pent->d_name,temp_val);
                        filename[dirnamelen+1+temp_val]='\0';
                        filenamelen=strlen(filename);
                        if(FT_Is_File(filename))
                        {
                            if(rpt_num>=MAX_ALLOW_RPT_NUM)
                            {
                                close((int)pdir);
                                free(FileInfo);
                                LOG_Write(LOG_KERNEL, "The Dir include too many files.\n", NULL);
                                return;
                            }
                            memcpy(FileInfo[rpt_num].filename,filename,filenamelen);
                            FileInfo[rpt_num].filename[filenamelen]='\0';
                            if(stat(filename,&pStat)==OK)
                            {
                                FileInfo[rpt_num].timer=pStat.st_mtime;
                                FileInfo[rpt_num].FileSN=strtoul(pent->d_name+3, NULL, 16);
                            }
                            rpt_num++;
                        }
                    }
                }
                closedir(pdir);
                temp_val=sizeof(File_Info)*rpt_num;
                pTmpFileInfo=malloc(temp_val);
                if (pTmpFileInfo==NULL)
                {
                    if (bLog0700)
                    {
                        bLog0700=FALSE;
                        if(ENG_MODE == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Fail to malloc In explain() when deal 0x0700 msg.\n", NULL);
                        }
                        else if(ENG_MODE == 0)
                        {
                            LOG_Write(LOG_KERNEL, "解释0x0700报文时申请内存失败.\n", NULL);
                        }
                    }
                    free(FileInfo);
                    break;
                }
                memcpy(pTmpFileInfo,FileInfo,temp_val);
                free(FileInfo);
                qsort(pTmpFileInfo,rpt_num,sizeof(File_Info),compare_func);
                bsort=FALSE;
                if(pTmpFileInfo[rpt_num-1].FileSN-pTmpFileInfo[0].FileSN>10000)
                {
                    bsort=TRUE;
                    TmpInfo=pTmpFileInfo[0];
                    memmove(pTmpFileInfo,pTmpFileInfo+1,(rpt_num-1)*sizeof(File_Info));
                    pTmpFileInfo[rpt_num-1]=TmpInfo;
                    while(bsort)
                    {
                        if(((pTmpFileInfo[0].FileSN-pTmpFileInfo[rpt_num-1].FileSN)>0)&&
                                ((pTmpFileInfo[0].FileSN-pTmpFileInfo[rpt_num-1].FileSN)<=10000))
                        {
                            TmpInfo=pTmpFileInfo[0];
                            memmove(pTmpFileInfo,pTmpFileInfo+1,(rpt_num-1)*sizeof(File_Info));
                            pTmpFileInfo[rpt_num-1]=TmpInfo;
                        }
                        else
                        {
                            bsort=FALSE;
                            break;
                        }
                    }
                }
                start_sn=U8_TO_U16(p_rcv_buffer[23],p_rcv_buffer[22]);
                want_num=U8_TO_U16(p_rcv_buffer[25],p_rcv_buffer[24]);
                if(start_sn<=rpt_num)
                {
                    if((rpt_num-start_sn+1)>=want_num)
                    {
                        unval=want_num;
                    }
                    else                /*从起始序号起没有期望数量的报告可供读取*/
                    {
                        unval=rpt_num-start_sn+1;
                    }
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=LO8(start_sn);
                    *p++=HI8(start_sn);
                    *p++=LO8(unval);
                    *p++=HI8(unval);
                    for(untemp_val=0; untemp_val<unval; untemp_val++)
                    {
                        p1=p_tmp_buffer;
                        strcpy(filename,pTmpFileInfo[rpt_num-start_sn-untemp_val].filename);
                        fp=open(filename,0,0);
                        if(fp>0)
                        {
                            retcode=read(fp,p_tmp_buffer,22);
                            if(retcode==22)
                            {
                                retcode=EP_SUCCESS;
                                *p++=p_tmp_buffer[9];
                                *p++=p_tmp_buffer[10];
                                *p++=0x00;
                                *p++=0x00;
                                *p++=0x00;
                                *p++=0x00;
                                *p++=0x00;
                                p1+=11;
                                memcpy(p,p1,6);
                                p+=6;
                                p1+=6;
                                sec=U8_TO_U16(p1[1],p1[0])/1000;
                                ms=U8_TO_U16(p1[1],p1[0])%1000;
                                *p++=sec;
                                *p++=LO8(ms);
                                *p++=HI8(ms);
                                p1+=2;
                                *p++=*p1++;
                                *p++=*p1++;
                                rtcode=read(fp,p1+1,*p1+4+23);
                                if(rtcode==*p1+4+23)
                                {
                                    p1+=*p1+1+4+20;
                                    *p++=*p1++;
                                    *p++=*p1++;
                                    *p++=*p1++;
                                }
                                else
                                {
                                    retcode=EP_ERROR;
                                    break;
                                }
                            }
                            else
                            {
                                retcode=EP_ERROR;
                                break;
                            }
                            close(fp);
                        }
                        else
                        {
                            retcode=EP_ERROR;
                            break;
                        }
                    }
                    if(retcode==EP_ERROR)
                    {
                        p=p_send_buffer;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x4f;
                        *p++=0x01;
                        strcpy(error_msg,"when read rpt list file read error");
                        err_msg_len=strlen(error_msg);
                        *p++=err_msg_len;
                        memcpy(p,error_msg,err_msg_len);
                        p+=err_msg_len;
                        send_lenth=p-p_send_buffer;
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                    }
                    else
                    {
                        send_lenth=p-p_send_buffer;
                        reply(new_fd,p_send_buffer,p_rcv_buffer,0x8d00,send_lenth);
                    }
                }
                else                    /*目录下报告文件的总数小于要读取的起始序号*/
                {
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x4f;
                    *p++=0x01;
                    strcpy(error_msg,"Your start SN is too large so can't find valid file");
                    err_msg_len=strlen(error_msg);
                    *p++=err_msg_len;
                    memcpy(p,error_msg,err_msg_len);
                    p+=err_msg_len;
                    send_lenth=p-p_send_buffer;
                    reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
                }
                free(pTmpFileInfo);
            }
            else                        /*目录不存在*/
            {
                p=p_send_buffer;
                *p++=0x00;
                *p++=0x00;
                *p++=0x4f;
                *p++=0x01;
                strcpy(error_msg,"Can't find the EVT Dir");
                err_msg_len=strlen(error_msg);
                *p++=err_msg_len;
                memcpy(p,error_msg,err_msg_len);
                p+=err_msg_len;
                send_lenth=p-p_send_buffer;
                reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            }
            break;

        default:
            p=p_send_buffer;
            *p++=0;
            *p++=0;
            *p++=0x0f;
            *p++=0x00;
            *p++=0;
            send_lenth=p-p_send_buffer;
            reply(new_fd,p_send_buffer,p_rcv_buffer,0x8e00,send_lenth);
            break;
    }
}

/***********************************************************************
* MasterCPU_autosocket_create - 建立socket5、6连接,用于主动上传
*
* RETURNS: 无
*
*/
EP_STATUS MasterCPU_autosocket_create(
    int *sfd
)
{
    struct sockaddr_in server_addr;
    int retcode;
    struct timeval connect_timeout;
    int sockoptval;
    struct linger lingerval;

    if(*sfd>0)
    {
        close(*sfd);
        *sfd=-1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    if(sfd == &sock5_fd)
    {
        if(isNumber_2_04CPU() || (ucCPUSeq_g == 1) )		/* 为从CPU或者第2块CPU */
            server_addr.sin_port=htons(2120);
        else
            server_addr.sin_port=htons(2110);
        server_addr.sin_addr.s_addr=inet_addr(MMI_IP);

        LOG_Write(LOG_KERNEL, "自动上送至HMI任务创建.\n", NULL);
    }
    else if(sfd == &sock6_fd)
    {
        server_addr.sin_port=htons(2041);
        if(server4_OK)
        {
            /* server_addr.sin_addr.s_addr=inet_addr("192.168.0.48"); */
            server_addr.sin_addr.s_addr=PCVIEW_IP.sin_addr.s_addr;
        }
        else
        {
            return EP_ERROR;
        }
    }
#ifdef PANEL_HMI_SUPPORT
    else if(sfd == &ph_sock_auto_fd)
    {
        if(isNumber_2_04CPU() || (ucCPUSeq_g == 1) )		/* 为从CPU或者第2块CPU */
            server_addr.sin_port=htons(2320);
        else
            server_addr.sin_port=htons(2310);
        server_addr.sin_addr.s_addr=inet_addr(PH_IP);
    }
#endif
    *sfd=socket(PF_INET,SOCK_STREAM,0);
    connect_timeout.tv_sec=5; /* 2->5 */
    connect_timeout.tv_usec=100000;
    if(*sfd<0)
    {
        if(sfd == &sock5_fd)		/* 连接MMI */
        {
            LOG_Dbg_Msg("can't create autosocket5\n", 0, 0, 0, 0, 0, 0);
            Comm_Log_Write(6,"can't create autosocket5");
        }
        else if(sfd == &sock6_fd)		/* 连接sgView */
            LOG_Dbg_Msg("can't create autosocket6\n", 0, 0, 0, 0, 0, 0);
#ifdef PANEL_HMI_SUPPORT
        else if(sfd == &ph_sock_auto_fd)		/* 连接PH */
        {
            LOG_Dbg_Msg("can't create autosocketph\n", 0, 0, 0, 0, 0, 0);
        }
#endif
        return EP_ERROR;
        /* 错误处理(代码待填) */
    }
    sockoptval=1;
    setsockopt(*sfd, SOL_SOCKET, SO_KEEPALIVE,(char *)&sockoptval, sizeof (sockoptval));
    lingerval.l_onoff=1;
    lingerval.l_linger=0;
    setsockopt (*sfd, SOL_SOCKET, SO_LINGER,(char *)&lingerval, sizeof (lingerval));
    sockoptval=1;
    setsockopt (*sfd, IPPROTO_TCP, TCP_NODELAY, (char *)&sockoptval, sizeof (sockoptval));
    if((retcode=connect_with_timeout_posix(*sfd,(struct sockaddr *)&server_addr,sizeof(server_addr), connect_timeout.tv_sec))<0)
    {
        if(sfd == &sock5_fd)
        {
            LOG_Dbg_Msg("can't connect to MMI\n", 0, 0, 0, 0, 0, 0);
            Comm_Log_Write(7, "can't connect to MMI");
        }
        else if(sfd == &sock6_fd)
            LOG_Dbg_Msg("can't connect PC_VIEW\n", 0, 0, 0, 0, 0, 0);
#ifdef PANEL_HMI_SUPPORT
        else if(sfd == &ph_sock_auto_fd)
        {
            LOG_Dbg_Msg("can't connect to PH\n", 0, 0, 0, 0, 0, 0);
        }
#endif
        close(*sfd);
        *sfd=-1;
        return EP_ERROR;
        /* 错误处理(代码待填) */
    }

    if(sfd == &sock5_fd)
    {
        bConnectMmiSuccessFlag=TRUE;
        LOG_Dbg_Msg("connect to MMI\n", 0, 0, 0, 0, 0, 0);
    }
    else if(sfd == &sock6_fd)
        LOG_Dbg_Msg("connect to PC_VIEW\n", 0, 0, 0, 0, 0, 0);
#ifdef PANEL_HMI_SUPPORT
    else if(sfd == &ph_sock_auto_fd)
    {
        LOG_Dbg_Msg("connect to PH\n", 0, 0, 0, 0, 0, 0);
    }
#endif
    return EP_SUCCESS;
}

/***********************************************************************
* ReceiveWithTimeout - 接收函数
*
* RETURNS: 无
*
*/
EP_STATUS ReceiveWithTimeout(
    int sock_fd,
    int8_t *pcBuf,
    int32_t lLength,
    uint32_t ulTime
)
{
    EP_STATUS retcode;
    fd_set fd;
    struct timeval tv;

    FD_ZERO(&fd);
    FD_SET(sock_fd,&fd);

    tv.tv_usec=(ulTime%1000)*1000;
    tv.tv_sec=ulTime/1000;

    retcode=select(sock_fd+1, &fd, NULL, NULL, &tv);		/* 等待接收 */

    g_ComRcvSts = retcode;

    if(retcode == 0)
        return EP_TIMEOUT;
    else if(retcode == ERROR || retcode<0)
    {
        return EP_ERROR;
    }

    return recv(sock_fd, (char*)pcBuf, lLength, 0);

}

/***********************************************************************
* WriteWithTimeout - 发送函数
*
* RETURNS: 无
*
*/
EP_STATUS WriteWithTimeout(
    int sock_fd,
    uint8_t *pcBuf,
    int32_t lLength,
    uint32_t ulTime
)
{
    EP_STATUS retcode;
    fd_set fd;
    struct timeval tv;

    FD_ZERO(&fd);
    FD_SET(sock_fd,&fd);

    tv.tv_usec=(ulTime%1000)*1000;
    tv.tv_sec=ulTime/1000;

    retcode=select(sock_fd+1, NULL, &fd, NULL, &tv);

    if(retcode == 0)
        return EP_TIMEOUT;
    else if(retcode == ERROR || retcode<0)
        return EP_ERROR;

    return send(sock_fd,(char*)pcBuf, lLength, 0);
}

/***********************************************************************
* AutoSend - 自动上送
*
* RETURNS: 无
*
*/
EP_STATUS AutoSend(
    uint8_t *pcBuf,
    uint32_t ulLength,
    int *sfd
)
{
    int retcode;

    if(*sfd<0)
    {
        retcode=MasterCPU_autosocket_create(sfd);
        if(retcode == EP_ERROR)
        {
            return EP_ERROR;			/* 加上这句就是避免下边提出的问题 */
        }
    }
    retcode=write(*sfd, pcBuf, ulLength);	/* 这句可能有问题，忘记判断*sfd是不是大于零了 */

    if(retcode != ulLength)
    {
        close(*sfd);
        *sfd=-1;

        retcode=MasterCPU_autosocket_create(sfd);
        if(retcode == EP_ERROR)
            return EP_ERROR;
        else if (*sfd >= 0)
        {
            retcode=write(*sfd,pcBuf,ulLength);
            if(retcode != ulLength)
            {
                close(*sfd);
                *sfd=-1;
            }
        }
        else
        {
            return EP_ERROR;
        }
    }
    return retcode;
}

/***********************************************************************
* AutoAsk - 自动查询
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS AutoAsk(
    uint8_t *pcSendBuf,
    uint32_t ulSendLen,
    uint16_t unType,
    uint8_t *pcRecvBuf,
    uint32_t ulRecvLen,
    uint16_t *punRecvType,
    uint32_t ulTime,
    int *sfd
)
{
    uint8_t *p=NULL;
    uint8_t psp[Max_Frame_Lenth];
    uint16_t unAllFrames;
    uint16_t unFrame;
    uint16_t len;
    uint32_t reallen = 0;
    int retcode;
    uint32_t ulDataCnt;
    uint32_t ulAllDataCnts = 0;
    int32_t lLen;
    int rcv_number=0;

    /* 获得sock使用信号量,待定 */
    if(*sfd<0)
    {
        retcode=MasterCPU_autosocket_create(sfd);
        if(retcode == EP_ERROR)
        {
            retcode=EP_ERROR;
            goto exit_add;
        }
    }

    while(1)
    {
        rcv_number=ReceiveWithTimeout(*sfd,psp,Max_Frame_Lenth,0);
        if (rcv_number>0)
            continue;
        else if (rcv_number == ERROR)
        {
            close( *sfd);
            *sfd=-1;
            return(ERROR);
        }
        else
            break;
    }

    p=psp;
    *p++=Rpt_IntelType;
    *p++=Rpt_version;
    if(sfd==&sock5_fd)
    {
        *p++=MMI_addr;
    }
    else if(sfd==&sock6_fd)
    {
        *p++=PC_addr;
    }
#ifdef PANEL_HMI_SUPPORT
    else if(sfd==&ph_sock_auto_fd)
    {
        *p++=MMI_addr;
    }
#endif
    *p++=MasterCPU_addr;
    *p++=LO8(unType);
    *p++=HI8(unType);
    *p++=LL8(ulSendLen);
    *p++=LH8(ulSendLen);
    *p++=HL8(ulSendLen);
    *p++=HH8(ulSendLen);
    *p++=0;
    *p++=0;
    if(ulSendLen>0)
        unAllFrames=(ulSendLen+Max_Frame_Data_Lenth-1)/Max_Frame_Data_Lenth;
    else
        unAllFrames=1;

    *p++=LO8(unAllFrames);
    *p++=HI8(unAllFrames);
    for(unFrame=0; unFrame<unAllFrames; unFrame++)
    {
        p=psp+14;
        *p++=LO8(unFrame);
        *p++=HI8(unFrame);
        if(sfd==&sock5_fd)
        {
            *p++=LO8(SOCK5_RI_CNT);
            *p++=HI8(SOCK5_RI_CNT);
        }
        else if(sfd==&sock6_fd)
        {
            *p++=LO8(SOCK6_RI_CNT);
            *p++=HI8(SOCK6_RI_CNT);
        }
#ifdef PANEL_HMI_SUPPORT
        else if(sfd==&ph_sock_auto_fd)
        {
            *p++=LO8(PH_SOCK_AUTO_RI_CNT);
            *p++=HI8(PH_SOCK_AUTO_RI_CNT);
        }
#endif
        if(ulSendLen>=Max_Frame_Data_Lenth)
            len=Max_Frame_Data_Lenth;
        else
        {
            len=ulSendLen;
        }

        *p++=LO8(len);
        *p++=HI8(len);
        memcpy(p,pcSendBuf,len);
        p+=len;
        ulSendLen-=len;
        pcSendBuf+=len;

        *p=Cal_CheckSum(psp,len+Frame_Head_Lenth);	/*求和校验*/

        reallen=len+Frame_Head_Lenth+1;

        /*发送报文*/
        retcode=AutoSend(psp,reallen,sfd);
        if(retcode<0)
        {
            retcode=EP_ERROR;
            goto exit_add;
        }
    }

    /*接收应答，待定*/
    if(pcRecvBuf==NULL)					/*NULL表示无需应答*/
    {
        retcode=EP_SUCCESS;
        goto exit_add;
    }

    ulDataCnt=0;
    unAllFrames=1;
    for(unFrame=0; unFrame<unAllFrames; unFrame++)
    {
        /*接收应答*/
        retcode=ReceiveWithTimeout(*sfd,psp,Max_Frame_Lenth,ulTime);
        if(retcode<Frame_Head_Lenth+1)
        {
            /* LOG_Write(LOG_KERNEL, "无应答, 本次主动上送失败.\n", NULL); */

            /* 接收不到直接关闭, 再重连 */
            close( *sfd);
            *sfd=-1;
            retcode=EP_ERROR;
            goto exit_add;
        }
        lLen=retcode-Frame_Head_Lenth-1;

        /*检查校验和//待定*/

        /*检查规约标识、目的地址、源地址、帧序号、数据长度*/
#ifdef PANEL_HMI_SUPPORT
        if(psp[0]!=Rpt_IntelType ||
                psp[2]!=MasterCPU_addr ||
                (psp[3]!=MMI_addr && psp[3]!=PC_addr) ||
                U8_TO_U16(psp[15],psp[14])!=unFrame ||
                ((sfd==&sock5_fd)&&(U8_TO_U16(psp[17],psp[16])!=SOCK5_RI_CNT))||
                ((sfd==&sock6_fd)&&(U8_TO_U16(psp[17],psp[16])!=SOCK6_RI_CNT))||
                ((sfd==&ph_sock_auto_fd)&&(U8_TO_U16(psp[17],psp[16])!=PH_SOCK_AUTO_RI_CNT))||
                U8_TO_U16(psp[19],psp[18])!=retcode-(Frame_Head_Lenth+1))
        {

            /* LOG_Write(LOG_KERNEL, "数据校验错误(1).\n", NULL); */

            /* 数据校验错误 */
            close( *sfd);
            *sfd=-1;

            retcode=EP_ERROR;
            goto exit_add;
        }
#else
        if(psp[0]!=Rpt_IntelType ||
                psp[2]!=MasterCPU_addr ||
                (psp[3]!=MMI_addr && psp[3]!=PC_addr) ||
                U8_TO_U16(psp[15],psp[14])!=unFrame ||
                ((sfd==&sock5_fd)&&(U8_TO_U16(psp[17],psp[16])!=SOCK5_RI_CNT))||
                ((sfd==&sock6_fd)&&(U8_TO_U16(psp[17],psp[16])!=SOCK6_RI_CNT))||
                U8_TO_U16(psp[19],psp[18])!=retcode-(Frame_Head_Lenth+1))
        {

            /* LOG_Write(LOG_KERNEL, "数据校验错误(2).\n", NULL); */

            /* 数据校验错误 */
            close( *sfd);
            *sfd=-1;

            retcode=EP_ERROR;
            goto exit_add;
        }
#endif

        if(unFrame==0)
        {
            /*记录数据总长度*/
            ulAllDataCnts=U8_TO_U32(psp[9],psp[8],psp[7],psp[6]);
            /*记录总帧数*/
            unAllFrames=U8_TO_U16(psp[13],psp[12]);
            /*存储帧类型*/
            *punRecvType=U8_TO_U16(psp[5],psp[4]);
        }
        else
        {
            if(ulAllDataCnts!=U8_TO_U32(psp[9],psp[8],psp[7],psp[6])||
                    unAllFrames!=U8_TO_U16(psp[13],psp[12])||
                    *punRecvType!=U8_TO_U16(psp[5],psp[4]) )
            {

                /* LOG_Write(LOG_KERNEL, "数据校验错误(3).\n", NULL); */

                /* 数据校验错误 */
                close( *sfd);
                *sfd=-1;

                retcode=EP_ERROR;
                goto exit_add;
            }

        }
        /*存储数据*/
        lLen=U8_TO_U16(psp[19],psp[18]);
        if(lLen>ulRecvLen)
        {

            /* LOG_Write(LOG_KERNEL, "数据校验错误(4).\n", NULL); */

            /* 数据校验错误 */
            close( *sfd);
            *sfd=-1;

            retcode=EP_ERROR;
            goto exit_add;
        }
        memcpy(pcRecvBuf+ulDataCnt,psp+Frame_Head_Lenth,lLen);

        ulRecvLen-=lLen;
        ulDataCnt+=lLen;
    }

    if(ulDataCnt!=ulAllDataCnts)
    {

        /* LOG_Write(LOG_KERNEL, "数据校验错误(5).\n", NULL); */

        /* 数据校验错误 */
        close( *sfd);
        *sfd=-1;

        retcode=EP_ERROR;
        goto exit_add;
    }
    retcode=ulDataCnt;

exit_add:

    if(sfd==&sock5_fd)
        SOCK5_RI_CNT++;
    else if(sfd==&sock6_fd)
        SOCK6_RI_CNT++;
#ifdef PANEL_HMI_SUPPORT
    else if(sfd==&ph_sock_auto_fd)
        PH_SOCK_AUTO_RI_CNT++;
#endif
    /*临界资源管理*/
    /*释放sock使用信号量，待定*/
    return retcode;

}

/***********************************************************************
* listen_socket1 - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socket1(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    sock1_fd=MasterCPU_serversock_create(local_IP,2010);
    if(sock1_fd>0)
        MasterCPU_socket_listen(&sock1_fd,&new_fd1);

    return 0;
}

/***********************************************************************
* listen_socket2 - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socket2(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    sock2_fd=MasterCPU_serversock_create(local_IP,2020);
    if(sock2_fd>0)
        MasterCPU_socket_listen(&sock2_fd,&new_fd2);

    return 0;
}

/***********************************************************************
* listen_socketrec - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socketrec(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    sockrec_fd=MasterCPU_serversock_create(local_IP,2050);
    if(sockrec_fd>0)
        MasterCPU_socket_listen(&sockrec_fd,&new_fdrec);

    return 0;
}

/***********************************************************************
* listen_socket3 - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socket3(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    sock3_fd=MasterCPU_serversock_create(local_IP,2030);
    if(sock3_fd>0)
        MasterCPU_socket_listen(&sock3_fd,&new_fd3);

    return 0;
}

/***********************************************************************
* listen_socket4 - 建立帧听socket
*
* RETURNS: 无
*
*/
int listen_socket4(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    EDP_NET_CFG_INFO NetCfgInfo;
    if(NT_GetNetRunCfg(&NetCfgInfo) == EP_SUCCESS)
    {
        /* 支持使用网口2的IP地址连接sgView */
        char SetIpAddr[32];
#ifdef EDP02_PSR_BUILD         /*psr660u是网口1连接SGVIEW*/
        sprintf(SetIpAddr, "%d.%d.%d.%d",
                NetCfgInfo.NetInfArr[0].aucIpAddr[0],
                NetCfgInfo.NetInfArr[0].aucIpAddr[1],
                NetCfgInfo.NetInfArr[0].aucIpAddr[2],
                NetCfgInfo.NetInfArr[0].aucIpAddr[3]);

#else
        sprintf(SetIpAddr, "%d.%d.%d.%d",
                NetCfgInfo.NetInfArr[1].aucIpAddr[0],
                NetCfgInfo.NetInfArr[1].aucIpAddr[1],
                NetCfgInfo.NetInfArr[1].aucIpAddr[2],
                NetCfgInfo.NetInfArr[1].aucIpAddr[3]);
#endif
        sock4_fd=MasterCPU_serversock_create(SetIpAddr,2040);
    }
    else
    {
        sock4_fd=MasterCPU_serversock_create(PSVIEW_IP,2040);
    }
    if(sock4_fd>0)
        MasterCPU_socket_listen(&sock4_fd,&new_fd4);

    return 0;
}

#ifdef FASTER_OPR_SUPPORT
/***********************************************************************
* listen_socket - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socket(
    int psockfd,
    int pnewfd,
    int port,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int hpnewfd,
    int hpsockfd
)
{

    uint64_t psockfd_val;
    psockfd_val=hpsockfd;
    psockfd_val=psockfd_val<<32;
    psockfd_val=psockfd_val|psockfd;

    uint64_t pnewfd_val;
    pnewfd_val=hpnewfd;
    pnewfd_val=pnewfd_val<<32;
    pnewfd_val=pnewfd_val|pnewfd;

    *((int *)psockfd_val)=MasterCPU_serversock_create(local_IP,port);
    if(&psockfd_val>0)
        MasterCPU_socket_listen((int *)psockfd_val,(int *)pnewfd_val);

    return 0;
}

#endif

#ifdef PANEL_HMI_SUPPORT
/***********************************************************************
* listen_socket_ph - 建立帧听socket
*
* RETURNS: fd
*
*/
int listen_socket_ph(
    int psockfd,
    int pnewfd,
    int port,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{


    *((int *)psockfd)=MasterCPU_serversock_create(local_IP,port);
    if(&psockfd>0)
        MasterCPU_socket_listen((int *)psockfd,(int *)pnewfd);

    return 0;
}

#endif

/***********************************************************************
* init - 初始化通讯口相关变量
*
* RETURNS: 无
*
*/
void init(void)
{
    uint8_t i;
    uint8_t addr[4];

    sock1_fd=-1;
    sock2_fd=-1;
    sockrec_fd=-1;
    sock3_fd=-1;
    sock4_fd=-1;
    sock5_fd=-1;
    sock6_fd=-1;
    new_fd1=-1;
    new_fd2=-1;
    new_fdrec=-1;
    new_fd3=-1;
    new_fd4=-1;
    new_fd5=-1;
    pipe_fd=-1;
    for(i=0; i<5; i++)
    {
        pUsingBuf[i]=NULL;
        UsingFD[i]=-1;
    }
    /* pEventBuf=malloc(EventBuf_Size); */
    m_ulRecvAutoPtr=0;
    autosem=semCCreate(SEM_Q_PRIORITY,0);
    server_sem1=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    server_sem2=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    server_semrec=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    server_sem3=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    server_sem4=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);

#ifdef FASTER_OPR_SUPPORT
    sock_faster_fd = -1;
    new_faster_fd = -1;
    server_sem_faster = semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
#endif

#ifdef PANEL_HMI_SUPPORT
    ph_sock_fast_fd=-1;
    ph_sock_slow_fd=-1;
    ph_sockrec_fd=-1;
    ph_sock_auto_fd=-1;

    ph_new_fast_fd=-1;
    ph_new_slow_fd=-1;
    ph_new_sockrec_fd=-1;

    ph_server_sem_fast=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    ph_server_sem_slow=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
    ph_server_sem_rec=semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);

#endif

    strcpy(local_IP,"");
    if(isNumber_2_04CPU() || (ucCPUSeq_g == 1))		/* 为从CPU或第2块CPU */
    {
        strcpy(local_IP,"10.10.10.3");
        addr[0]=10;
        addr[1]=10;
        addr[2]=10;
        addr[3]=3;
        MasterCPU_addr=0x81;
    }
    else
    {
        strcpy(local_IP,"10.10.10.4");
        addr[0]=10;
        addr[1]=10;
        addr[2]=10;
        addr[3]=4;
        MasterCPU_addr=0x80;
    }
#ifndef EDP02_PSR_BUILD
    // if(Set_HdlcIP(INNER_HDLC_IP_PORT, addr) != OK)
    {
        if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Net configuration failue: set HDLC IP address failure.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "网络配置失败: 设置HDLC IP地址失败.\n", NULL);
        }
    }
#endif
    Init_Net();
    EP_AdjNetPri();
    // beginHdlc();	/* 初始化HDLC */
}

/***********************************************************************
* com_init - 初始化通讯口
*
* RETURNS: 无
*
*/
void com_init(void)
{
    int tID;
#ifndef  EDP02_PSR_BUILD

    /* init(); */
    tID = taskSpawn("listen1", TSK_PRI_FAST_SEVER_SVR+1, VX_FP_TASK, 50000,listen_socket1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    tID = taskSpawn("listen2", TSK_PRI_LISTEN_SVR, VX_FP_TASK, 50000,listen_socket2,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    tID = taskSpawn("listenRec", TSK_PRI_REC_SVR, VX_FP_TASK, 50000,listen_socketrec,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    tID = taskSpawn("listen3", TSK_PRI_LISTEN_SVR, VX_FP_TASK, 50000,listen_socket3,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    tID = taskSpawn("listen4", TSK_PRI_LISTEN_SVR, VX_FP_TASK, 50000,listen_socket4,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    tID = taskSpawn("AutoTask", TSK_PRI_EVT_SND, VX_FP_TASK, 50000,WaitAutoTask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    tID = MasterCPU_autosocket_create(&sock5_fd);
    tID = MasterCPU_autosocket_create(&sock6_fd);

#ifdef FASTER_OPR_SUPPORT
    tID = taskSpawn("listen_faster", TSK_PRI_EVT_SND-1, VX_FP_TASK, 50000,listen_socket,
                    (int)(&sock_faster_fd), (int)(&new_faster_fd), 2310, 0, 0, 0, 0, 0, (int)(((uint64_t)(&new_faster_fd))>>32), (int)(((uint64_t)(&sock_faster_fd))>>32));
#endif

#ifdef PANEL_HMI_SUPPORT
    tID = taskSpawn("listen_ph_fast", TSK_PRI_FAST_SEVER_SVR+1, VX_FP_TASK, 50000,listen_socket_ph,
                    (int)(&ph_sock_fast_fd), (int)(&ph_new_fast_fd), 2210, 0, 0, 0, 0, 0, 0, 0);
    tID = taskSpawn("listen_ph_slow", TSK_PRI_LISTEN_SVR, VX_FP_TASK, 50000,listen_socket_ph,
                    (int)(&ph_sock_slow_fd), (int)(&ph_new_slow_fd), 2220, 0, 0, 0, 0, 0, 0, 0);
    tID = taskSpawn("listen_ph_rec", TSK_PRI_REC_SVR, VX_FP_TASK, 50000,listen_socket_ph,
                    (int)(&ph_sockrec_fd), (int)(&ph_new_sockrec_fd), 2250, 0, 0, 0, 0, 0, 0, 0);

    tID = MasterCPU_autosocket_create(&ph_sock_auto_fd);

    /*在路由表中添加面板HMI地址*/
    routeAdd(PH_IP,MMI_IP);
#endif

}

/***********************************************************************
* CalTimeDelta - 计算时间差
*
* RETURNS: 时间差
*
*/
uint32_t CalTimeDelta(
    uint32_t time1,
    uint32_t time2
)
{
    uint32_t delta;

    if(time2 >= time1)
    {
        delta=time2-time1;
    }
    else
    {
        delta=0xffffffff-time1+1+time2;
    }

    return delta;
}

/***********************************************************************
* AutoMmiMsgQReceiveTask - 自动事件上送到MMI的接收缓冲中去
*
* RETURNS: 返回值
*
*/
int AutoMmiMsgQReceiveTask()
{
    int retcode;
    uint16_t unRType;
    char p_send_buffer[512+2+2+1]= {0};		/* 实际数据为512个字节+2个字节数据大小+2个字节事件类型 */
    char p_rcv_buffer[Max_Frame_Lenth]= {0};

    while(1)
    {
        if( msgQReceive(SendEventToMmiMsgQFd, (char *)p_send_buffer, 512+2+2, WAIT_FOREVER)>0)
        {
            if(server3_OK)
            {
                retcode=AutoAsk(p_send_buffer,U8_TO_U16(p_send_buffer[513],p_send_buffer[512]),
                                U8_TO_U16(p_send_buffer[515],p_send_buffer[514]),p_rcv_buffer,
                                Max_Frame_Data_Lenth,&unRType,ACK_TIMEOUT,&sock5_fd);

                if(retcode<0)
                    taskDelay(10);
            }
#ifdef PANEL_HMI_SUPPORT
            if(ph_server_OK)
            {
                retcode=AutoAsk(p_send_buffer,U8_TO_U16(p_send_buffer[513],p_send_buffer[512]),
                                U8_TO_U16(p_send_buffer[515],p_send_buffer[514]),p_rcv_buffer,
                                Max_Frame_Data_Lenth,&unRType,ACK_TIMEOUT,&ph_sock_auto_fd);

                if(retcode<0)
                    taskDelay(10);
            }
#endif

        }
    }

    return 1;
}

/***********************************************************************
* AutoSgviewMsgQReceiveTask - 自动事件上送到SGVIEW的接收缓冲中去
*
* RETURNS: 返回值
*
*/
int AutoSgviewMsgQReceiveTask()
{
    int retcode;
    uint16_t unRType;
    char p_send_buffer[512+2+2+1]= {0};	/* 实际数据为512个字节+2个字节数据大小+2个字节事件类型 */
    char p_rcv_buffer[Max_Frame_Lenth]= {0};

    while(1)
    {
        if( msgQReceive(SendEventToSgviewMsgQFd, (char *)p_send_buffer, 512+2+2, WAIT_FOREVER)>0)
        {
            if(server4_OK)
                retcode=AutoAsk(p_send_buffer,U8_TO_U16(p_send_buffer[513],p_send_buffer[512]),
                                U8_TO_U16(p_send_buffer[515],p_send_buffer[514]),p_rcv_buffer,
                                Max_Frame_Data_Lenth,&unRType,ACK_TIMEOUT,&sock6_fd);
            else
                taskDelay(10);
        }
    }

    return 1;
}

/***********************************************************************
* InitEventMsgQTask - 自动事件上送到SGVIEW的接收缓冲中去
*
* RETURNS: ID
*
*/
int InitEventMsgQTask()
{
    int tID;

    if (uiAppType_g == APP_BUS)
    {
        SendEventToMmiMsgQFd = msgQCreate(1024, 512+2+2, MSG_Q_FIFO);
    }
    else
    {
        SendEventToMmiMsgQFd = msgQCreate(256, 512+2+2, MSG_Q_FIFO);
    }

    SendEventToSgviewMsgQFd = msgQCreate(256, 512+2+2, MSG_Q_FIFO);		/* 16 to 256. */

    tID = taskSpawn("tAutoMmiMsgQReceiveTask", TSK_PRI_EVT_SND-1, 0, 50000, AutoMmiMsgQReceiveTask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


    tID = taskSpawn("tAutoSgviewMsgQReceiveTask", TSK_PRI_EVT_SND-1, 0, 50000, AutoSgviewMsgQReceiveTask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


#ifdef GXC01U
    tID = taskSpawn("tWaitAutoSendToGxc01UTask", TSK_PRI_EVT_SND-1, 0, 50000, WaitAutoSendToGxc01UTask,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    return 0;
}

#ifdef EDP02_PSR_BUILD
SEM_ID semNewInfCom_gcan ;
#define CAN_OK 1
#endif
/***********************************************************************
* WaitAutoTask - 获取信号量后进行主动上传报文处理
*
* RETURNS: EP_SUCCESS，正常返回
*          EP_ERROR，  错误返回
*
*/
int WaitAutoTask(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    uint8_t *p;
    uint8_t p_send_buffer[Max_Frame_Lenth];
    uint8_t i;
    u_int autoIdx=0;
    const VI_RUN_INFO *prunInfo;
    EP_DATE_TIME dttm;
    uint8_t pucGet[4];
    uint16_t err_msg_len;
    uint8_t uctemp_val;
    int retcode;
    int sem_ok;
    RD_LGC_LED_CH *pch;
    uint8_t ucSn = 0;  /* 序号 */
    unsigned char bOpeRslt;
    unsigned char bIsRepairSts;

#ifdef EDP02_PSR_BUILD
    semNewInfCom_gcan = semCCreate(SEM_Q_PRIORITY,0);
#endif
    InitEventMsgQTask();
    while(1)
    {
#ifdef PANEL_HMI_SUPPORT
        if(server3_OK||server4_OK||ph_server_OK)
#else
#ifdef EDP02_PSR_BUILD
        if(server3_OK||server4_OK||CAN_OK)
#else
        if(server3_OK||server4_OK)
#endif
#endif
            break;
        else
        {
            taskDelay(sysClkRateGet( )/10);
        }
    }
    while(1)
    {
        if(AdjustTimeSuccessFlag==TRUE)
            break ;
        else
            taskDelay(sysClkRateGet( )/10);
    }

    while(!GetRptSNProcessState())
    {
        /* 等待事件模块初始化之前产生的事件处理完 */
        taskDelay(10);
    }

    while(1)
    {
        int mea_frame_num=0;
        int mea_num_per_frame=0;
        uint16_t mea_overflow_num=0;
        int ii;
        ME_MEA_AI_DATA_DB **my_ppmeadb;

        sem_ok=semTake(semNewInfCom_g,WAIT_FOREVER);
        if(sem_ok==ERROR)
            LOG_Dbg_Msg("Take a Event sem and semTake ERROR\n",0,0,0,0,0,0);

        while(ulInfNonComleteCnt_g!=0)
        {
            /*若还有新消息未完全写入消息队列，则等待100ms 2010-4-26 ZY  */
            taskDelay(10);
        }

#ifdef EDP02_PSR_BUILD
        semGive(semNewInfCom_gcan);
        if(!(server3_OK||server4_OK))
            continue;
#endif
        prunInfo=VI_Rd_Run_Info(autoIdx);
        autoIdx++;

        if(prunInfo!=NULL)
        {
            switch(prunInfo->type)
            {
                case MEA_OVER:
                    mea_overflow_num=prunInfo->msg.mea.uiNum;
                    mea_frame_num=mea_overflow_num/(MAX_MEA_NUM)+1;
                    my_ppmeadb=prunInfo->msg.mea.ppcfg;

                    for(ii=0; ii<mea_frame_num; ii++)/*超过MAX_MEA_NUM个变化量,则进行分帧发送*/
                    {
                        mea_num_per_frame = (ii==(mea_frame_num-1))?  (mea_overflow_num-(mea_frame_num-1)*(MAX_MEA_NUM)) : MAX_MEA_NUM;
                        p=p_send_buffer;
                        *p++=0x01;      //0为发送全部测量量，1为只发送发生越限的测量量

                        /* 遥测时标填写 */
                        retcode = TM_To_Dttm(prunInfo->msg.mea.ulTime, &dttm);

                        if (retcode == EP_SUCCESS)
                        {
                            *p++ = dttm.ucHour;
                        }
                        else
                        {
                            *p = dttm.ucHour;
                            *p |= 0x80;
                            p++;
                        }
                        *p++ = dttm.ucMinute;
                        *p++ = dttm.ucSec;
                        *p++ = LO8(dttm.unMSEL);
                        *p++ = HI8(dttm.unMSEL);
                        *p++ = LO8(dttm.unMicroSec);
                        *p++ = HI8(dttm.unMicroSec);

                        /* 时标品质 */
                        *p++ = GetSysTimeQFlag();

                        /* 保护测控一体化装置 */
                        if (uiAppType_g == APP_PROT_MEA_MERGE)
                        {
                            *p++ = 0x01; /* 是否包含品质 */
                            for (i = 0; i<7; i++)
                                *p++ = 0x00;
                        }
                        else
                        {
                            for(i=0; i<8; i++)
                                *p++=0x00;
                        }
                        *p++=prunInfo->msg.mea.ucCOT;
                        *p++=LO8(mea_num_per_frame);
                        *p++=HI8(mea_num_per_frame);

                        for(i=0; i<mea_num_per_frame; i++)
                        {
                            FLT_U32_UNION ulAI_Val;
                            *p++=LO8((my_ppmeadb[i+ii*MAX_MEA_NUM]->uiCode));
                            *p++=HI8((my_ppmeadb[i+ii*MAX_MEA_NUM]->uiCode));

                            /* 保护测控一体化装置 */
                            if (uiAppType_g == APP_PROT_MEA_MERGE)
                            {
                                *p++ = LO8(my_ppmeadb[i+ii*MAX_MEA_NUM]->usQuality);
                                *p++ = HI8(my_ppmeadb[i+ii*MAX_MEA_NUM]->usQuality);
                            }
                            else
                            {
                                *p++ = 0x00;
                                *p++ = 0x00;
                            }

                            *p++=my_ppmeadb[i+ii*MAX_MEA_NUM]->ucUnit;
                            *p++=my_ppmeadb[i+ii*MAX_MEA_NUM]->ucAttr;
                            ulAI_Val.fVal=my_ppmeadb[i+ii*MAX_MEA_NUM]->fVal;/*实际值*/
                            *p++=LL8(ulAI_Val.ulVal);
                            *p++=LH8(ulAI_Val.ulVal);
                            *p++=HL8(ulAI_Val.ulVal);
                            *p++=HH8(ulAI_Val.ulVal);
                        }
                        p_send_buffer[512]=LO8(20+mea_num_per_frame*10);/*有效数据区大小*/
                        p_send_buffer[513]=HI8(20+mea_num_per_frame*10);
                        p_send_buffer[514]=LO8(0xb000);
                        p_send_buffer[515]=HI8(0xb000);
                        retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer, 512+2+2, NO_WAIT,MSG_PRI_NORMAL);
                        taskDelay(1);
                        retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer, 512+2+2, NO_WAIT,MSG_PRI_NORMAL);
                    }
                    break;

                case NEW_EVT:
                    if((prunInfo->msg.evt.pcfg->ucType!=32)&&(prunInfo->msg.evt.pcfg->ucType!=33))
                    {
                        p=p_send_buffer;
                        *p = 0x00;
                        bOpeRslt = EP_Get_Repair_Sts(&bIsRepairSts);
                        if (bOpeRslt && bIsRepairSts)
                        {
                            *p |= 0x08;
                        }
                        p++;

                        *p++=prunInfo->msg.evt.bState?0:1;
                        *p++=prunInfo->msg.evt.ucCOT;
                        *p++=prunInfo->msg.evt.pcfg->ucType;
                        *p++=LO8(prunInfo->msg.evt.unRptSN);
                        *p++=HI8(prunInfo->msg.evt.unRptSN);
                        *p++=prunInfo->msg.evt.ucRecSN;


                        retcode=TM_To_Dttm(prunInfo->msg.evt.ulTime,&dttm);


                        if(retcode==EP_SUCCESS)
                        {
                            *p++=dttm.ucHour;
                        }
                        else
                        {
                            *p=dttm.ucHour;
                            *p|=0x80;
                            p++;
                        }
                        p_send_buffer[0] |= dttm.ucIrigbLSFlag;  /*闰秒时间标志*/
                        SYN_LOG("kevin 0x8400  1 : 时-分-秒-毫秒-微秒: %d-%d-%d-%d-%d,  闰秒FLAG: 0x%x\n",
                                dttm.ucHour, dttm.ucMinute, dttm.ucSec, dttm.unMSEL, dttm.unMicroSec, p_send_buffer[0]);
                        *p++=dttm.ucMinute;
                        *p++=dttm.ucSec;
                        *p++=LO8(dttm.unMSEL);
                        *p++=HI8(dttm.unMSEL);
                        *p++=LO8(dttm.unMicroSec);
                        *p++=HI8(dttm.unMicroSec);
                        *p++=LO8(prunInfo->msg.evt.pcfg->unCode);
                        *p++=HI8(prunInfo->msg.evt.pcfg->unCode);
                        *p++=prunInfo->msg.evt.pcfg->ucParmNum;
                        /*add 20050607*/
                        if(prunInfo->msg.evt.pcfg->ucParmNum>MAX_EVT_PARM_NUM)
                            break;
                        for(i=0; i<prunInfo->msg.evt.pcfg->ucParmNum; i++)
                        {
                            *p++=prunInfo->msg.evt.pcfg->aparmcfg[i].ucAttrib;
                            FLT_TO_BYTES(pucGet,REAL(prunInfo->msg.evt.aparm[i].xVal));
                            memcpy(p,pucGet,4);
                            p+=4;
                            FLT_TO_BYTES(pucGet,IMAGE(prunInfo->msg.evt.aparm[i].xVal));
                            memcpy(p,pucGet,4);
                            p+=4;
                        }

                        p_send_buffer[512]=LO8(17+p_send_buffer[16]*9);/*有效数据区大小*/
                        p_send_buffer[513]=HI8(17+p_send_buffer[16]*9);
                        p_send_buffer[514]=LO8(0x8400);
                        p_send_buffer[515]=HI8(0x8400);
                        retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                         512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                        taskDelay(1);
                        retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer,
                                         512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    }
                    break;

                case NEW_SOE:

                    p=p_send_buffer;
                    *p = 0x00;

                    bOpeRslt = EP_Get_Repair_Sts(&bIsRepairSts);
                    if (bOpeRslt && bIsRepairSts)
                    {
                        *p |= 0x08;
                    }
                    p++;

                    *p++=HI8(prunInfo->msg.soe.unCh);
                    *p++=prunInfo->msg.soe.ucCOT;
                    *p++=prunInfo->msg.soe.ucFtype;
                    *p++=LO8(prunInfo->msg.soe.unRptSN);
                    *p++=HI8(prunInfo->msg.soe.unRptSN);
                    *p++=prunInfo->msg.soe.ucRecSN;


                    if(prunInfo->msg.soe.ullusCntFrom1970 != 0)
                    {
                        US_CNT_UTC_TIME usUTCtmTmp;
                        usUTCtmTmp.ullusCntFrom1970 = prunInfo->msg.soe.ullusCntFrom1970;
                        Us_UTC_Time_To_Dttm(&usUTCtmTmp, &dttm);
                        retcode=EP_SUCCESS;
                    }
                    else
                    {
                        retcode=TM_To_Dttm(prunInfo->msg.soe.ulTime,&dttm);
                    }


                    if(retcode==EP_SUCCESS)
                    {
                        *p++=dttm.ucHour;
                    }
                    else
                    {
                        *p=dttm.ucHour;
                        *p|=0x80;
                        p++;
                    }
                    p_send_buffer[0] |= dttm.ucIrigbLSFlag;  /*闰秒时间标志*/
                    SYN_LOG("kevin 0x8410  1 : 时-分-秒-毫秒-微秒: %d-%d-%d-%d-%d,  闰秒FLAG: 0x%x\n",
                            dttm.ucHour, dttm.ucMinute, dttm.ucSec, dttm.unMSEL, dttm.unMicroSec, p_send_buffer[0]);
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);
                    *p++=LO8(prunInfo->msg.soe.unCh);
                    *p++=prunInfo->msg.soe.ucDIQ;
                    
                    /* 年/月/日信息 */
                    *p++=0;
                    *p++=0;
                    *p++=(uint8_t)(dttm.unYear&0xff);
                    *p++=(uint8_t)(dttm.unYear>>8);
                    *p++=dttm.ucMonth;
                    *p++=dttm.ucDate;
                    
                    p_send_buffer[512]=LO8(p-p_send_buffer);/*有效数据区大小*/
                    p_send_buffer[513]=HI8(p-p_send_buffer);
                    p_send_buffer[514]=LO8(0x8410);
                    p_send_buffer[515]=HI8(0x8410);
                    retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    taskDelay(1);
                    retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    break;

                case LED_CHG:
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x01;
                    *p++ = iHwLedChNum_g;

                    for (pch = plgcledch_g; pch<plgcledch_g+iHwLedChNum_g; pch++)
                    {
                        /* 不能超过256个指示灯(1个字节)
                         * 而配置工具允许65536个(2个字节)
                         * 一致性问题
                         */
                        ucSn = pch - plgcledch_g;
                        *p++ = ucSn;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x00;
                        *p++=0x00;
                        if (ucSn == prunInfo->msg.led.unIdx)
                        {
                            /* 本次变位指示灯使用变位后状态 */
                            *p++=prunInfo->msg.led.bSts;
                        }
                        else
                        {
                            /* 其它使用当前状态 */
                            *p++ = pch->bSts;
                        }
                    }

                    *p++=0;				/*液晶屏指示灯不要了*/
                    p_send_buffer[512]=LO8(p-p_send_buffer);/*有效数据区大小*/
                    p_send_buffer[513]=HI8(p-p_send_buffer);
                    p_send_buffer[514]=LO8(0x8780);
                    p_send_buffer[515]=HI8(0x8780);
                    retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    taskDelay(1);
                    break;

                case LINK_CHG:
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=prunInfo->msg.link.ucCOT;
                    retcode=TM_To_Dttm(prunInfo->msg.link.ulTime,&dttm);
                    if(retcode==EP_SUCCESS)
                    {
                        *p++=dttm.ucHour;
                    }
                    else
                    {
                        *p=dttm.ucHour;
                        *p|=0x80;
                        p++;
                    }
                    p_send_buffer[0] |= dttm.ucIrigbLSFlag;  /*闰秒时间标志*/
                    SYN_LOG("kevin 0x9700  1 : 时-分-秒-毫秒-微秒: %d-%d-%d-%d-%d,  闰秒FLAG: 0x%x\n",
                            dttm.ucHour, dttm.ucMinute, dttm.ucSec, dttm.unMSEL, dttm.unMicroSec, p_send_buffer[0]);
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);
                    *p++=0x01;              /*一次只送一个压板状态变化*/
                    *p++=prunInfo->msg.link.unCh;
                    *p++=prunInfo->msg.link.bSts;
                    p_send_buffer[512]=LO8(p-p_send_buffer);/*有效数据区大小*/
                    p_send_buffer[513]=HI8(p-p_send_buffer);
                    p_send_buffer[514]=LO8(0x9700);
                    p_send_buffer[515]=HI8(0x9700);
                    retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    taskDelay(1);
                    retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);

                    break;

                case ERR_OCR:
                    p=p_send_buffer;
                    *p++=0x00;
                    *p++=0x00;
                    *p++=0x01;
                    *p++=0x05;
                    *p++=LO8(prunInfo->msg.err.unRptSN);
                    *p++=HI8(prunInfo->msg.err.unRptSN);
                    *p++=0x00;
                    retcode=TM_To_Dttm(prunInfo->msg.err.ulTime,&dttm);
                    if(retcode==EP_SUCCESS)
                    {
                        *p++=dttm.ucHour;
                    }
                    else
                    {
                        *p=dttm.ucHour;
                        *p|=0x80;
                        p++;
                    }
                    p_send_buffer[0] |= dttm.ucIrigbLSFlag;  /*闰秒时间标志*/
                    SYN_LOG("kevin 0x8400  2 : 时-分-秒-毫秒-微秒: %d-%d-%d-%d-%d,  闰秒FLAG: 0x%x\n",
                            dttm.ucHour, dttm.ucMinute, dttm.ucSec, dttm.unMSEL, dttm.unMicroSec, p_send_buffer[0]);
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);
                    *p++=LO8(prunInfo->msg.err.unErrCode);
                    *p++=HI8(prunInfo->msg.err.unErrCode);
                    err_msg_len=strlen(prunInfo->msg.err.aucNote);
                    uctemp_val=err_msg_len/8+1;
                    *p++=uctemp_val;
                    for(i=0; i<(uctemp_val-1); i++)
                    {
                        *p++=0x68;
                        memcpy(p,prunInfo->msg.err.aucNote+i*8,8);
                        p+=8;
                    }
                    if(err_msg_len%8)
                    {
                        *p++=0x68;
                        memcpy(p,prunInfo->msg.err.aucNote+(uctemp_val-1)*8,err_msg_len%8+1);
                        p+=err_msg_len%8+1;
                    }
                    else
                    {
                        *p++=0x68;
                        *p++='\0';
                    }

                    p_send_buffer[512]=LO8(17+p_send_buffer[16]*9);/*有效数据区大小*/
                    p_send_buffer[513]=HI8(17+p_send_buffer[16]*9);
                    p_send_buffer[514]=LO8(0x8400);
                    p_send_buffer[515]=HI8(0x8400);
                    retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    taskDelay(1);
                    retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    break;

                case ERR_OCR_STAT:
                    p=p_send_buffer;
                    *p++=0x00;
                    if(prunInfo->msg.errstat.ulSts==0)
                    {
                        *p++=1;
                    }
                    else
                    {
                        *p++=0;
                    }
                    *p++=0x01;
                    *p++=0x05;
                    *p++=LO8(prunInfo->msg.errstat.unRptSN);
                    *p++=HI8(prunInfo->msg.errstat.unRptSN);
                    *p++=0x00;
                    retcode=TM_To_Dttm(prunInfo->msg.errstat.ulTime,&dttm);
                    if(retcode==EP_SUCCESS)
                    {
                        *p++=dttm.ucHour;
                    }
                    else
                    {
                        *p=dttm.ucHour;
                        *p|=0x80;
                        p++;
                    }
                    p_send_buffer[0] |= dttm.ucIrigbLSFlag;  /*闰秒时间标志*/
                    SYN_LOG("kevin 0x8400  3 : 时-分-秒-毫秒-微秒: %d-%d-%d-%d-%d,  闰秒FLAG: 0x%x\n",
                            dttm.ucHour, dttm.ucMinute, dttm.ucSec, dttm.unMSEL, dttm.unMicroSec, p_send_buffer[0]);
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);
                    *p++=LO8(prunInfo->msg.errstat.unErrCode);
                    *p++=HI8(prunInfo->msg.errstat.unErrCode);
                    err_msg_len=strlen(prunInfo->msg.errstat.aucNote);
                    uctemp_val=err_msg_len/8+1;
                    *p++=uctemp_val;
                    for(i=0; i<(uctemp_val-1); i++)
                    {
                        *p++=0x68;
                        memcpy(p,prunInfo->msg.errstat.aucNote+i*8,8);
                        p+=8;
                    }
                    if(err_msg_len%8)
                    {
                        *p++=0x68;
                        memcpy(p,prunInfo->msg.errstat.aucNote+(uctemp_val-1)*8,err_msg_len%8+1);
                        p+=err_msg_len%8+1;
                    }
                    else
                    {
                        *p++=0x68;
                        *p++='\0';
                    }

                    p_send_buffer[512]=LO8(17+p_send_buffer[16]*9);/*有效数据区大小*/
                    p_send_buffer[513]=HI8(17+p_send_buffer[16]*9);
                    p_send_buffer[514]=LO8(0x8400);
                    p_send_buffer[515]=HI8(0x8400);
                    retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    taskDelay(1);
                    retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer,
                                     512+2+2,NO_WAIT,MSG_PRI_NORMAL);
                    break;

                default:
                    break;
            }
        }
    }
}

#ifdef GXC01U
/*下面这部分程序是为了支持和GXC01U人机界面耦合功能*/

MSG_Q_ID SendEventToGxcMsgQFd;		/* 到MMI队列 */
BOOL serverGxc_OK = FALSE;
BOOL MsgQGxc_OK = FALSE;

/***********************************************************************
* WaitAutoSendToGxc01UTask - 获取信号量后进行主动上传报文处理
*专用于将事件送给GXC01U的人机界面处理程序
* RETURNS: EP_SUCCESS，正常返回
*          EP_ERROR，  错误返回
*
*/
int WaitAutoSendToGxc01UTask(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    uint8_t *p;

    /*现在该长度并未考虑所有类型事件名称，需注意不能超出长度*/
    uint8_t p_send_buffer[8+MAX_ID_LEN+12];
    u_int autoIdx=0;
    const VI_RUN_INFO *prunInfo;
    EP_DATE_TIME dttm;
    uint16_t err_msg_len;
    int retcode;
    int sem_ok;

    SendEventToGxcMsgQFd = msgQCreate(32, 8+MAX_ID_LEN+12, MSG_Q_FIFO);
    MsgQGxc_OK=TRUE;
    while(1)
    {
        if(serverGxc_OK)
            break;
        else
        {
            taskDelay(sysClkRateGet( )/10);
        }
    }

    while(!GetRptSNProcessState())
    {
        taskDelay(10);
    }

    while(1)
    {


        /*{
            static BOOL bState=TRUE;
            	p=p_send_buffer;
        	*p++=0x04;

                //retcode=TM_To_Dttm(prunInfo->msg.evt.ulTime,&dttm);
                TM_Get_Sys_Time(&dttm);
        	*p++=LO8(dttm.unYear-1900);
        	*p++=dttm.ucMonth;
        	*p++=dttm.ucDate;
        	 *p++=dttm.ucHour;
        	*p++=dttm.ucMinute;
        	*p++=dttm.ucSec;
        	*p++=LO8(dttm.unMSEL);
        	*p++=HI8(dttm.unMSEL);
        	*p++=LO8(dttm.unMicroSec);
        	*p++=HI8(dttm.unMicroSec);

        	*p++=bState;
                bState=!bState;
                ii++;
                sprintf(p,"test io %d\0",ii);

        msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
        	8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                taskDelay(500);
                continue;
        }*/

        sem_ok=semTake(semNewInfGxc_g,WAIT_FOREVER);
        if(sem_ok==ERROR)
            LOG_Dbg_Msg("Take a Event sem and semTake ERROR\n",0,0,0,0,0,0);

        while(ulInfNonComleteCnt_g!=0)
        {
            /*若还有新消息未完全写入消息队列，则等待100ms 2010-4-26 ZY  */
            taskDelay(10);
        }

        prunInfo=VI_Rd_Run_Info(autoIdx);
        autoIdx++;

        /*if (prunInfo->type!=LED_CHG)
            printf("prunInfo->type=%d\n",prunInfo->type);*/
        if(prunInfo!=NULL)
        {
            switch(prunInfo->type)
            {
                case NEW_EVT:
                    if((prunInfo->msg.evt.pcfg->ucType!=32)&&(prunInfo->msg.evt.pcfg->ucType!=33))
                    {
                        p=p_send_buffer;
                        *p++=0x01; /*事件类型*/

                        retcode=TM_To_Dttm(prunInfo->msg.evt.ulTime,&dttm);
                        *p++=LO8(dttm.unYear-1900);
                        *p++=dttm.ucMonth;
                        *p++=dttm.ucDate;
                        *p++=dttm.ucHour;
                        *p++=dttm.ucMinute;
                        *p++=dttm.ucSec;
                        *p++=LO8(dttm.unMSEL);
                        *p++=HI8(dttm.unMSEL);
                        *p++=LO8(dttm.unMicroSec);
                        *p++=HI8(dttm.unMicroSec);

                        *p++=prunInfo->msg.evt.bState?1:0;
                        strcpy(p,prunInfo->msg.evt.pcfg->aucName);

                        msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
                                 8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                    }
                    break;

                case ERR_OCR_STAT:
                    p=p_send_buffer;
                    *p++=0x02; /*事件类型*/
                    retcode=TM_To_Dttm(prunInfo->msg.errstat.ulTime,&dttm);
                    *p++=LO8(dttm.unYear-1900);
                    *p++=dttm.ucMonth;
                    *p++=dttm.ucDate;
                    *p++=dttm.ucHour;
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);
                    *p++=prunInfo->msg.errstat.ulSts?1:0;
                    err_msg_len=strlen(prunInfo->msg.errstat.aucNote);
                    if (err_msg_len>=MAX_ID_LEN)
                        err_msg_len=MAX_ID_LEN-1;
                    memcpy(p,prunInfo->msg.errstat.aucNote,err_msg_len);
                    p+=err_msg_len;
                    *p='\0';

                    msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
                             8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                    break;

                case NEW_SOE:

                    p=p_send_buffer;
                    *p++=0x04;

                    if(prunInfo->msg.soe.ullusCntFrom1970 != 0)
                    {
                        US_CNT_UTC_TIME usUTCtmTmp;
                        usUTCtmTmp.ullusCntFrom1970 = prunInfo->msg.soe.ullusCntFrom1970;
                        Us_UTC_Time_To_Dttm(&usUTCtmTmp, &dttm);
                        retcode=EP_SUCCESS;
                    }
                    else
                    {
                        retcode=TM_To_Dttm(prunInfo->msg.soe.ulTime,&dttm);
                    }

                    *p++=LO8(dttm.unYear-1900);
                    *p++=dttm.ucMonth;
                    *p++=dttm.ucDate;
                    *p++=dttm.ucHour;
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);

                    *p++=(prunInfo->msg.soe.ucDIQ & 0x01);
                    strcpy(p,prunInfo->msg.soe.pcfg->aucName);

                    msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
                             8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                    break;

                case DI_CHG:
                    /* 序号为0到254,共255个
                     */
                    if (prunInfo->msg.di.unCh >= 255)
                    {
                        break;
                    }
                    p=p_send_buffer;
                    *p++=0x04;

                    retcode=TM_To_Dttm(prunInfo->msg.di.ulTime,&dttm);

                    *p++=LO8(dttm.unYear-1900);
                    *p++=dttm.ucMonth;
                    *p++=dttm.ucDate;
                    *p++=dttm.ucHour;
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);

                    *p++=(prunInfo->msg.di.ucDIQ & 0x01);
                    strcpy(p,prunInfo->msg.di.pcfg->aucName);

                    msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
                             8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                    break;


                /*case LED_CHG:
                    p=p_send_buffer;
                    *p++=0x04;

                    //retcode=TM_To_Dttm(prunInfo->msg.led.ulTime,&dttm);
                    TM_Get_Sys_Time(&dttm);
                    *p++=LO8(dttm.unYear-1900);
                    *p++=dttm.ucMonth;
                    *p++=dttm.ucDate;
                    *p++=dttm.ucHour;
                    *p++=dttm.ucMinute;
                    *p++=dttm.ucSec;
                    *p++=LO8(dttm.unMSEL);
                    *p++=HI8(dttm.unMSEL);
                    *p++=LO8(dttm.unMicroSec);
                    *p++=HI8(dttm.unMicroSec);

                    *p++=(prunInfo->msg.led.bSts & 0x01);
                    printf("prunInfo->msg.led.bSts=%04X\n",prunInfo->msg.led.bSts);
                    strcpy(p,prunInfo->msg.led.pcfg->aucName);

                    msgQSend(SendEventToGxcMsgQFd,(char *)p_send_buffer,
                        8+MAX_ID_LEN+12,NO_WAIT,MSG_PRI_NORMAL);
                break;*/

                default:
                    break;
            }
        }
    }
}
#endif

/*  功能:
 *      设置双点开入是否取反(设置后会覆盖以前根据类型判断的逻辑,未调用则遵循之前的逻辑)
 *  参数:
 *      bDpDiValIsCounter, TRUE取反状态, FALSE非取反状态
 *  返回值:
 *     无.
 */
void SetDpDiValIsCounter(BOOL bDpDiValIsCounter)
{
    if(bDpDiValIsCounter)
    {
        n_ucDpDiValMode = DP_DI_VAL_MODE_COUNTER;
    }
    else
    {
        n_ucDpDiValMode = DP_DI_VAL_MODE_POSITIVE;
    }

    return;
}

/* 查看socket描述符 */
void socketShow(void)
{
    logMsg("帧听描述符(5个)!\n", 0, 0, 0, 0, 0, 0);
    logMsg("快速=%d 慢速=%d 前面板=%d 录波=%d sgView连接=%d\n",
           sock1_fd, sock2_fd, sock3_fd, sockrec_fd, sock4_fd, 0);
    logMsg("\n", 0, 0, 0, 0, 0, 0);

    logMsg("服务描述符(7个)!\n", 0, 0, 0, 0, 0, 0);
    logMsg("快速=%d 慢速=%d 前面板=%d 录波=%d sgView连接=%d\n",
           new_fd1, new_fd2, new_fd3, new_fdrec, new_fd4, 0);
    logMsg("主动上送HMI=%d 主动上送sgView=%d\n",
           sock5_fd, sock6_fd, 0, 0, 0, 0);
}

/* 获取通信状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL getInnerComSts(void)
{
    return (server_hmi_1_OK || server_hmi_2_OK || server_hmi_3_OK || server_hmi_rec_OK);
}
