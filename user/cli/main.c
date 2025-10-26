#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "../../kernel/nxp_simtemp_data.h"
#include "../../kernel/nxp_simtemp_ioctl.h"

int main(void)
{
    struct nxp_simtemp_sample_t sample;
    struct pollfd pfd;
    float fTempToShow;
    int fd, numFdWEv;
    unsigned char u8CyclesToMonitor = 0;
    unsigned int samplingms = 0;
    int thresholdmc = 0;
    unsigned char mode = 0;

    fd = open("/dev/simtemp", (O_RDWR | O_NONBLOCK));

    if(-1 == fd)
    {
        printf("Error opening file.\n");
        return -1;
    }

    // read parameters
    if(ioctl(fd, NXP_SIMTEMP_GET_SAMPLINGMS, &samplingms) < 0)
    {
        printf("Error reading sampling rate.\n");
    }
    else
    {
        printf("RD - Parameter sampling = %u\n", samplingms);
    }
    if(ioctl(fd, NXP_SIMTEMP_GET_THRESHOLDMC, &thresholdmc) < 0)
    {
        printf("Error reading threshold.\n");
    }
    else
    {
        printf("RD - Parameter threshold = %u\n", thresholdmc);
    }
    if(ioctl(fd, NXP_SIMTEMP_GET_MODE, &mode) < 0)
    {
        printf("Error reading mode.\n");
    }
    else
    {
        printf("RD - Parameter mode = %u\n", mode);
    }
    // configure parameters
    samplingms = 1000;
    if(ioctl(fd, NXP_SIMTEMP_SET_SAMPLINGMS, &samplingms) < 0)
    {
        printf("Error writing sampling rate.\n");
    }
    else
    {
        printf("WR - Parameter sampling = %u\n", samplingms);
    }

    pfd.fd = fd;
    pfd.events = (POLLIN | POLLRDNORM);

    while(u8CyclesToMonitor++ < 5) {
        printf("Polling...\n");
        numFdWEv = poll(&pfd, 1, 10000);

        if(numFdWEv < 0)
        {
            printf("Error in poll\n");
        }

        if(numFdWEv > 0)
        {
            if((pfd.revents & POLLIN) == POLLIN)
            {
                read(pfd.fd, &sample, sizeof(sample));
                fTempToShow = sample.temp_mc / 1000.0;
                printf("%llu temp=%.1fC alert=%u\n", sample.timestamp_ns, fTempToShow/*sample.temp_mc*/, ((sample.flags & 2) != 0)? 1: 0);
            }
        }
    }


    close(fd);
    printf("Closed file\n");

    return 0;
}
