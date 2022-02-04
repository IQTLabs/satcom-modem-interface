#ifndef _MODEM_H
#define _MODEM_H

#include <Arduino.h>
#include "wiring_private.h"

class Modem
{
public:
    Modem(int sleepPin = -1) {
        this->sleepPin=sleepPin;
    };

    virtual int begin();
    virtual int send(const char* msg);
    virtual void sleep();
    virtual void wake();
    virtual int getSignalQuality(int &);

protected:
    int sleepPin;
};

#endif
