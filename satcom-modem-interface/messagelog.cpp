#include "messagelog.h"

#define INVALID_CHAR 255

// MessageLog constructs a new MessageLog object. Set activityLEDPin to < 1 to
// disable activity LED functionality.
MessageLog::MessageLog(String filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin) {
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

// Wrapper for SDLib::File::read() which operates atomically on a File as well
// as implements an activity LED
char MessageLog::read(uint32_t position) {
  ledOn();
  File file = SD.open(this->filename, FILE_READ);
  if (!file) {
    ledOff();
    return INVALID_CHAR;
  }
  file.seek(position);
  char c = file.read();
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
  ledOff();
  return s;
}

// normalize ensures the underlying file is properly formatted and is idempotent
int MessageLog::normalize() {
  // Ensure newline is at end of file
  int s = size();
  if (s == 0) {
    if (write('\n') != 1) {
      MESSAGELOG_PRINTLN("Error adding newline to " + this->filename);
      return -1;
    }
    return 0;
  }
  char c = read(size() - 1);
  if (c == INVALID_CHAR) {
    MESSAGELOG_PRINTLN("Error initializing " + this->filename);
    return -1;
  }
  if (c != '\n') {
    if (write('\n') != 1) {
      MESSAGELOG_PRINTLN("Error adding newline to " + this->filename);
      return -1;
    }
  }
  return 0;
}

// Dump contents of this->filename to Serial
void MessageLog::dumpToSerial() {
  normalize();
  Serial.println("----------");
  for (size_t i = 0; i < size(); i++) {
    Serial.print((char)read(i));
  }
  Serial.println("----------");
}

// push places a String on the stack
int MessageLog::push(String message) {
  normalize();
  message.trim();
  MESSAGELOG_PRINTLN("push(\"" + message + "\")");
  // Make sure message is terminated with a newline
  message.concat('\n');
  for (size_t i = 0; i < message.length(); i++) {
    write(message.charAt(i));
  }
  return 0;
}

// pop removes and returns the most recent String on the stack
String MessageLog::pop() {
  normalize();
  MESSAGELOG_PRINTLN("pop()");
  // The majority of this method is a workaround for the fact that some versions
  // of the SD library don't support having multiple files open at once.

  // Find penultimate newline and note position
  if (size() == 0) {
    return String();
  }
  int penultimateNewline = 0, curNewline = 0;
  for (size_t i = 0; i < size(); i++) {
    if (read(i) == '\n') {
      penultimateNewline = curNewline;
      curNewline = i;
    }
  }

  // Get last line
  String lastLine;
  char c;
  for (size_t i = penultimateNewline; i < size(); i++) {
    c = read(i);
    if (c != INVALID_CHAR) {
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
  for (size_t i = 0; i < size(); i++) {
    char c = read(i);
    File temp = SD.open(tempFilename, FILE_WRITE);
    if (!temp) {
      MESSAGELOG_PRINTLN("Unable to create temp file " + tempFilename);
      return String();
    }
    if (temp.write(c) != 1) {
      MESSAGELOG_PRINTLN("Error writing to temp file " + tempFilename);
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
      MESSAGELOG_PRINTLN("Unable to open temp file " + tempFilename);
      return String();
    }
    temp.seek(i);
    char c = temp.read();
    if (c == INVALID_CHAR) {
      MESSAGELOG_PRINTLN("Error reading from temp file " + tempFilename);
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
  normalize();
  int num = 0;
  // start with i = 1 since an "empty" normalized file will still have a
  // newline as the 0th character
  if (size() < 2) {
    return 0;
  }
  for (size_t i = 1; i < size(); i++) {
    char c = read(i);
    if (c == INVALID_CHAR) {
      MESSAGELOG_PRINTLN("Error reading from " + this->filename);
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
