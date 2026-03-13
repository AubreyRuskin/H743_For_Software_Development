#include "GooseInterface.h"
//todo

int get_sub_mea_info_from_iec61850( unsigned char cpuId, unsigned short pointId,
                                    unsigned long appId, char *ldName, char *leafName){
    return -1;
                                    }


int get_sub_rsv_info_from_iec61850(unsigned char cpuId, unsigned short pointId,
                                   unsigned long appId, char **ldName,
                                   char **leafName, int *nNum)
                                   {
                                    return -1;
                                   }



char *QuerySubGcRefByIdx(int iIdx){
    return NULL;
}



BOOL   InitAllGoYabanInfo(){
    return TRUE;
}

BOOL GetSubTestMode(int nIdx){
    return FALSE;
}




BOOL ReadSubStat(int iSubIndex, int  *piRtValidNetCnt, unsigned char *pucRtSubStatArrBase, unsigned char *pucRtSubStatArrOrigin){
    return FALSE;
}


int drive_goose(unsigned long appid, unsigned long ulUsCnt){
    return -1;
}


BOOL   QuerySubGoIdxByDaIdx(int  iDaIdx,int  *piRtSubIdx){
    return FALSE;
}


BOOL readSubGooseDa(int nSubDaIdx, GOOSE_DA_VALUE *Da_Val, int *nStat){
    return FALSE;
}

BOOL readSubGooseDa_T_New(int nSubDaIdx, TIMESTAMP_VALUE *t_Val){
    return FALSE;
}

BOOL   GO_GetActiveGoDIValByDaIndx
(
    int  iDaIndx,
    void *pSubMapData,
    BOOL *pbRtVal
){
    return FALSE;
}

BOOL   GO_GetActiveGoAIValByDaIndx
(
    int  iDaIndx,
    void *pSubMapData,
    float *pfRtVal
)
{
    return FALSE;
}

BOOL   GO_QueryActiveGoDaIdxByAiNum
(uint32_t iGoSrcType,
 int      iAiNum,
 int *piRtDaIndex,
 void **ppSubMapData){
    return FALSE;
 }

 BOOL   GO_QueryActiveGoDaIdxByDiNum(uint32_t iGoSrcType,
                                    int      iDiNum,
                                    int  *   piRtDaIndex,
                                    int *piSubNum,
                                    void **ppSubMapData,
                                    int *piTDaIndex,
                                    void **ppTSubMapData,
                                    VALUETYPE *pValueType,
                                    int **ppiSubYabanIndex,
                                    int  *   piRtDaVtIndex
                                   )
                                   {
                                    return FALSE;
                                   }


BOOL GO_QueryActiveGoTimeSourceDiIdxByDoNum(uint32_t iGoSrcType,
        int iDoNum,
        int *pindex
                                           )
                                           {
                                            return FALSE;
                                           }
                                        
STATUS reg_Int_Goose_Recv_Fun(uint8_t port, eth_cb_func func){
    return ERROR;
}