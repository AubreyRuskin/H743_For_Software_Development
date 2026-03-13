#include "ioLib.h"
#include <sys_stat_compat.h>

int vxworks_mkdir(const char* path){
    return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
