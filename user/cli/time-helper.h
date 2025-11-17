#ifndef __TIME_HELPER__
#define __TIME_HELPER__

#include <linux/types.h>
#include <time.h>

class timehelper
{
  private:
    long long diff_ns;

  public:
    timehelper();
    ~timehelper();

    __u8 init();
    __u8 getRealTime(long long current_ns, struct tm *tmp_local, __u32 *u32plocal_ns);
};

#endif //__TIME_HELPER__
