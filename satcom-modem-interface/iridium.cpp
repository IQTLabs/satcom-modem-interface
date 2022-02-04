#include "iridium.h"

IridiumModem::IridiumModem(Uart *u, int sleepPin) : Modem(sleepPin) {
  this->uart = u;
  this->modem = new IridiumSBD((Stream &)u, sleepPin);
}

int IridiumModem::getSignalQuality(int &quality) {
  return this->modem->getSignalQuality(quality);
}

int IridiumModem::begin() {
  this->uart->begin(19200); // Start the serial port connected to the satellite modem

  // Begin satellite modem operation
  int result = this->modem->begin();
  return result;
}

int IridiumModem::send(const char* msg) {
  return this->modem->sendSBDText(msg);
}

void IridiumModem::sleep() {
  digitalWrite(this->sleepPin, LOW);
}

void IridiumModem::wake() {
  digitalWrite(this->sleepPin, HIGH);
}
