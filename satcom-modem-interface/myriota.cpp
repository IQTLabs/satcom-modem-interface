#include "myriota.h"


void begin() {

}

bool Myriota::send(const char* msg) {
  int32_t lat=0, lon=0;
  time_t timestamp=0;
  //TODO parse msg into fields

  const myriota_message message = {1, lat, lon, timestamp};
  
  stream->write((const char *)&message, sizeof(message));
  return true;
}

