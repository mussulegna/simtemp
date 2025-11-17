#ifndef __TERM_HELPER__
#define __TERM_HELPER__

#include <termios.h>

class termhelper
{
  private:
    struct termios sConfigOrig;

  public:
    termhelper();
    ~termhelper();
    void config();
    void restore();
    int isKeyDetected();
    int getKey();
};

#endif //__TERM_HELPER__
