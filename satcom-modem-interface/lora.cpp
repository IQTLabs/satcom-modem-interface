#include "lora.h"

LoraModem::LoraModem(int sleepPin) : Modem(sleepPin) {
  this->radio = new RH_RF95();
}

LoraModem::LoraModem(int sleepPin, int chipSelectPin, int interruptPin, int resetPin, float frequency) {
  this->radio = new RH_RF95(chipSelectPin, interruptPin);
  this->resetPin = resetPin;
  this->frequency = frequency;
}

int LoraModem::getSignalQuality(int &quality) {
  quality = this->radio->lastRssi();
  return 0;
}

int LoraModem::begin() {
  this->reset();
  if (!this->radio->init()) {
    Serial.println("Error initializing LoraModem");
    return -1;
  }
  if (!this->radio->setFrequency(this->frequency)) {
    Serial.println("Error setting LoraModem frequency");
    return -1;
  }
  return 0;
}

int LoraModem::send(const char* msg) {
  if (!this->radio->send((uint8_t *)msg, strlen(msg))) {
    Serial.println("Error sending message");
    return -1;
  }
  return 0;
}

void LoraModem::sleep() {
  this->radio->sleep();
}

void LoraModem::wake() {
  // Not implemented. Lora will wake when a message is received/sent or the
  // state of the radio otherwise changes.
}

void LoraModem::reset() {
  digitalWrite(this->resetPin, LOW);
  delay(10);
  digitalWrite(this->resetPin, HIGH);
  delay(10);
}
