#include "myriota.h"

Myriota::Myriota(Stream *s, int sleepPin = -1) : Modem(sleepPin) {
  this->stream = s;
}

int Myriota::begin() {
  pinMode(this->sleepPin, OUTPUT);
  return 0;
}

int Myriota::send(const char* msg) {
  int32_t lat=0, lon=0;
  time_t timestamp=0;
  //TODO parse msg into fields

  const myriota_message message = {1, lat, lon, timestamp};
  
  return stream->write((const char *)&message, sizeof(message));
}

void Myriota::sleep() {
  
}

void Myriota::wake() {
  
}
