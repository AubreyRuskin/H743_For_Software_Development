#ifndef __SYS_STAT_COMPAT_H__
#define __SYS_STAT_COMPAT_H__

#if defined(__has_include)
#  if __has_include(<sys/stat.h>)
#    include <sys/stat.h>
#  endif
#else
#  include <sys/stat.h>
#endif

#endif