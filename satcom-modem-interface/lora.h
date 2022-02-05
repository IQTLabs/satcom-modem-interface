#ifndef _LORA_H
#define _LORA_H

#include "modem.h"
#include <SPI.h>
#include <RH_RF95.h>
#define DEFAULT_RFM95_CS 8
#define DEFAULT_RFM95_RST 4
#define DEFAULT_RFM95_INT 3
#define DEFAULT_RF95_FREQ 433.0

class LoraModem : public Modem {
  private:
    RH_RF95 *radio;
    int resetPin;
    float frequency;
  public:
    LoraModem(int);
    LoraModem(int, int, int, int, float);
    int begin();
    int send(const char* msg);
    void sleep();
    void wake();
    int getSignalQuality(int &quality);
    void reset();
};

#endif
