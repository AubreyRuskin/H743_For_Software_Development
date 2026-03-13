#ifndef _CONFIG_H
#define _CONFIG_H

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS



#include <stdio_compat.h>


#define MXML_VERSION "Mini-XML v2.6"

#define inline _inline


extern char	*_mxml_strdup(const char *);
extern char	*_mxml_strdupf(const char *, ...);
extern char	*_mxml_vstrdupf(const char *, va_list);
extern int	_mxml_snprintf(char *, size_t, const char *, ...);
extern int	_mxml_vsnprintf(char *, size_t, const char *, va_list);


#define snprintf 		_mxml_snprintf
#define strdup			_mxml_strdup
#define vsnprintf 		_mxml_vsnprintf



#endif


/*
 * End of "$Id: config.h,v 1.1.4.1 2011/05/25 07:16:02 xu.difei Exp $".
 */
