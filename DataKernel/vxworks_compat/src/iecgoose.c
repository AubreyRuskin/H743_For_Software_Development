#include "iecgoose.h"
#include "edpbase.h"

DA_VALUE 	 	*NewDaValue(){
    return NULL;
}

DA_VALUE 		*NewDaValueDouble(){
    return NULL;
}

SUB_MAP_INFO 	*NewSubMapInfoNode(){
    return NULL;
}

GSE_PUB_INFO 	*NewPubInfoNode(){
    return NULL;
}

PUB_MAP_INFO 	*NewPubMapInfoNode(){
    return NULL;
}

GSE_SUB_INFO *QuerySubByIdx(int idx){
    return NULL;
}

STATUS GsDeQue(T_WATT_QUEUE *Q, T_WATT_DATA_ELE **p){
    return EP_ERROR;
}
char *QuerySubIEDNameByIdx(int idx){
    return NULL;
}

BOOL goose_cfg_start(void){
    return FALSE;
}

BOOL GetSubTestModeByMap(SUB_MAP_INFO *pm){
    return FALSE;
}

int Get_Goose_Sub_Num(){
    return 0;
}

void ClearCommStsChangeFlag(){
    return;
}

GSE_PUB_INFO 	*GetPubInfoRootNode(){
    return NULL;
}

void Set_Goose_Platform_Support_Func_Code(uint8_t FuncCode){
    return;
}

GSE_SUB_INFO 	*GetSubInfoRootNode(){
    return NULL;
}

BOOL GetCommStsChangeFlag(){
    return FALSE;
}

int16_t GetPubGoLinkIndexByDoNum(int cpuid, int pointid, int typeId, VALUETYPE *pVtValType)
{
    return -1;
}

BOOL			InitPubGoYabanInfo(){
    return FALSE;
}

STATUS GsEnQue(T_WATT_QUEUE *Q, T_WATT_DATA_ELE **p){
    return ERROR;
}