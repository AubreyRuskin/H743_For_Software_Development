#ifndef VXWORKS_IO_COMPAT_H
#define VXWORKS_IO_COMPAT_H
#include "ioLib.h"
#define mkdir(path) vxworks_mkdir(path)


#endif