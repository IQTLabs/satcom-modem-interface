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
    int read(uint32_t position);
    size_t size();
};

#endif
