#include <SPI.h>
#include <SD.h>

// Comment this line out to disable Serial console logging (or redefine as desired)
#define MESSAGELOG_PRINTLN(x) (Serial.println(x))

#ifndef MESSAGELOG_PRINTLN
#define MESSAGELOG_PRINTLN(x) (0)
#endif

#ifndef _MESSAGELOG_H
#define _MESSAGELOG_H

// MessageLog implements a LIFO stack with a file on an SD card
class MessageLog {
  public:
    MessageLog(String filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin);
    int push(String newMessage);
    String pop();
    int numMessages();
    void dumpToSerial();
  private:
    String filename;
    int sdChipSelectPin, sdCardDetectPin, activityLEDPin;
    void ledOn();
    void ledOff();
    unsigned int copyBytes(String sourceFilename, String destFilename, unsigned int start, unsigned int count);
    size_t write(uint8_t);
    char read(uint32_t position);
    size_t size();
    int normalize();
};

#endif
