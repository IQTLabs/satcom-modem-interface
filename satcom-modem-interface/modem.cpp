#include "modem.h"

bool Modem::sleep() {
  digitalWrite(this->sleepPin, LOW);
}