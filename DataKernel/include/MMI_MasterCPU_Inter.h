#include "vxworks_type.h"
struct WaitAsk
{
    uint16_t            Rpt_Type;       /*报文类型*/
    uint16_t            RI;             /*报文返回码*/
    uint16_t            Wait_time;      /*报文等待确认已经经过的时间*/
    uint8_t             *psb;           /*准备重发报文的发送缓冲区*/
    struct WaitAsk      *next;          /*指向等待链表的下一个元素*/
} ;

struct FileProtect
{
    char                filename[100];  /*文件名*/
    uint8_t             writing_staus;  /*当前文件的写入状态*/
    struct FileProtect  *next;
};

/*文件名列表*/
struct FileProtect *pfplhead=NULL;      /*指向文件写保护链表头*/
char dza_filename[100]="/tffs/set/area";





