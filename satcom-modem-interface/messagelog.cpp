#include <SPI.h>
#include "SdFat.h"

#include "messagelog.h"

// MessageLog constructs a new MessageLog object. Set activityLEDPin to < 1 to
// disable activity LED functionality.
MessageLog::MessageLog(const char* filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin) {
  this->filename = filename;
  this->sdChipSelectPin = sdChipSelectPin;
  this->sdCardDetectPin = sdCardDetectPin;
  this->activityLEDPin = activityLEDPin;

  // Ensure SD card pin modes are configured
  pinMode(this->activityLEDPin, OUTPUT);
  pinMode(this->sdCardDetectPin, INPUT_PULLUP);
  if (this->activityLEDPin >= 0) {
    pinMode(this->activityLEDPin, OUTPUT);
  }
}

// Wrapper for SDLib::File::println() which implements an activity LED.
// Returns number of bytes written.
int MessageLog::append(String *message) {
  size_t s = 0;
  ledOn();

  // Check SD card status before proceding
  if (digitalRead(this->sdCardDetectPin) == LOW) {
    MESSAGELOG_PRINTLN(F("SD card not inserted."));
    ledOff();
    return s;
  }

  SdFat sd;

  if (!sd.begin(this->sdChipSelectPin, SD_SCK_MHZ(50))) {
    MESSAGELOG_PRINTLN(F("Error initializing SD card interface."));
    ledOff();
    return s;
  }

  SdFile file;

  if (file.open(this->filename.c_str(), O_WRONLY | O_CREAT | O_EXCL)) {
    message->trim();
    s = file.println(*message);
  } else {
    MESSAGELOG_PRINTLN("Unable to open " + this->filename + " with mode FILE_WRITE");
  }

  file.close();
  ledOff();
  return s;
}

// ledOn sets the led pin high
void MessageLog::ledOn() {
  if (this->activityLEDPin >= 0) {
    digitalWrite(this->activityLEDPin, HIGH);
  }
}

// ledOff sets the led pin low
void MessageLog::ledOff() {
  if (this->activityLEDPin >= 0) {
    digitalWrite(this->activityLEDPin, LOW);
  }
}
