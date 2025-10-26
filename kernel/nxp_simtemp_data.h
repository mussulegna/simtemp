#ifndef __NXP_SIMTEMP_DATA__
#define __NXP_SIMTEMP_DATA__


#include <linux/types.h>

struct nxp_simtemp_sample_t {
    __u64 timestamp_ns;    // monotonic timestamp
    __s32 temp_mc;         // milli-degree Celcius
    __u32 flags;           // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
} __attribute__((packed));


#endif //__NXP_SIMTEMP_DATA__
