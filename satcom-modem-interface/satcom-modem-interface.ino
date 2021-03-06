#include <IridiumSBD.h>
#include <Arduino.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function
#include "timediff.h"
#include "sleepmanager.h"

#define WINDOWS_DEV
#define SDCARD_ENABLE_LED true

// Ensure MISO/MOSI/SCK pins are not connected to the port replicator board
#include "messagelog.h"
MessageLog *sentMessageLog;

#define IridiumSerial Serial1
#define DIAGNOSTICS false // Change this to see diagnostics
IridiumSBD modem(IridiumSerial); // Declare the IridiumSBD object

#define RX_PIN 11
#define TX_PIN 10
#define RX_PAD SERCOM_RX_PAD_0
#define TX_PAD UART_TX_PAD_2

#include "modem.h"

#define AWAKE_INTERVAL (60 * 1000)
#define interruptPin 19

Uart RelaySerial (&sercom1, RX_PIN, TX_PIN, RX_PAD, TX_PAD);
void SERCOM1_Handler()
{
  RelaySerial.IrqHandler();
}

#define LED_BLINK_TIMER 500
#define IRIDIUM_LED_BLINK_TIMER 100

uint32_t ledBlinkTimer = 2000000000L;
String message;

SleepManager sleepmanager(interruptPin, AWAKE_INTERVAL);

void setup()
{
  pinMode(IRIDIUM_SLEEP_PIN, OUTPUT);
  // make sure iridium modem is awake
  digitalWrite(IRIDIUM_SLEEP_PIN, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Show we're awake

  Serial.begin(115200);
  RelaySerial.begin(57600);

  delay(2000);
  Serial.println(F("Setup Starting!"));
  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX_PIN, PIO_SERCOM);
  pinPeripheral(TX_PIN, PIO_SERCOM);

  sentMessageLog = new MessageLog("sent.txt", SDCardCSPin, SDCardDetectPin, SDCardActivityLEDPin);

  Serial.println(F("success"));

  IridiumSerial.begin(19200); // Start the serial port connected to the satellite modem

  // Begin satellite modem operation
  Serial.print(F("Starting modem..."));
  int result = modem.begin();

  if (result == ISBD_SUCCESS) {
    Serial.println(F("success!"));
  }
  else {
    Serial.print(F("Begin failed: error "));
    Serial.println(result);
    for (int i = 0; i <= 5; i++) {blinkError(4); delay(1000);}
  }

  // Test modem connectivity & ensure Sparkfun SBD Library is being used
  getIridiumIMEI();

  // put the iridium modem to sleep until messages need to be sent
  digitalWrite(IRIDIUM_SLEEP_PIN, LOW);
  Serial.println(F("Setup Finish!"));
}

void loop()
{
  messageCheck();
  sleepCheck();
  checkLEDBlink();
}

void messageCheck() {
  while (RelaySerial.available()) {
    Serial.println(F("Message received. Processing."));
    message = RelaySerial.readStringUntil('\n');
    if (message.length() < 1) {
      Serial.println(F("Error reading message from RelaySerial."));
      continue;
    }
    if (sentMessageLog->append(&message) < message.length()) {
      Serial.println(F("Error from sentMessageLog->append()"));
    }
    sendMessage(modem, &message);
  }
}

void sleepCheck() {
  if (sleepmanager.SleepTime()) {
    // set pin mode to low
    digitalWrite(LED_BUILTIN, LOW);
    // Ensure SD card activity LED is off before going to sleep
    #if SDCARD_ENABLE_LED
    digitalWrite(SDCardActivityLEDPin, LOW);
    #endif
    Serial.println(F("sleeping as timed out"));
    // put iridium modem to sleep
    digitalWrite(IRIDIUM_SLEEP_PIN, LOW);
    #ifdef WINDOWS_DEV
    USBDevice.detach();
    #else
    USBDevice.standby();
    #endif
    sleepmanager.WFI();
    #ifdef WINDOWS_DEV
    USBDevice.attach();
    #endif
    delay(1000);
    Serial.println(F("wake due to interrupt"));
    Serial.println();
    // Prompt relay controller for new messages
    if (RelaySerial.available()==0) {
      RelaySerial.println();
    }
    // toggle output of built-in LED pin
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void checkLEDBlink() {
  if (timeExpired(&ledBlinkTimer, LED_BLINK_TIMER, true)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif

void blinkError(int count) {
  for (int i=0; i<count; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
  }
}

void getIridiumIMEI() {
  // Get the IMEI
  char IMEI[16];
  int err = modem.getIMEI(IMEI, sizeof(IMEI));
  if (err != ISBD_SUCCESS)
  {
     Serial.print(F("getIMEI failed: error "));
     Serial.println(err);
     return;
  }
  Serial.print(F("IMEI is "));
  Serial.print(IMEI);
  Serial.println(F("."));
}

bool ISBDCallback() {
  // Rapid LED blink to indicate Iridium sending
  if (timeExpired(&ledBlinkTimer, IRIDIUM_LED_BLINK_TIMER, true)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  return true;
}
