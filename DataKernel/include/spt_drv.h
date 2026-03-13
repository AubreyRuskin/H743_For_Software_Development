/* spt_drv.c */

#ifndef		__SPT_DRV_H__
#define		__SPT_DRV_H__

// #include "copyright_wrs.h"
#include <vxworks_type.h>

/*

2011-1-4 cqx finish the first version

*/

#define		MAX_SLOT							        16 /*the bus can support 16 slots max*/

/*define the register offset*/

#define		OFFSET_COUNTER_REGISTER                     0
#define		OFFSET_TIME_REGISTER                        1
#define		OFFSET_ADDRESS_REGISTER                     2
#define		OFFSET_STATUS_REGISTER                      3
#define		OFFSET_COMMAND_REGISTER                     4
#define		OFFSET_SPARE_REGISTER                       5
#define		OFFSET_IRQ_MASK_REGISTER                    6
#define		OFFSET_SLOT_STATUS_REGISTER                 7
#define		OFFSET_SLOT_ARRAY_REGISTER1                 8
#define		OFFSET_SLOT_ARRAY_REGISTER2                 9
#define		OFFSET_SLOT_ARRAY_REGISTER3                 10
#define		OFFSET_SLOT_ARRAY_REGISTER4                 11
#define		OFFSET_MULTICAST_REGISTER1                  12
#define		OFFSET_MULTICAST_REGISTER2                  13
#define		OFFSET_MULTICAST_REGISTER3                  14
#define		OFFSET_MULTICAST_REGISTER4                  15
#define		OFFSET_NUM_REGISTER                         16
#define		OFFSET_SLOT_ATTR_REGISTER1                  17
#define		OFFSET_SLOT_ATTR_REGISTER2                  18
#define		OFFSET_SLOT_ATTR_REGISTER3                  19
#define		OFFSET_SLOT_ATTR_REGISTER4                  20
#define		OFFSET_BASE_REGISTER                        21
#define		OFFSET_RESTORE_BASE_REGISTER                22
#define		OFFSET_BUFFER_REGISTER                      0x2000
#define		OFFSET_READ_OVER_ADDR                       0x4000

/*define the status register bit*/
#define     STATUS_BIT_BUS_IS_OK                        0x00000001
#define     STATUS_BIT_RECEIVE_BUFFER_HAVE_PACKET       0x00000002
#define     STATUS_BIT_RECEIVE_BUFFER_OVERFLOW          0x00000004
#define     STATUS_BIT_TRANSMIT_BUFFER_EMPTY            0x00000008
#define     STATUS_BIT_SLAVE_OR_MASTER                  0x00000010
#define     STATUS_BIT_TOKEN_OBTAIN                     0x00000020
#define     STATUS_BIT_CONFIG_OK_ITSELF                 0x00000040
#define     STATUS_BIT_CONFIG_OK_ALL                    0x00000080
#define     STATUS_BIT_WRITE_BUFFER_SUCCESSFULLY        0x00000100
#define     STATUS_BIT_WRITE_BUFFER_OVERFLOW            0x00000200
#define     STATUS_BIT_MASTER_CONFIG_OK                 0x00000400


/*define max rx function can be register*/
#define		SPT_RX_FUN_MAX                              4

typedef union
{
    unsigned char byte[4];
    unsigned long dword;
} bufUnion;

enum    SPT_PORT
{
    SPT_1 = 0,
    MAX_SPT_COUNT,/*max spt port*/
};



/*function decalare*/

typedef int (*RECVCALLBACK_FUN)(unsigned char, int);
typedef struct drv_ctrl
{
    uint8_t		multicast_register[16]; /*multicast register shadow */
    uint8_t     slot_attr_register[16]; /*slot attr register shadow */
    uint8_t     slot_array_register[16];/*slot array register shadow*/
    char		local_addr;
    int			gooseRxFunCount;
    RECVCALLBACK_FUN recvCallBackFun[SPT_RX_FUN_MAX];
} DRV_CTRL;

extern DRV_CTRL *sptCtrl[MAX_SPT_COUNT];
/*init the common data of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
return value: OK ------function execute successfully.
             ERROR ------function execute failure.*/
int		sptInit (int unit);
/*start the SPT bus,enable the interrupt of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
return value: OK ------function execute successfully.
              ERROR ------function execute failure.*/

int     sptStart( int unit );
/*get the adress of the slot.
parameter: none.
return value: The address of the slot.*/

char		sptGetAddr();
/*get the multicast address of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
        pTable---the array to hold the return value.
        n---the count you want to get. The max count is 16
return value: OK ------function execute successfully.
              ERROR ------function execute failure.*/
int sptMultiCastAddrGet(uint8_t unit, char *pTable, int n);
/*get the slot array address of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
        pTable---the array to hold the return value.
        n---the count you want to get. The max count is 16
return value: OK ------function execute successfully.
              ERROR ------function execute failure.*/
int sptSlotArrayGet(uint8_t unit, char *pTable, int n);

/*get the multicast address of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
        pTable---the array to hold the return value.
        n---the count you want to get. The max count is 16
return value: OK ------function execute successfully.
            ERROR ------function execute failure.*/
int sptMultiCastAddrSet(uint8_t unit, char *pTable,int n);
/*set the slot array of the SPT bus
parameter: unit---reserved for multi SPT system.the value is 0 now.
        pTable---the array to hold the return value.
        n---the count you want to get. The max count is 16
return value: OK ------function execute successfully.
            ERROR ------function execute failure.*/
int sptSlotArraySet(uint8_t unit, char *pTable,int n);

/*get the register of the SPT bus
parameter: offset---register index.The value came from the register offset defined above
return value: OK ------function execute successfully.
             ERROR ------function execute failure.*/
uint32_t	sptGetRegister(int offset);
/*send the data in the buf
parameter: unit---reserved for multi SPT system.the value is 0 now.
        buf---the data pointer to be sent.the data must in the following format
              dest address   -----byte
              source address -----byte
              data length ---high byte
              data length ---low  byte
              data .....  ---the count of the bytes equal to the data length
return value: OK ------function execute successfully,data have been writen to the buf of the SPT controller.
             ERROR ------function execute failure.*/
int		sptSend (int unit, char *buf);
/*register callback function you want deal the data
parameter: unit---reserved for multi SPT system.the value is 0 now.
           recvFun---the function you defined to deal the packet.it compare three parameter.you can call sptGetVal() function to get the data without the address .Attention:the vale it return is DWORD.
                     the first is unit,it is reserved now.It must be zero.
                     the second is the length of the packet .
return value: OK ------ it has deal the data,the driver will release the packet
             ERROR -----it was not deal by the function,and the drive will pass the packet to the next callback function.*/
int reg_SPT_Recv_Fun(uint8_t unit, int (*recvFun)(unsigned char, int));
/*unregister callback function you want deal the data*/
int unreg_SPT_Recv_Fun(uint8_t unit, int (*recvFun)(unsigned char, int));


#endif