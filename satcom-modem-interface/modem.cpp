#include "modem.h"

void Modem::sleep() {
  digitalWrite(this->sleepPin, LOW);
}