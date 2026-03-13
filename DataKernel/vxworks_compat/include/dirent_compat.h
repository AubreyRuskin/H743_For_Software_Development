#ifndef __DIRENT_COMPAT_H__
#define __DIRENT_COMPAT_H__

#if defined(__has_include)
#  if __has_include(<dirent.h>)
#    include <dirent.h>
#  endif
#else
#  include <dirent.h>
#endif

#endif
