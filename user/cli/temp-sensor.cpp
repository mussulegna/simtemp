#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "temp-sensor.h"

#include "../../kernel/nxp_simtemp_ioctl.h"

TempSensor::TempSensor()
{
}

TempSensor::~TempSensor()
{
}

__u8 TempSensor::connect()
{
    fd_dev = open("/dev/simtemp", (O_RDWR | O_NONBLOCK));

    if(fd_dev != -1)
    {
        // NO error
        return 0;
    }
    else
    {
        // error
        return 1;
    }
}

__u8 TempSensor::disconnect()
{
    close(fd_dev);

    return 0;
}

__u8 TempSensor::getSampling(__u32 *u32pSampling)
{
    __u32 u32Sampling = 0;

    if(ioctl(fd_dev, NXP_SIMTEMP_GET_SAMPLINGMS, &u32Sampling) >= 0)
    {
        *u32pSampling = u32Sampling;
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::getThreshold(__s32 *s32pThreshold)
{
    __s32 s32Threshold = 0;

    if(ioctl(fd_dev, NXP_SIMTEMP_GET_THRESHOLDMC, &s32Threshold) >= 0)
    {
        *s32pThreshold = s32Threshold;
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::getMode(__u8 *u8pMode)
{
    __u8 u8Mode = 0;

    if(ioctl(fd_dev, NXP_SIMTEMP_GET_MODE, &u8Mode) >= 0)
    {
        *u8pMode = u8Mode;
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::setSampling(__u32 u32NewSampling)
{
    if(ioctl(fd_dev, NXP_SIMTEMP_SET_SAMPLINGMS, &u32NewSampling) >= 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::setThreshold(__s32 s32NewThreshold)
{
    if(ioctl(fd_dev, NXP_SIMTEMP_SET_THRESHOLDMC, &s32NewThreshold) >= 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::setMode(__u8 u8NewMode)
{
    if(ioctl(fd_dev, NXP_SIMTEMP_SET_MODE, &u8NewMode) >= 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

__u8 TempSensor::getSampleBlocking(struct nxp_simtemp_sample_t &stSampleStorage, __u16 u16TimeoutMs)
{
    struct pollfd pfd;
    int iNumEvents;
    __u8 u8RetVal;

    pfd.fd = fd_dev;
    pfd.events = (POLLIN | POLLRDNORM);

    iNumEvents = poll(&pfd, 1, u16TimeoutMs);

    if(iNumEvents > 0)
    {
        if((pfd.revents & POLLIN) == POLLIN)
        {
            read(pfd.fd, &stSampleStorage, sizeof(stSampleStorage));
            u8RetVal = 0;
        }
        else
        {
            u8RetVal = 1;
        }
    }
    else if(iNumEvents < 0)
    {
        u8RetVal = 1;
    }
    else
    {
        u8RetVal = 1;
    }

    return u8RetVal;
}
