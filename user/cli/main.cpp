#include <iostream>
#include <iomanip>
//#include <string>

#include "term-helper.h"
#include "time-helper.h"
#include "temp-sensor.h"

//using namespace std;

int main(void)
{
    termhelper clterm;
    timehelper cltime;
    TempSensor clsensor;
    struct nxp_simtemp_sample_t stSampleMain;
    float fShowTemp;
    struct tm tm_localtime;
    __u32 u32LocalTimeNs;
    char caTimeText[32] = {"demo"};
    __u8 u8RetVal;
    __u8 u8Cycles;

    cltime.init();

    std::cout<<"opening unistd.\n";

    u8RetVal = clsensor.connect();
    if(u8RetVal != 0)
    {
        std::cout<<"Error opening unistd.\n";
    }

    clsensor.setSampling(1000);

    clterm.config();
    u8Cycles = 0;
    while(u8Cycles++ < 100)
    {
        u8RetVal = clsensor.getSampleBlocking(stSampleMain, 8000);

        if(0 == u8RetVal)
        {
            if(0 == cltime.getRealTime(stSampleMain.timestamp_ns, &tm_localtime, &u32LocalTimeNs))
            {
                strftime(caTimeText, sizeof(caTimeText), "%Y-%m-%dT%H:%M:%S.", &tm_localtime);
                std::cout<<caTimeText;
                u32LocalTimeNs /= 1000000;
                std::cout<<std::setfill('0')<<std::setw(3)<<u32LocalTimeNs<<"Z ";
            }
            else
            {
                std::cout<<"TIME_NOT_AVAILABLE ";
            }

            fShowTemp = (float)stSampleMain.temp_mc/1000;
            std::cout<<std::fixed<<std::setprecision(1)<<"temp="<<fShowTemp<<"C ";

            std::cout<<"alert="<<stSampleMain.flags<<std::endl;
        }

        if(clterm.isKeyDetected())
        {
            int iKey = clterm.getKey();
            if(('q' == iKey) || ('Q' == iKey))
            {
                break;
            }
        }
    }
    clterm.restore();

    clsensor.disconnect();

    std::cout<<"closed unistd.\n";

    return 0;
}
