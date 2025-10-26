#ifndef __NXP_SIMTEMP_IOCTL__
#define __NXP_SIMTEMP_IOCTL__

#include <linux/ioctl.h>

#define NXP_SIMTEMP_TYPE    's'

// IOCTL commands.
#define NXP_SIMTEMP_GET_SAMPLINGMS    _IOR(NXP_SIMTEMP_TYPE, 1, unsigned int)
#define NXP_SIMTEMP_SET_SAMPLINGMS    _IOW(NXP_SIMTEMP_TYPE, 2, unsigned int)
#define NXP_SIMTEMP_GET_THRESHOLDMC   _IOR(NXP_SIMTEMP_TYPE, 3, int)
#define NXP_SIMTEMP_SET_THRESHOLDMC   _IOW(NXP_SIMTEMP_TYPE, 4, int)
#define NXP_SIMTEMP_GET_MODE          _IOR(NXP_SIMTEMP_TYPE, 5, unsigned char)
#define NXP_SIMTEMP_SET_MODE          _IOW(NXP_SIMTEMP_TYPE, 6, unsigned char)
#define NXP_SIMTEMP_GET_STATS         _IOR(NXP_SIMTEMP_TYPE, 7, unsigned int)


#endif // __NXP_SIMTEMP_IOCTL
