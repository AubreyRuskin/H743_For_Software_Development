#ifndef __RELAY__
#define __RELAY__

#include	"math_compat.h"
#include	"edpbase.h"
#include	"logic.h"
/* #include	"RE_RelayEngine.h" */
/* #include "realdata.h" */

#ifdef	__cplusplus
extern "C" {
#endif

#define C_PI	3.141592653589793238462643
#define C_DPR	180.0/C_PI	/*degree per radius*/
#define C_RPD	C_PI/180.0	/*radius per degree*/

#define F_DEG(xVal)	(atan2(IMAGE(xVal), REAL(xVal))*C_DPR)
#define F_AMP(xVal)	(sqrt(REAL(xVal)*REAL(xVal)+IMAGE(xVal)*IMAGE(xVal)))

enum ENUM_IO_CLASS
{
	IOC_AICHN =0, IOC_DICHN, IOC_SETTING, IOC_DIMOD, IOC_MXCHN,
	IOC_TEMP	=0xFF
};

typedef COMPLEX	VECTOR;

#ifdef	__cplusplus
}
#endif

#endif