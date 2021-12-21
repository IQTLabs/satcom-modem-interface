#include <IridiumSBD.h>
#include <Arduino.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function

// Ensure MISO/MOSI/SCK pins are not connected to the port replicator board
#include <SPI.h>
#include <SD.h>
#include "messagelog.h"
#define SDCardCSPin 4
#define SDCardDetectPin 7
#define SDCardActivityLEDPin 8
MessageLog unsentMessageLog("unsent.txt", SDCardCSPin, SDCardDetectPin, SDCardActivityLEDPin);
MessageLog sentMessageLog("sent.txt", SDCardCSPin, SDCardDetectPin, SDCardActivityLEDPin);

#define IridiumSerial Serial1
#define DIAGNOSTICS false // Change this to see diagnostics
IridiumSBD modem(IridiumSerial); // Declare the IridiumSBD object

#define IRIDIUM_SLEEP_PIN 16
#define RX_PIN 11
#define TX_PIN 10
#define RX_PAD SERCOM_RX_PAD_0
#define TX_PAD UART_TX_PAD_2

Uart RelaySerial (&sercom1, RX_PIN, TX_PIN, RX_PAD, TX_PAD);
void SERCOM1_Handler()
{
  RelaySerial.IrqHandler();
}

#define AWAKE_INTERVAL (60 * 1000)
#define interruptPin 19

#define LED_BLINK_TIMER 500

uint32_t ledBlinkTimer = 2000000000L;

volatile uint32_t awakeTimer = 0;

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
  Serial.println("Setup Starting!");
  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX_PIN, PIO_SERCOM);
  pinPeripheral(TX_PIN, PIO_SERCOM);

  // Setup SD card pins
  pinMode(SDCardCSPin, OUTPUT);
  pinMode(SDCardDetectPin, INPUT_PULLUP);
  pinMode(SDCardActivityLEDPin, OUTPUT);

  // Initialize SD card interface
  digitalWrite(SDCardActivityLEDPin, HIGH);
  Serial.print("Initializing SD card interface...");
  while (digitalRead(SDCardDetectPin) == LOW) {
    Serial.println("SD card not inserted. Waiting.");
    blinkError(2); 
    delay(1000);
  }
  while (!SD.begin(SDCardCSPin)) {
    Serial.println("Error initializing SD card interface. Check card and wiring.");
    blinkError(2); 
    delay(1000);
  }

  digitalWrite(SDCardActivityLEDPin, LOW);
  Serial.println("success");

  IridiumSerial.begin(19200); // Start the serial port connected to the satellite modem

  // Begin satellite modem operation
  Serial.print(F("Starting modem..."));
  int result = modem.begin();

  if (result == ISBD_SUCCESS) {
    Serial.println("success!");
  }
  else {
    Serial.print(F("Begin failed: error "));
    Serial.println(result);
    while(1) {blinkError(4); delay(1000);}
  }

  // Setup interrupt sleep pin
  setupInterruptSleep();

  // put the iridium modem to sleep until messages need to be sent
  digitalWrite(IRIDIUM_SLEEP_PIN, LOW);
  Serial.println("Setup Finish!");
}

void loop()
{
  messageCheck();
  //sendMessages();
  sleepCheck();
  checkLEDBlink();
}

// messageID returns a string that's usable as a unique identifier
String messageID(String input) {
  String id = String(millis());
  id.concat(input.length());
  return id;
}

void messageCheck() {
  while (RelaySerial.available()) {
    Serial.println("Message received. Processing.");
    String message = RelaySerial.readStringUntil('\n');
    if (message.length() < 1) {
      Serial.println("Error reading message from RelaySerial.");
      continue;
    }
    int bytesWritten = unsentMessageLog.push(message);
    if (bytesWritten < message.length()) {
      Serial.println("Only " + String(bytesWritten) + " bytes of " + message.length() + " were written.");
    }
  }
}

void sendMessages() {
  // wake up iridium modem
  digitalWrite(IRIDIUM_SLEEP_PIN, HIGH);
  delay(1000); // TODO: check if this is long enough for modem to wake up
  while (unsentMessageLog.numMessages() > 0) {
    String message = unsentMessageLog.pop();
    if (message.equals("")) {
      Serial.println("Empty message pulled from message log. Probably an error.");
      continue;
    }
    // TODO: Actually send via Iridum modem
    Serial.println("Sending message: " + message);
    if (sentMessageLog.push(message) < message.length()) {
      Serial.println("Error while saving sent message to log.");
    }
  }
  // put iridium modem back to sleep
  digitalWrite(IRIDIUM_SLEEP_PIN, LOW);
}

void sleepCheck() {
  if (nowTimeDiff(awakeTimer) > AWAKE_INTERVAL) {
    // set pin mode to low
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("sleeping as timed out");
    USBDevice.standby();
    __WFI();  // wake from interrupt
    delay(500);
    Serial.println("wake due to interrupt");
    Serial.println();
    // Prompt relay controller for new messages
    RelaySerial.println();
    // toggle output of built-in LED pin
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void EIC_ISR(void) {
  awakeTimer = millis(); // refresh awake timer.
}

void setupInterruptSleep() {
  // whenever we get an interrupt, reset the awake clock.
  attachInterrupt(digitalPinToInterrupt(interruptPin), EIC_ISR, CHANGE);
  // Set external 32k oscillator to run when idle or sleep mode is chosen
  SYSCTRL->XOSC32K.reg |=  (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND);
  REG_GCLK_CLKCTRL  |= GCLK_CLKCTRL_ID(GCM_EIC) | // generic clock multiplexer id for the external interrupt controller
                       GCLK_CLKCTRL_GEN_GCLK1 |   // generic clock 1 which is xosc32k
                       GCLK_CLKCTRL_CLKEN;        // enable it
  // Write protected, wait for sync
  while (GCLK->STATUS.bit.SYNCBUSY);

  // Set External Interrupt Controller to use channel 4
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4;

  PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks

  // It is either Idle mode or Standby mode, not both.
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   // Enable Standby or "deep sleep" mode
}

unsigned long timeDiff(unsigned long x, unsigned long nowTime) {
  if (nowTime >= x) {
    return nowTime - x;
  }
  return (ULONG_MAX - x) + nowTime;
}

unsigned long nowTimeDiff(unsigned long x) {
  return timeDiff(x, millis());
}

void checkLEDBlink() {
  if (nowTimeDiff(ledBlinkTimer) > LED_BLINK_TIMER) {
    ledBlinkTimer = millis(); // reset the timer
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
