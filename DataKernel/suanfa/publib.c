#include "publib.h"

static void PUB_GetAmpt_Scan(EP_ELEMENT *pelm);

EP_STATUS PUB_GetAmpt(EP_ELEMENT *pelm)
{/* 获得电流的幅值的算法  */
	assert(pelm && pelm->ucType==0);
	
	if (pelm->unInNum!=1)
	{
		LOG_Dbg_Msg("EDP_GetAmpt input number error.\n", 0, 0, 0, 0, 0, 0);
		return EP_BAD_DATA;
	}
	
	assert(pelm->ppioIn[0]); 
	
	if (pelm->ppioIn[0]->ucType!=0 ||   /* In0=amplitude/angle current. */
	pelm->ppioIn[0]->ucAttrib!=0x0A)
	{
		LOG_Dbg_Msg("LPN_GetAmpt input attrib error.\n", 0, 0, 0, 0, 0, 0);
		return EP_BAD_DATA;
	}
	
	assert(pelm->ppioIn[0]->pvCh);
	
	/* Check output attrib. */
	if (pelm->ucOutNum!=1 ||
		pelm->aioOut[0].ucType!=0xFF || /* Out0=Middle value(电流幅值). */
		pelm->aioOut[0].ucAttrib!=0x08 ||
		pelm->aioOut[0].pvCh)
	{
		LOG_Dbg_Msg("LPN_GetAmpt output attrib error.\n", 0, 0, 0, 0, 0, 0);
		return EP_BAD_DATA;
	}
	
	/* Set initial output. */
	pelm->aioOut[0].now.fVal=0.0;
		
	pelm->Scan_Func=PUB_GetAmpt_Scan;
	
	return EP_SUCCESS;
}

static void PUB_GetAmpt_Scan(EP_ELEMENT *pelm)
{                             /* Disturbed judge threshold. */
	pelm->aioOut[0].now.fVal=REAL(pelm->ppioIn[0]->now.xVal);
}