#ifndef _MYRIOTA_H
#define _MYRIOTA_H

#include "modem.h"
#include <Arduino.h>

// Maximum size in bytes of individual transmit message.
#define MAX_MESSAGE_SIZE 20

// Format of the messages to be transmitted. Values are little endian
typedef struct {
  uint16_t sequence_number;
  int32_t latitude;   // scaled by 1e7, e.g. -891234567 (south 89.1234567)
  int32_t longitude;  // scaled by 1e7, e.g. 1791234567 (east 179.1234567)
  uint32_t time;      // epoch timestamp
} __attribute__((packed)) myriota_message;

class Myriota: public Modem
{
private:
    Stream * stream;
public:
    Myriota(Stream *, int);
    int begin();
    int send(const char* msg);
};

#endif
