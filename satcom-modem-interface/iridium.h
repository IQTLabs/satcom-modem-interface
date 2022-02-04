#ifndef _IRIDIUM_H
#define _IRIDIUM_H

#include "modem.h"
#include <Arduino.h>
#include <IridiumSBD.h>

class IridiumModem : public Modem {
  private:
    Uart *uart;
    IridiumSBD *modem;
  public:
    IridiumModem(Uart *, int);
    int begin();
    int send(const char* msg);
    void sleep();
    void wake();
    int getSignalQuality(int &quality);
};

#endif
