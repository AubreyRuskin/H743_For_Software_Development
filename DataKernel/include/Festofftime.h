#ifndef FESTOFFTIME_H
#define FESTOFFTIME_H


#include "stdio_compat.h"
#include "datetime.h"
#include "vxworks_type.h"

#ifdef	__cplusplus
extern "C" {
#endif


/*check Fest is ok */
void CheckFset();

/* read turn off time */
BOOL ReadOfftime(char *TempInfo);


/* write turn off time. */
void WriteOfftime(uint32_t time);


void SetFset(uint32_t iCheck);

BOOL GetFest();

BOOL GetDttmfromStr(char *pbuf,EP_DATE_TIME *pDttm);

#ifdef	__cplusplus
}
#endif

#endif
