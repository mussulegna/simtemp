#ifndef __TEMP_SENSOR__
#define __TEMP_SENSOR__

#include "../../kernel/nxp_simtemp_data.h"

class TempSensor
{
  private:
    int fd_dev;
    struct nxp_simtemp_sample_t stSample;

  public:
    TempSensor();
    ~TempSensor();

    __u8 connect();
    __u8 disconnect();

    __u8 getSampling(__u32 *u32pSampling);
    __u8 getThreshold(__s32 *s32pThreshold);
    __u8 getMode(__u8 *u8pMode);

    __u8 setSampling(__u32 u32NewSampling);
    __u8 setThreshold(__s32 s32NewThreshold);
    __u8 setMode(__u8 u8NewMode);

    __u8 getSampleBlocking(struct nxp_simtemp_sample_t &stSampleStorage, __u16 u16TimeoutMs);
};

#endif //__TEMP_SENSOR__
