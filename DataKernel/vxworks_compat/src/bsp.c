#include "bsp.h"
// #include "eth_callback.h"
#include <time.h>
#include <stdio_compat.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

uint32_t hdlcRecvNum;

void vxTimeBaseGet (UINT32 * pTbu, UINT32 * pTbl){


    // 获取当前时间戳（以微秒为单位）
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t total_microseconds = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)(ts.tv_nsec / 1000);

    // 将时间戳拆分为高32位和低32位
    if (pTbu) {
        *pTbu = (UINT32)(total_microseconds >> 32);
    }
    if (pTbl) {
        *pTbl = (UINT32)(total_microseconds & 0xFFFFFFFF);
    }
}

unsigned char Get_AD_Chip_Count()
{
    return 0;
}
short Get_AD_Value(unsigned char chipId){
    return 0;
}
void Init_Net()
{
    return;
}
void Init_Telnet()
{
    return ;
}
int IoPinInputHigh(IO_PIN_IN_FUN_TYPE funtype){
    return 0;
}
int IoPinOutputHigh(IO_PIN_OUT_FUN_TYPE funtype, int outVal){
    return 0;
}
int Set_EthIP(unsigned char port, unsigned char *addr){
    return 0;
}
int Set_EthMacAdrs(unsigned char port, unsigned char *addr){
        return 0;
}
int Set_HdlcIP(unsigned char port, unsigned char *addr){
    return 0;
}
void Set_Hdlc_Out_Bit(unsigned char settingBit){
    return ;
}
int goose_send_raw(uint8_t portNum, uint8_t *sendBuf, int sendNum){
    return 0;
}
int sysClkRateGet (void){
    return 0;
}
void    Write_FPGA_Program(){
    return ;
}

int16_t Get_Boot_Context(){
    return -1;
}

int Get_Boot_Info()
{
    return -1;
}
 BOOL IS_Boot_From_Net(){
    return 0;
 }

  int read_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
)
{
    return 0;
}

uint16_t GetBspVer(){
    return 1;
}

int write_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
){
    return 0;
}

BOOL SIO_GetIOExsitSts(int iModAddr){
    return TRUE;
}

BOOL SIO_Is_Open_QD(){
    return 0;
}

int Get_Sys_Hw_Clock(UINT8 *buf){
    return 0;
}
int Set_Sys_Hw_Clock(UINT8 *buf){
    return 0;
}

uint16_t GetBootromVer(){
    return 1;
}

char * sysModel(void){
    return NULL;
}

STATUS sysToMonitor(    int startType      ){
    return ERROR;
}

int sysProcNumGet(void){
    return 0;
}

unsigned int Ffx_Get_Nand_Size_In_MegaByte(){
    return 0;
}

int mCastAddrAdd(unsigned char port, unsigned char *addr){
    struct ifreq ifr;
    int sockfd;
    char ifname[IFNAMSIZ];

    // 1. 创建一个用于ioctl操作的套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // 2. 根据端口号生成接口名称 (例如, port 0 -> "eth0")
    snprintf(ifname, IFNAMSIZ, "eth%d", port);

    // 3. 准备 ifreq 结构体
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // 确保字符串以null结尾

    // 4. 复制6字节的多播MAC地址
    memcpy(ifr.ifr_hwaddr.sa_data, addr, 6);

    // 5. 调用 ioctl 将多播地址添加到网卡驱动
    if (ioctl(sockfd, SIOCADDMULTI, &ifr) < 0) {
        perror("ioctl(SIOCADDMULTI)");
        close(sockfd);
        return -1;
    }

    // 6. 关闭套接字并返回成功
    close(sockfd);
    return 0;
}