#include <SPI.h>
#include <SD.h>
#include "messagelog.h"

// MessageLog constructs a new MessageLog object. Set activityLEDPin to < 1 to
// disable activity LED functionality.
MessageLog::MessageLog(String filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin) {
  this->filename = filename;
  this->sdChipSelectPin = sdChipSelectPin;
  this->sdCardDetectPin = sdCardDetectPin;
  this->activityLEDPin = activityLEDPin;

  // Setup SD card pins
  pinMode(this->activityLEDPin, OUTPUT);
}

// Wrapper for SDLib::File::read() which operates atomically on a File as well
// as implements an activity LED
int MessageLog::read(uint32_t position) {
  ledOn();
  File file = SD.open(this->filename, FILE_READ);
  if (!file) {
    ledOff();
    return -1;
  }
  file.seek(position);
  int c = file.read();
  file.close();
  ledOff();
  return c;
}

// Wrapper for SDLib::File::write() which operates atomically on a File as well
// as implements an activity LED
size_t MessageLog::write(uint8_t c) {
  ledOn();
  File file = SD.open(this->filename, FILE_WRITE);
  if (!file) {
    ledOff();
    return 0;
  }
  size_t s = file.write(c);
  file.close();
  ledOff();
  return s;
}

// Wrapper for SDLib::File::size()
size_t MessageLog::size() {
  ledOn();
  File file = SD.open(this->filename, FILE_READ);
  if (!file) {
    ledOff();
    return 0;
  }
  size_t s = file.size();
  file.close();
  return s;
}

// Dump contents of this->filename to Serial
void MessageLog::dumpToSerial() {
  Serial.println("----------");
  for (int i = 0; i < size(); i++) {
    Serial.print((char)read(i));
  }
  Serial.println("----------");
}

// push places a String on the stack
int MessageLog::push(String message) {
  message.trim();
  Serial.println("push(\"" + message + "\")");
  // Make sure message is terminated with a newline
  message.concat('\n');
  for (int i = 0; i < message.length(); i++) {
    write(message.charAt(i));
  }
  return 0;
}

// pop removes and returns the most recent String on the stack
String MessageLog::pop() {
  // The majority of this method is a workaround for the fact that some versions
  // of the SD library don't support having multiple files open at once.

  // Find penultimate newline and note position
  if (size() == 0) {
    return String();
  }
  int penultimateNewline = 0, curNewline = 0;
  for (int i = 0; i < size(); i++) {
    if (read(i) == '\n') {
      penultimateNewline = curNewline;
      curNewline = i;
    }
  }

  // Get last line
  String lastLine;
  char c;
  for (int i = penultimateNewline; i < size(); i++) {
    c = read(i);
    if (c != -1) {
      lastLine.concat(c);
    }
  }

  // CopyBytes from 0 to the second to last newline position to temp file
  String tempFilename = String((uint16_t)millis());
  tempFilename.concat(".txt");
  
  // Delete and recreate the temp file first in case it already exists
  SD.remove(tempFilename);
  File temp = SD.open(tempFilename, (O_READ | O_WRITE | O_CREAT));
  temp.close();

  // Copy everything to the temp file
  for (int i = 0; i < size(); i++) {
    char c = read(i);
    File temp = SD.open(tempFilename, FILE_WRITE);
    if (!temp) {
      Serial.println("Unable to create temp file " + tempFilename);
      return String();
    }
    if (temp.write(c) != 1) {
      Serial.println("Error writing to temp file " + tempFilename);
      temp.close();
      return String();
    }
    temp.close();
  }

  // Write everything except the last line back to this->filename
  SD.remove(this->filename);
  for (int i = 0; i < penultimateNewline + 1; i++) {
    File temp = SD.open(tempFilename, FILE_READ);
    if (!temp) {
      Serial.println("Unable to open temp file " + tempFilename);
      return String();
    }
    temp.seek(i);
    char c = temp.read();
    if (c == -1) {
      Serial.println("Error reading from temp file " + tempFilename);
      temp.close();
      return String();
    }
    write(c);
  }

  // Delete temp file
  SD.remove(tempFilename);

  // Return line
  lastLine.trim();
  return lastLine;
}

// numMessages returns the number of messages in the stack
int MessageLog::numMessages() {
  int num = 0;
  for (int i = 0; i < size(); i++) {
    char c = read(i);
    if (c == -1) {
      Serial.println("Error reading from " + this->filename);
      return -1;
    }
    if (c == '\n') {
      num++;
    }
  }
  return num;
}

// ledOn sets the led pin high
void MessageLog::ledOn() {
  if (activityLEDPin > 0) {
    digitalWrite(this->activityLEDPin, HIGH);
  }
}

// ledOff sets the led pin low
void MessageLog::ledOff() {
  if (activityLEDPin > 0) {
    digitalWrite(this->activityLEDPin, LOW);
  }
}
