/********************************************************************************/
/*                                                                              */
/*      Copyright (c) 2005 SAC                                                  */
/*      All Rights Reserved.                                                    */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/* FILE NAME                                            VERSION                 */
/*                                                                              */
/* lnp_qd.c                                  	      EDP01-04-0.1              */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      EX - Extend arithmetic part.                                            */
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains an example for developing arithmetic parts.          */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      hu zaichao, SAC                                                         */
/*                                                                              */
/* DATA STRUCTURES                                                              */
/*                                                                              */
/*      None.                                                                   */
/*                                                                              */
/* FUNCTIONS                                                                    */
/*                                                                              */
/*      lnp_tblqd                       Init function of the example logic part.*/
/*      lnp_tblqd_init                   Scan function of the example logic part.*/
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      logic.h                         Interface between logic parts to EDP 01.*/
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*      hu zaichao      2005.10.8      Created first version 0.1.               */
/*                                                                              */
/********************************************************************************/


#include "logic.h"
/*#include "LNP_flag.h"//weiyao,2006.1*/

#include "math_compat.h"
#include "relay.h"

/* Scan function of the example logic part. */
static void LNP_TBLQD(struct tag_EP_ELEMENT *pelm);

/*ȡӺ½ */
static void YK_Get(EP_ELEMENT *pelm);

/* Init function of the example logic part.
 * Input:   pelm, working data area.
 * Return:  EP_SUCCESS, initilized OK.
 *          EP_BAD_DATA, input singnal error. */

EP_STATUS YK_Get_Init(EP_ELEMENT *pelm)
{
   assert(pelm); 

	pelm->aulUser[0]=0;
	pelm->aioOut[0].now.bVal=FALSE;
	
	

	//LOG_Dbg_Msg("yk_get int ok .\n", 0, 0, 0, 0, 0, 0);

	
    pelm->Scan_Func=YK_Get;
    
    return EP_SUCCESS;  
}


/*ȡӺ½ */
float fUaTestArray[500];
float fUbTestArray[500];
float fUcTestArray[500];

BOOL ProtectStopFlag=TRUE;
BOOL ProtectContinueFlag=TRUE;
BOOL ProtectGlobalFlag=TRUE;
BOOL bTmpFlag;

static void YK_Get(EP_ELEMENT *pelm)
{

#if 0
float Alt1,Alt2,Alt3;
static uint32_t ulCnt = 0;
static float fVal = 0;
float fTmp;
uint32_t dCounter, ddCounter;
int i;
float fTestVal;
float fUaTest;
float fUbTest;
float fUcTest;
float fUaTest0;
float fUbTest0;
float fUcTest0;

float fIaTest;
float fIbTest;
float fIcTest;
float fIaTest0;
float fIbTest0;
float fIcTest0;

static int lNun=0;
static BOOL CycleFinishFlag=FALSE;
int forPointNum;
COMPLEX *pxUa, *pxUb, *pxUc;
COMPLEX *pxIa, *pxIb, *pxIc;
float *pfUa, *pfUb, *pfUc;
float *pfIa, *pfIb, *pfIc;


	if(pelm->pchart->bSetChg)
	{
		//LOG_Dbg_Msg("Slow Task Setting Changed!%x\n", pelm->ppioIn[33]->now.ulVal, 0, 0, 0, 0, 0);
	}

	pelm->aioOut[13].now.fVal=fUaTest;
		
	dCounter=pelm->pchart->ulScnAiCnt;
	for(i=0; i<24; i++)
	{
		ddCounter=pelm->pchart->ulScnAiCnt;
		if(ddCounter != dCounter)
		{
			//logMsg("Different Counter!Error!\n", 0, 0, 0, 0, 0, 0);
		}
	}
	
	if(pelm->aulUser[0]!=pelm->ppioIn[0]->now.bVal)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;
	
	if(pelm->ppioIn[13]->now.bVal)
	{
		//logMsg("ʼ!    ʼֵΪ=%d!\n", (int)(pelm->ppioIn[7]->now.fVal*1000), 0, 0, 0, 0, 0);
		fVal=0;
		pelm->aioOut[6].now.fVal = 0;
		pelm->ppioIn[7]->now.fVal = 0;
	}
	
	pelm->aulUser[0]=pelm->ppioIn[0]->now.bVal;

   pelm->aioOut[1].now.fVal=1.01; 


	Alt1 = RI_CPLX_MOD(pelm->ppioIn[3]->now.xVal);
	pelm->aioOut[2].now.fVal=50.0/Alt1;

	Alt2 = RI_CPLX_MOD(pelm->ppioIn[4]->now.xVal);
	pelm->aioOut[3].now.fVal=50.0/Alt2;

	Alt3 = RI_CPLX_MOD(pelm->ppioIn[5]->now.xVal);
	pelm->aioOut[4].now.fVal=50.0/Alt3;

	/* pelm->aioOut[5].now.fVal=pelm->aioOut[5].now.fVal+0.001*pelm->ppioIn[6]->now.fVal*Alt1*Alt2; */
	/* pelm->aioOut[5].now.fVal=Alt1*Alt2; */
	/* pelm->aioOut[1].now.fVal=2500.0/pelm->aioOut[5].now.fVal; */ 

	fVal=fVal+0.001*pelm->ppioIn[6]->now.fVal*Alt1*Alt2; 

	pelm->aioOut[6].now.fVal = fVal+pelm->ppioIn[7]->now.fVal;

	if(pelm->ppioIn[13]->now.bVal)
	{
		pelm->ppioIn[13]->now.bVal=FALSE;
		//logMsg("!    ʼֵΪ=%d!\n", (int)(pelm->ppioIn[7]->now.fVal*1000), 0, 0, 0, 0, 0);
	}

	pelm->aioOut[5].now.fVal = pelm->aioOut[6].now.fVal;
	ulCnt++;
	if(ulCnt%(2400*60) == 1)
	{
		/* LOG_Dbg_Msg("ضֵ!%d\n", (int)(pelm->ppioIn[8]->now.fVal*1000), 0, 0, 0, 0, 0); */
		LOG_Dbg_Msg("ErrorFlag = %x GetSysErrFlag()=%x\n", pelm->pchart->ErrorFlag, GetSysErrFlag(), 0, 0, 0, 0);
	}

	pelm->aioOut[15].now.bVal=pelm->ppioIn[0]->now.bVal;
	pelm->aioOut[16].now.ulVal=0x1;
	pelm->aioOut[17].now.ulVal=0x2;	
		
	if(pelm->ppioIn[0]->now.bVal)
	{
		pelm->ppioIn[0]->now.bVal=FALSE;
		logMsg("Begin to Adjust!\n", 0, 0, 0, 0, 0, 0);
		if(fabs(pelm->ppioIn[9]->now.fVal)>FLT_PRECISION)
			fTmp = pelm->ppioIn[10]->now.fVal/pelm->ppioIn[9]->now.fVal;

		if(fTmp<5.5&&fTmp>4.5)
   			pelm->aioOut[7].now.fVal = 5/fTmp; 
		
		if(fabs(pelm->ppioIn[10]->now.fVal)>FLT_PRECISION)
			fTmp = pelm->ppioIn[12]->now.fVal/pelm->ppioIn[10]->now.fVal;
		if(fTmp<5.5&&fTmp>4.5)
   			pelm->aioOut[8].now.fVal = 5/fTmp; 
	}


	pelm->aioOut[9].now.xVal = pelm->ppioIn[3]->now.xVal;
	pelm->aioOut[10].now.xVal = pelm->ppioIn[5]->now.xVal;
	pelm->aioOut[11].now.xVal = pelm->ppioIn[17]->now.xVal;
	pelm->aioOut[12].now.xVal = pelm->ppioIn[18]->now.xVal;

	if(ulCnt%1000 == 1)
	{
		/* LOG_Dbg_Msg("ֵ\n%d	%d	%d\n", (int)(pelm->ppioIn[19]->now.fVal), (int)(pelm->ppioIn[20]->now.fVal*1000), (int)(pelm->ppioIn[21]->now.fVal), 0, 0, 0); */
	}

	if(pelm->pchart->bRecvNewFgCmdFlag)
	{
		//logMsg("źŸ!\n", 0, 0, 0, 0, 0, 0);
	}

	pelm->aioOut[18].now.bVal=pelm->pchart->bRecvNewFgCmdFlag;
	pelm->aioOut[19].now.bVal=pelm->pchart->bRecvNewFgCmdFlag;

	if(pelm->aioOut[19].now.bVal)
	{
		//logMsg("źŵ!\n", 0, 0, 0, 0, 0, 0);
	}

	if((pelm->pchart->ErrorGlobalFlag) && (ProtectGlobalFlag))
	{
		ProtectGlobalFlag=FALSE;
		//logMsg("ȫִ!\n", 0, 0, 0, 0, 0, 0);
	}
	
	if(pelm->pchart->bErrorRelayStop && ProtectStopFlag)
	{
		ProtectStopFlag=FALSE;
		//logMsg("Ϸ˳!\n", 0, 0, 0, 0, 0, 0);
	}

	if(pelm->pchart->bErrorRelayContinue && ProtectContinueFlag)
	{
		ProtectContinueFlag=FALSE;
		//logMsg("Ϸ˳!\n", 0, 0, 0, 0, 0, 0);
	}	


#if 1
	if(ulCnt%50 == 1)
	{
		pelm->aioOut[23].now.bVal	=!pelm->aioOut[23].now.bVal	;
	}

	pelm->aioOut[24].now.bVal=pelm->ppioIn[26]->now.bVal;	
#endif

	if(RI_CPLX_MOD(pelm->ppioIn[17]->now.xVal)>5.0)
	{
		pelm->aioOut[25].now.bVal=TRUE;
	}	

	pelm->aioOut[11].now.xVal=pelm->ppioIn[17]->now.xVal;
	
	if(RI_CPLX_MOD(pelm->ppioIn[17]->now.xVal) >= 1.0)
	{
		pelm->aioOut[12].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[12].now.bVal=FALSE;
	}
	{
		COMPLEX *pxI1;
		COMPLEX *pxI11;
		COMPLEX *pxI12;
		COMPLEX *pxI13;

	pxI1=RD_Calc_AI_P(pelm->ppioIn[17]->pvCh,pelm->pchart->ulScnAiCnt);
  pxI11=RD_Adj_Calc_AI_P(pxI1, 0);
	pxI12=RD_Adj_Calc_AI_P(pxI1, -1);
	pxI13=RD_Adj_Calc_AI_P(pxI1, -2);
	

	pelm->aioOut[13].now.xVal=pelm->ppioIn[17]->now.xVal;
  pelm->aioOut[13].recbuf[0].xVal=*pxI12;
  pelm->aioOut[13].recbuf[1].xVal=*pxI13;

	pelm->aioOut[14].now.fVal=RI_CPLX_MOD(pelm->ppioIn[17]->now.xVal);
  pelm->aioOut[14].recbuf[0].fVal=RI_CPLX_MOD(*pxI12);
  pelm->aioOut[14].recbuf[1].fVal=RI_CPLX_MOD(*pxI13);
}
	/* pelm->aioOut[13].recbuf[0].fVal=Alt1; */

#endif
}
