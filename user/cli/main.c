#include <stdio.h>

//#include <conio.h> //kbhit. getch
//#include <ncurses.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <time.h>

#include "../../kernel/nxp_simtemp_data.h"
#include "../../kernel/nxp_simtemp_ioctl.h"

struct termios oldCfg;
long long diff_ns;

void resetTerminalMode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldCfg);
    printf("Reset terminal mode.\n");
}

void setTerminalMode(void)
{
    struct termios newCfg;

    tcgetattr(STDIN_FILENO, &oldCfg);
    //tcgetattr(STDIN_FILENO, &newCfg);
    memcpy(&newCfg, &oldCfg, sizeof(newCfg));

    atexit(resetTerminalMode);

//    cfmakeraw(&newCfg);
    newCfg.c_lflag &= ~(ICANON | ECHO);
    newCfg.c_cc[VMIN] = 0;
    newCfg.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &newCfg);
}

int kbhit(void) 
{
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
}

int getch(void)
{
    int r;
    unsigned char c;
    if((r = read(STDIN_FILENO, &c, sizeof(c))) < 0)
    {
        return r;
    }
    else
    {
        return c;
    }
}

void getDiffTimeValue(void)
{
    struct timespec ts_monotonic, ts_realtime;
    long long monotonic_ns, realtime_ns;
    

    if(clock_gettime(CLOCK_MONOTONIC, &ts_monotonic) == -1)
    {
        //error handling
    }
    if(clock_gettime(CLOCK_REALTIME, &ts_realtime) == -1)
    {
        //error handling
    }

    monotonic_ns = (long long)ts_monotonic.tv_sec * 1000000000LL + ts_monotonic.tv_nsec;
    realtime_ns = (long long)ts_realtime.tv_sec * 1000000000LL + ts_realtime.tv_nsec;
    diff_ns = realtime_ns - monotonic_ns;
}

unsigned int getRealTimeValue(long long time_ns, unsigned char *bufp, unsigned char buf_size)
{
    struct timespec ts_time;
    struct tm *tm_info;

    time_ns += diff_ns;
    ts_time.tv_sec = time_ns / 1000000000LL;
    ts_time.tv_nsec = time_ns % 1000000000LL;

    tm_info = localtime(&ts_time.tv_sec);

    if(tm_info == NULL)
    {
        // error handling localtime
    }

    strftime(bufp, buf_size, "%Y-%m-%dT%H:%M:%S", tm_info);

    return (unsigned int)ts_time.tv_nsec;
}

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

    struct termios prevTerm, newTerm;
    int newKey = 0;

    unsigned char timeStr[32] = {' '};
    unsigned int timeNs = 0;

    /*initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);*/

/*    tcgetattr(STDIN_FILENO, &prevTerm);
    tcgetattr(STDIN_FILENO, &newTerm);
    //newTerm = prevTerm;
    //memcpy
    newTerm.c_lflag &= ~(ICANON | ECHO);
    newTerm.c_cc[VMIN] = 0;
    newTerm.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);*/

    setTerminalMode();

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

    diff_ns = 0;
    getDiffTimeValue();

    while(u8CyclesToMonitor++ < 500) {
        //printf("Polling...\n");
        numFdWEv = poll(&pfd, 1, 4000);

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
                timeNs = getRealTimeValue(sample.timestamp_ns, timeStr, sizeof(timeStr));
                printf("%d \n", timeNs);
                timeNs /= 1000000UL;
                printf("%s.%03dZ temp=%.1fC alert=%u\n", timeStr, timeNs, fTempToShow/*sample.temp_mc*/, ((sample.flags & 2) != 0)? 1: 0);
            }
        }

        /*if(_kbhit()) {
            newKey = _getch();
        }*/

        /*newKey = wgetch();
        if(newKey != ERR)
        {
            if(newKey == 'q')
            {
                break;
            }
        }*/

        /*newKey = getchar();
            printf("KEY = %c (%d)\n", newKey, newKey);
        if(newKey != EOF)
        {
            printf("KEY = %c (%d)\n", newKey, newKey);
            if((newKey == 'q') || (newKey == 'Q'))
            {
                break;
            }
        }*/

        if(kbhit())
        {
            newKey = getch();
            if((newKey == 'q') || (newKey == 'Q'))
            {
                break;
            }
        }
    }

//    tcsetattr(STDIN_FILENO, TCSANOW, &prevTerm);


    close(fd);
    printf("Closed file\n");
//    exit(1);
//    resetTerminalMode();

    return 0;
}
