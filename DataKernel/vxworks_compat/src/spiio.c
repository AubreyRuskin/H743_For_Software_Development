#include "spiio.h"
SPI_COM_INFO spiinfo;
SPI_IO_BUF aspibuf_g[MAX_MOD_NUM];

BOOL bNormalSndFlag;

BOOL SIO_Get_DI(void *pvDiCh, uint32_t *pulChgTime, US_CNT_UTC_TIME *putChgtm)
{
    return 1;
}

void *SIO_Init_AI(int iModAddr, u_int uiCh, float *pfRate){
    return NULL;
}

/* initialize the AO channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 * Return:
 *     pointer to this AI channel, NULL if error occur.
 */
void *SIO_Init_AO(int iModAddr, u_int uiCh){
    return NULL;
}

/* initialize the DO channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 * Return:
 *     pointer to this DO channel, NULL if error occur.
 */
void *SIO_Init_DO(int iModAddr, u_int uiCh)
{
    DO_CHANNEL *pdoch = NULL;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DO_PER_MOD);

    if (aspibuf_g[iModAddr].ucModType==IDLE_MODULE)
    {
        aspibuf_g[iModAddr].ucModType=DO_MODULE;
    }
    else if(aspibuf_g[iModAddr].ucModType==DI_MODULE)
    {
        aspibuf_g[iModAddr].ucModType=DIO_MODULE;
    }

    pdoch=&aspibuf_g[iModAddr].chinfo.doch[DO_CLR_CH];
    pdoch->iModAddr=iModAddr;
    pdoch->iChIdx=DO_CLR_CH;
    pdoch->ulPassWd=DO_PASSWORD;

    pdoch=&aspibuf_g[iModAddr].chinfo.doch[uiCh];

    pdoch->iModAddr=iModAddr;
    pdoch->iChIdx=uiCh;
    pdoch->ulPassWd=DO_PASSWORD;

    return pdoch;
}

/* initialize the DI channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 *     ulFilt, filting time, unit is us.
 *     bInvalidDftVal, default value type.
 * Return:
 *     pointer to this DI channel, NULL if error occur.
 */
void *SIO_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt, BOOL  bInvalidDftVal){

    DI_CHANNEL *pdich = NULL;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DI_PER_MOD);

    aspibuf_g[iModAddr].unDiChNum=uiCh+1;

    if (aspibuf_g[iModAddr].ucModType==IDLE_MODULE)
    {
        aspibuf_g[iModAddr].ucModType=DI_MODULE;
    }   
    else if(aspibuf_g[iModAddr].ucModType==DO_MODULE)
    {
        aspibuf_g[iModAddr].ucModType=DIO_MODULE;
    }

    pdich=&aspibuf_g[iModAddr].chinfo.dich[uiCh];

    pdich->ulFiltTime=ulFilt;		
    pdich->ulFltTmp = pdich->ulFltCfg;
    pdich->pIoBuf = (void *)&aspibuf_g[iModAddr];
    pdich->ulInvalidDftVal = (uint32_t)bInvalidDftVal; 

    return pdich;
}

void SIO_Enable_DO(void){
    return;
}

/* disable the start-up DO.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Disable_DO(void){
    return ;
}

/* alarm out.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Enable_Alm(void){
    return ;
}

/* stop alarm out.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Disable_Alm(void){

    return;
}


void SPI_NEW_COM(int nScanInterval){
    return ;
}

void SIO_Mod_Info(int iAddr, SUB_MOD_INFO *pmodinfo)
{
    return ;
}

void SIO_Set_DO(void *pvDoCh, BOOL bClose){
    return ;
}

EP_STATUS SIO_Initialize(void){
    return EP_SUCCESS;
}

uint32_t SIO_GetDiChgTime(void *pSrc){
    return 0;
}

uint64_t SIO_GetDiChgUTCTime(void *pSrc){
    return 0;
}

void SIO_Clr_DO_Keep(void)
{
    return ;
}

int16_t SIO_Get_AI(void *pvAiCh){
    return 0;
}

void SPI_Write(int iModAddr, uint8_t *pucData){
    return;
}