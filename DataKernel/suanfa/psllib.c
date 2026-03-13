#include "psllib.h"

static struct PSL_ANALOG
{
	BOOL		ct_1;
	VECTOR 		Ua,Ub,Uc,Ux,U0;
	VECTOR 		Uab,Ubc,Uca;
	VECTOR 		Ia,Ib,Ic,I0;
	
	VECTOR 		Uab3,Ubc3,Uca3;
	uint8_t		dir_flag[3];
	BOOL		dir_pickup[3];
	float  		Umax, Umin;
}	psl_ac;

/* 相间方向元件 */
static float deg_normalize(float fDeg)
{
	if (fDeg < -180.0)
		return (fDeg+360.0);
	if (fDeg > 180.0)
		return (fDeg-360.0);
	return fDeg;	
}

#define DIR_F_PICKUP	0x80	/*正方向标志*/
#define DIR_F_CHECKED	0x08	/*是否进行过比相标志*/
#define DIR_F_VOLTOK	0x02	/*电压符合标志*/
#define DIR_F_CALLED	0x01	/*无压时比相计算过标志*/
static BOOL dir_assert(VECTOR Ia, VECTOR Ubc, VECTOR Ubc3, uint8_t *dir_flags)
{
	float 		fAngle;
	uint8_t		uFlags =*dir_flags;
	
    if (uFlags & DIR_F_CHECKED)
    { 
    	/*非初次比相判别*/
        if (REAL(Ubc) > 2.0)
        {
        	/* 本次电压符合 */
        	uFlags &=~DIR_F_CALLED;
			if (uFlags & DIR_F_VOLTOK)
            {   
            	/* 两次电压比较符合 */
				fAngle =deg_normalize(IMAGE(Ubc)-IMAGE(Ia));
				if ((fAngle > -90.0) && (fAngle < 30.0))
				    uFlags |=DIR_F_PICKUP;
				else
				    uFlags &=~DIR_F_PICKUP;
            }
            else
            {   
            	/* 上次电压不符合, 置符合标志，但不进行方向判别，保留原方向 */
            	uFlags |=DIR_F_VOLTOK;
            }
        }
        else
        {   
        	/* 本次电压不符合 */
            uFlags &=~DIR_F_VOLTOK;
            if (!(uFlags & DIR_F_CALLED))
            {   
                uFlags |=DIR_F_CALLED;
                fAngle =deg_normalize(IMAGE(Ubc3)-IMAGE(Ia));
                if ((fAngle > -90.0) && (fAngle < 30.0))
                    uFlags |=DIR_F_PICKUP;
                else
                    uFlags &=~DIR_F_PICKUP;
            }
            /* 保留原方向*/
		}
    }
    else
   	{
		/* 初次比相 */
	    uFlags |=DIR_F_CHECKED;
		if (REAL(Ubc) > 1.0)
		{
			uFlags |=DIR_F_VOLTOK;
			uFlags |=DIR_F_PICKUP;
		}
		else
		{
			uFlags &=~DIR_F_VOLTOK;
			uFlags &=~DIR_F_PICKUP;		
	    }
	}
	*dir_flags =uFlags;
	return ((uFlags & DIR_F_PICKUP)?TRUE:FALSE);
}
/*	%IN%:Ua,Ub,Uc,Ux,Ia,Ib,Ic,I0,CT_1A
	%OUT%:AC_OK
*/
static void PSL_CALC_Scan(EP_ELEMENT *pelm)
{
	#if 0
	COMPLEX t;
	COMPLEX *pxUa, *pxUb, *pxUc;
	COMPLEX Ua =pelm->ppioIn[0]->now.xVal;
	COMPLEX Ub =pelm->ppioIn[1]->now.xVal;
	COMPLEX Uc =pelm->ppioIn[2]->now.xVal;
	COMPLEX Ux =pelm->ppioIn[3]->now.xVal;
	COMPLEX Ia =pelm->ppioIn[4]->now.xVal;
	COMPLEX Ib =pelm->ppioIn[5]->now.xVal;
	COMPLEX Ic =pelm->ppioIn[6]->now.xVal;
	COMPLEX I0 =pelm->ppioIn[7]->now.xVal;
	
	psl_ac.ct_1 =pelm->ppioIn[8]->now.bVal;
	
	psl_ac.Ua =F_AMP(Ua)+F_DEG(Ua)*1j;
	psl_ac.Ub =F_AMP(Ub)+F_DEG(Ub)*1j;
	psl_ac.Uc =F_AMP(Uc)+F_DEG(Uc)*1j;
	psl_ac.Ux =F_AMP(Ux)+F_DEG(Ux)*1j;
	t =Ua+Ub+Uc;
	psl_ac.U0 =F_AMP(t)+F_DEG(t)*1j;
	t =Ua-Ub;
	psl_ac.Uab =F_AMP(t)+F_DEG(t)*1j;
	t =Ub-Uc;
	psl_ac.Ubc =F_AMP(t)+F_DEG(t)*1j;
	t =Uc-Ua;
	psl_ac.Uca =F_AMP(t)+F_DEG(t)*1j;
	
	psl_ac.Umax =psl_ac.Umin =REAL(psl_ac.Uab);
	
	if (REAL(psl_ac.Ubc) > psl_ac.Umax)
		psl_ac.Umax =REAL(psl_ac.Ubc);
	if (REAL(psl_ac.Ubc) < psl_ac.Umin)
		psl_ac.Umin =REAL(psl_ac.Ubc);
		
	if (REAL(psl_ac.Uca) > psl_ac.Umax)
		psl_ac.Umax =REAL(psl_ac.Uca);
	if (REAL(psl_ac.Uca) < psl_ac.Umin)
		psl_ac.Umin =REAL(psl_ac.Uca);		
		
	psl_ac.Ia =F_AMP(Ia)+F_DEG(Ia)*1j;
	psl_ac.Ib =F_AMP(Ib)+F_DEG(Ib)*1j;
	psl_ac.Ic =F_AMP(Ic)+F_DEG(Ic)*1j;
	psl_ac.I0 =F_AMP(I0)+F_DEG(I0)*1j;
	
	pxUa=RD_Calc_AI_P(pelm->ppioIn[0]->pvCh, pelm->pchart->ulScnAiCnt-3*uiAiPts_g);
	pxUb=RD_Calc_AI_P(pelm->ppioIn[1]->pvCh, pelm->pchart->ulScnAiCnt-3*uiAiPts_g);
	pxUc=RD_Calc_AI_P(pelm->ppioIn[2]->pvCh, pelm->pchart->ulScnAiCnt-3*uiAiPts_g);
	
	t =*pxUa-*pxUb;
	psl_ac.Uab3 =F_AMP(t)+F_DEG(t)*1j;
	t =*pxUb-*pxUc;
	psl_ac.Ubc3 =F_AMP(t)+F_DEG(t)*1j;
	t =*pxUc-*pxUa;
	psl_ac.Uca3 =F_AMP(t)+F_DEG(t)*1j;	
	
	psl_ac.dir_pickup[0] =dir_assert(psl_ac.Ia, psl_ac.Ubc, psl_ac.Ubc3, &psl_ac.dir_flag[0]);
	psl_ac.dir_pickup[1] =dir_assert(psl_ac.Ib, psl_ac.Uca, psl_ac.Uca3, &psl_ac.dir_flag[1]);
	psl_ac.dir_pickup[2] =dir_assert(psl_ac.Ic, psl_ac.Uab, psl_ac.Uab3, &psl_ac.dir_flag[2]);
	
	pelm->aioOut[0].now.bVal=TRUE;
	#endif
}

EP_STATUS PSL_CALC(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 9);
	assert(pelm->ucOutNum == 1);
	
	psl_ac.dir_flag[0] =psl_ac.dir_flag[1] =psl_ac.dir_flag[2] =0;
	psl_ac.dir_pickup[0] =psl_ac.dir_pickup[1] =psl_ac.dir_pickup[2] =FALSE;
	/* Set initial output. */
	pelm->aioOut[0].now.bVal=FALSE;
		
	pelm->Scan_Func=PSL_CALC_Scan;
	
	return EP_SUCCESS;
}

/*	%IN%:PTDX Enable
	%OUT%:PTDX
*/
static void PSL_PTDX_Scan(EP_ELEMENT *pelm)
{
	float idz;
	
	if (!pelm->ppioIn[0]->now.bVal)
	{
		pelm->aioOut[0].now.bVal=FALSE;
		return;
	}
	if ((REAL(psl_ac.Ua) < 8.0) && (REAL(psl_ac.Ub) < 8.0) && (REAL(psl_ac.Uc) < 8.0))
	{
		idz =(psl_ac.ct_1)?0.05:0.25;
		if ((REAL(psl_ac.Ia) > idz) || (REAL(psl_ac.Ic) > idz))
		{
			pelm->aioOut[0].now.bVal=TRUE;
			return;
		}
	}
	if (REAL(psl_ac.U0) > 8.0)
	{
		if (psl_ac.Umin < 16.0)
		{
			pelm->aioOut[0].now.bVal=TRUE;
			return;
		}
		if ((psl_ac.Umax-psl_ac.Umin) > 16.0)
		{
			pelm->aioOut[0].now.bVal=TRUE;
			return;
		}
	}
	pelm->aioOut[0].now.bVal=FALSE;
}

EP_STATUS PSL_PTDX(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 1);
	assert(pelm->ucOutNum == 1);
	
	/* Set initial output. */
	pelm->aioOut[0].now.bVal=FALSE;
		
	pelm->Scan_Func=PSL_PTDX_Scan;
	
	return EP_SUCCESS;
}

/*
	%IN%:TOC_ENABLE, DIR_ENABLE, LV_ENABLE, Idz, Udz
	%OUT%:TOC_PICKUP, TOC_OPERATE
*/
enum {TOC_ENABLE =0, TOC_DIR, TOC_LV, TOC_ISET, TOC_VSET};
enum {TOC_PICKUP =0, TOC_OPERATE};
static void PSL_TOC_Scan(EP_ELEMENT *pelm)
{
	float idz;
	BOOL  pua =FALSE, pub =FALSE, puc =FALSE;

	if(pelm->pchart->bSetChg)
	{
		LOG_Dbg_Msg("Fask Task Setting Changed!\n", 0, 0, 0, 0, 0, 0);
	}

	idz =pelm->ppioIn[TOC_ISET]->now.fVal;
	if (REAL(psl_ac.Ia) > idz)
		pua =TRUE;
	if (REAL(psl_ac.Ib) > idz)
		pub =TRUE;
	if (REAL(psl_ac.Ic) > idz)
		puc =TRUE;
	if (pua || pub || puc)
	{
		pelm->aioOut[TOC_PICKUP].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[TOC_PICKUP].now.bVal =FALSE;
		pelm->aioOut[TOC_OPERATE].now.bVal=FALSE;
		return;
	}
	if (!pelm->ppioIn[TOC_ENABLE]->now.bVal)
	{
		pelm->aioOut[TOC_OPERATE].now.bVal=FALSE;
		return;
	}
	if (pelm->ppioIn[TOC_LV]->now.bVal && (psl_ac.Umin > pelm->ppioIn[TOC_VSET]->now.fVal))
	{
		pelm->aioOut[TOC_OPERATE].now.bVal=FALSE;
		return;
	}
	if (!pelm->ppioIn[TOC_DIR]->now.bVal)
	{
		pelm->aioOut[TOC_OPERATE].now.bVal=TRUE;
		return;
	}
	if ((pua && psl_ac.dir_pickup[0]) ||
		(pub && psl_ac.dir_pickup[1]) ||
		(puc && psl_ac.dir_pickup[2]))
	{
		pelm->aioOut[TOC_OPERATE].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[TOC_OPERATE].now.bVal=FALSE;
	}	
}

EP_STATUS PSL_TOC(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 5);
	assert(pelm->ucOutNum == 2);
	
	pelm->aioOut[TOC_PICKUP].now.bVal =FALSE;
	pelm->aioOut[TOC_OPERATE].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_TOC_Scan;
	
	return EP_SUCCESS;	
}

/*	%IN%:	AC_OK
	%OUT%:	PICKUP
*/
static void PSL_NOCURRENT_Scan(EP_ELEMENT *pelm)
{
	float iw;
	
	iw =(psl_ac.ct_1)?(1.0/16):0.2;
	
	if ((REAL(psl_ac.Ia) < iw) &&
		(REAL(psl_ac.Ib) < iw) &&
		(REAL(psl_ac.Ic) < iw))
	{
		pelm->aioOut[0].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[0].now.bVal=FALSE;
	}
}

EP_STATUS PSL_NOCURRENT(EP_ELEMENT *pelm)
{
	#if 0
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 1);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	#endif
	
	pelm->Scan_Func =PSL_NOCURRENT_Scan;
	
	return EP_SUCCESS;		
}

/*
	%IN%:	OL_ENABLE, OL_ISET
	%OUT%:	PICK_UP
*/
enum {OL_ENABLE =0, OL_ISET};

static void PSL_OVERLOAD_Scan(EP_ELEMENT *pelm)
{
	float idz;
	if (!pelm->ppioIn[OL_ENABLE]->now.bVal)
	{
		pelm->aioOut[0].now.bVal=FALSE;
		return;
	}
	idz =pelm->ppioIn[OL_ISET]->now.fVal;
	
	if ((REAL(psl_ac.Ia) > idz) ||
		(REAL(psl_ac.Ib) > idz) ||
		(REAL(psl_ac.Ic) > idz))
	{
		pelm->aioOut[0].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[0].now.bVal=FALSE;
	}
}

EP_STATUS PSL_OVERLOAD(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 2);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_OVERLOAD_Scan;
	
	return EP_SUCCESS;
}

/* 零序方向元件 */
static BOOL dir0_assert(VECTOR U0, VECTOR I0)
{
	float fDeg;
	
	if (REAL(U0) < 2.0)
		return FALSE;
	
	fDeg =deg_normalize(IMAGE(U0)-IMAGE(I0));
	
	/* [165, 180] || [-75, -180] */
    if ((fDeg > 165.0) || (fDeg < -75.0))
		return TRUE;
	else
		return FALSE;	
}

/* 零序过流元件 */
enum {NOC_ENABLE =0, NOC_DIR, NOC_ISET};
enum {NOC_PICKUP =0, NOC_OPERATE};
static void PSL_NOC_Scan(EP_ELEMENT *pelm)
{
	if (REAL(psl_ac.I0) < pelm->ppioIn[NOC_ISET]->now.fVal)
	{
		pelm->aioOut[NOC_PICKUP].now.bVal =FALSE;
		pelm->aioOut[NOC_OPERATE].now.bVal=FALSE;
		return;
	}
	pelm->aioOut[NOC_PICKUP].now.bVal =TRUE;
	if (!pelm->ppioIn[NOC_ENABLE]->now.bVal)
	{
		pelm->aioOut[NOC_OPERATE].now.bVal=FALSE;
		return;
	}
	if (pelm->ppioIn[NOC_DIR]->now.bVal)
	{
		pelm->aioOut[NOC_OPERATE].now.bVal=dir0_assert(psl_ac.U0, psl_ac.I0);
	}
	else
	{
		pelm->aioOut[NOC_OPERATE].now.bVal=TRUE;
	}
}

EP_STATUS PSL_NOC(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 3);
	assert(pelm->ucOutNum == 2);
	
	pelm->aioOut[NOC_PICKUP].now.bVal =FALSE;
	pelm->aioOut[NOC_OPERATE].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_NOC_Scan;
	
	return EP_SUCCESS;	
}

/*
	%IN%:AC_OK
	%COND%:Uab < 0.3Un && Ubc < 0.3Un && Uca < 0.3Un
	%OUT%:PICKUP
*/
static void PSL_MXWY_Scan(EP_ELEMENT *pelm)
{
	if (psl_ac.Umax < 30.0)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;
}

EP_STATUS PSL_MXWY(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 1);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_MXWY_Scan;
	
	return EP_SUCCESS;			
}

/*
	%IN%:AC_OK
	%COND%:Uab > 0.75Un && Ubc > 0.75Un && Uca > 0.75Un
	%OUT%:PICKUP
*/
static void PSL_MXYY_Scan(EP_ELEMENT *pelm)
{
	if (psl_ac.Umin > 75.0)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;
}

EP_STATUS PSL_MXYY(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 1);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_MXYY_Scan;
	
	return EP_SUCCESS;
}

/*
	%IN%:	AC_OK, UX_SELECTOR
	%COND%: Ux < 0.3Uxn
	%OUT%:	PICKUP
*/
enum {UX_PHASEA =0, UX_PHASEB, UX_PHASEC, UX_PHASEAB, UX_PHASEBC, UX_PHASECA};
enum {XL_OK =0, XL_UX};
static void PSL_XLWY_Scan(EP_ELEMENT *pelm)
{
	float		udz;
	uint32_t	phase =pelm->ppioIn[XL_UX]->now.ulVal;
	if ((phase == UX_PHASEA) || (phase == UX_PHASEB) || (phase == UX_PHASEC))
		udz =0.3*57.7;
	else
		udz =0.3*100.0;
	if (REAL(psl_ac.Ux) < udz)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;	
}

EP_STATUS PSL_XLWY(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 2);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_XLWY_Scan;
	
	return EP_SUCCESS;	
}
/*
	%IN%:	AC_OK, UX_SELECTOR
	%COND%: Ux > 0.75Uxn
	%OUT%:	PICKUP
*/
static void PSL_XLYY_Scan(EP_ELEMENT *pelm)
{
	float		udz;
	uint32_t	phase =pelm->ppioIn[XL_UX]->now.ulVal;
	if ((phase == UX_PHASEA) || (phase == UX_PHASEB) || (phase == UX_PHASEC))
		udz =0.75*57.7;
	else
		udz =0.75*100.0;
	if (REAL(psl_ac.Ux) > udz)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;		
}

EP_STATUS PSL_XLYY(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 2);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_XLYY_Scan;
	
	return EP_SUCCESS;	
}

/*
	%IN%:	AC_OK, TQ_UX, TQ_ANGLE
	%OUT%:	PICKUP
*/
enum {TQ_ENABLE =0, TQ_UX, TQ_ANGLE};
static void PSL_TQ_Scan(EP_ELEMENT *pelm)
{
	uint32_t	phase =pelm->ppioIn[TQ_UX]->now.ulVal;
	VECTOR		um;
	float		fAngle;
	switch (phase)
	{
	case UX_PHASEA:
		um =psl_ac.Ua;
		break;
	case UX_PHASEB:
		um =psl_ac.Ub;
		break;
	case UX_PHASEC:
		um =psl_ac.Uc;
		break;
	case UX_PHASEAB:
		um =psl_ac.Uab;
		break;
	case UX_PHASEBC:
		um =psl_ac.Ubc;
		break;
	case UX_PHASECA:
		um =psl_ac.Uca;
		break;
	default:
		pelm->aioOut[0].now.bVal=FALSE;
		return;
	}
	fAngle =fabs(deg_normalize(IMAGE(um)-IMAGE(psl_ac.Ux)));
	
	if (fAngle < pelm->ppioIn[TQ_ANGLE]->now.fVal)
		pelm->aioOut[0].now.bVal=TRUE;
	else
		pelm->aioOut[0].now.bVal=FALSE;	
}

EP_STATUS PSL_TQ(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 3);
	assert(pelm->ucOutNum == 1);
	
	pelm->aioOut[0].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_TQ_Scan;
	
	return EP_SUCCESS;		
}

/*
	%IN%:	IJS_ENABLE, IJS_START, IJS_LV, IJS_ISET, IJS_VSET
	%OUT%:	IJS_PICKUP, IJS_OPERATE
*/
enum {IJS_ENABLE =0, IJS_START, IJS_LV, IJS_ISET, IJS_VSET};
enum {IJS_PICKUP, IJS_OPERATE};
static void PSL_IJS_Scan(EP_ELEMENT *pelm)
{
	float idz;
	
	if (!pelm->ppioIn[IJS_START]->now.bVal)
	{
		pelm->aioOut[IJS_PICKUP].now.bVal =FALSE;
		pelm->aioOut[IJS_OPERATE].now.bVal=FALSE;
		return;
	}
	idz =pelm->ppioIn[IJS_ISET]->now.fVal;
	
	if ((REAL(psl_ac.Ia) >  idz) ||
		(REAL(psl_ac.Ib) >  idz) ||
		(REAL(psl_ac.Ic) >  idz))
	{
		pelm->aioOut[IJS_PICKUP].now.bVal =TRUE;
	}
	else
	{
		pelm->aioOut[IJS_PICKUP].now.bVal =FALSE;
		pelm->aioOut[IJS_OPERATE].now.bVal=FALSE;
		return;
	}
	if (!pelm->ppioIn[IJS_ENABLE]->now.bVal)
	{
		pelm->aioOut[IJS_OPERATE].now.bVal=FALSE;
		return;
	}
	if (pelm->ppioIn[IJS_LV]->now.bVal && (psl_ac.Umin > pelm->ppioIn[IJS_VSET]->now.fVal))
	{
		pelm->aioOut[IJS_OPERATE].now.bVal=FALSE;
	}
	else
	{
		pelm->aioOut[IJS_OPERATE].now.bVal=TRUE;
	}
}

EP_STATUS PSL_IJS(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 5);
	assert(pelm->ucOutNum == 2);
	
	pelm->aioOut[IJS_PICKUP].now.bVal =FALSE;
	pelm->aioOut[IJS_OPERATE].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_IJS_Scan;
	
	return EP_SUCCESS;	
}

/*
	%IN%:	I0JS_ENABLE, I0JS_START, I0JS_ISET
	%OUT%:	I0JS_PICKUP
*/
enum {I0JS_ENABLE =0, I0JS_START, I0JS_ISET};
enum {I0JS_PICKUP =0, I0JS_OPERATE};
static void PSL_I0JS_Scan(EP_ELEMENT *pelm)
{
	if (!pelm->ppioIn[I0JS_ENABLE]->now.bVal)
	{
		pelm->aioOut[I0JS_PICKUP].now.bVal =FALSE;
		pelm->aioOut[I0JS_OPERATE].now.bVal=FALSE;
		return;
	}
	if (!pelm->ppioIn[I0JS_START]->now.bVal)
	{
		pelm->aioOut[I0JS_PICKUP].now.bVal =FALSE;
		pelm->aioOut[I0JS_OPERATE].now.bVal=FALSE;
		return;
	}
	if (REAL(psl_ac.I0) > pelm->ppioIn[I0JS_ISET]->now.fVal)
	{
		pelm->aioOut[I0JS_PICKUP].now.bVal =TRUE;
		pelm->aioOut[I0JS_OPERATE].now.bVal=TRUE;
	}
	else
	{
		pelm->aioOut[I0JS_PICKUP].now.bVal =FALSE;
		pelm->aioOut[I0JS_OPERATE].now.bVal=FALSE;
	}
}

EP_STATUS PSL_I0JS(EP_ELEMENT *pelm)
{
	assert(pelm && pelm->ucType==0);
	assert(pelm->unInNum == 3);
	assert(pelm->ucOutNum == 2);
	
	pelm->aioOut[I0JS_PICKUP].now.bVal =FALSE;
	pelm->aioOut[I0JS_OPERATE].now.bVal=FALSE;
	
	pelm->Scan_Func =PSL_I0JS_Scan;
	
	return EP_SUCCESS;	
}