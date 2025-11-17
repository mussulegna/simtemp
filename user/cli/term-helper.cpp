#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <cstring>

#include "term-helper.h"

termhelper::termhelper()
{
}

termhelper::~termhelper()
{
}

void termhelper::config()
{
    struct termios sConfigNew;

    tcgetattr(STDIN_FILENO, &sConfigOrig);
    memcpy(&sConfigNew, &sConfigOrig, sizeof(sConfigNew));

    sConfigNew.c_lflag &= ~(ICANON | ECHO);
    sConfigNew.c_cc[VMIN] = 0;
    sConfigNew.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &sConfigNew);
}

void termhelper::restore()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &sConfigOrig);
}

int termhelper::isKeyDetected()
{
    struct timeval stv = {0L, 0L};
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO+1, &fds, NULL, NULL, &stv);
}

int termhelper::getKey()
{
    int iRetRead;
    unsigned char ucKey; 

    iRetRead = read(STDIN_FILENO, &ucKey, sizeof(ucKey));
    if(iRetRead < 0)
    {
        return iRetRead;
    }
    else
    {
        return ucKey;
    }
}
