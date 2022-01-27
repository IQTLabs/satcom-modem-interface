#include <IridiumSBD.h>
#include <Arduino.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function

#define WINDOWS_DEV
#define SDCARD_ENABLE_LED true

// Ensure MISO/MOSI/SCK pins are not connected to the port replicator board
#include "messagelog.h"
MessageLog *sentMessageLog;

#define IridiumSerial Serial1
#define DIAGNOSTICS false // Change this to see diagnostics
IridiumSBD modem(IridiumSerial); // Declare the IridiumSBD object

#define IRIDIUM_SLEEP_PIN 16
#define IRIDIUM_SIGNAL_QUALITY_THRESHOLD 2
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
#define IRIDIUM_LED_BLINK_TIMER 100

uint32_t ledBlinkTimer = 2000000000L;
volatile uint32_t awakeTimer = 0;
String message;

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

  // Setup interrupt sleep pin
  setupInterruptSleep();

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
    sendMessage(&message);
  }
}

void sendMessage(String *message) {
  // wake up iridium modem
  digitalWrite(IRIDIUM_SLEEP_PIN, HIGH);
  delay(1000); // TODO: check if this is long enough for modem to wake up
  Serial.print(F("Sending message..."));
  Serial.println(*message);

  // send via Iridum modem
  int signalQualityResult;
  int signalQuality = -1;
  signalQualityResult = modem.getSignalQuality(signalQuality);
  if (signalQualityResult == ISBD_SUCCESS) {
    Serial.print("Signal quality: ");
    Serial.println(signalQuality);
    if (signalQuality >= IRIDIUM_SIGNAL_QUALITY_THRESHOLD ) {
      Serial.println("Sending message: " + *message);
      Serial.println(F("This might take several minutes."));
      int sendSBDTextResult;
      sendSBDTextResult = modem.sendSBDText(((String)*message).c_str());
      if (sendSBDTextResult != ISBD_SUCCESS) {
        Serial.print(F("sendSBDText failed: error "));
        Serial.println(sendSBDTextResult);
      }
    } else {
      Serial.println(F("Quality should be 2 or higher to send"));
    }
  } else {
    Serial.print(F("SignalQuality failed: error "));
    Serial.println(signalQualityResult);
  }
}

void sleepCheck() {
  if (nowTimeDiff(awakeTimer) > AWAKE_INTERVAL) {
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
    __WFI();  // wake from interrupt
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
  if (nowTimeDiff(ledBlinkTimer) > IRIDIUM_LED_BLINK_TIMER) {
    ledBlinkTimer = millis(); // reset the timer
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  return true;
}
