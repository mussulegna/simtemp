#include <iostream>
#include <fstream>
#include <string>

//using namespace std;

#include "../../kernel/nxp_simtemp_data.h"

int main(void)
{
    int fd = 0;

    //fd = open("/dev/simtemp", O_RDWR | O_NONBLOCK);
    std::ifstream  sensor("/dev/simtemp");

    if(fd == -1)
    {
        return -1;
    }

    //close(fd);
    sensor.close();

    return 0;
}
