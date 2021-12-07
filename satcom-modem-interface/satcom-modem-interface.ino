#include <IridiumSBD.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function

#define IridiumSerial Serial1
#define DIAGNOSTICS false // Change this to see diagnostics

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);

void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

uint32_t timer = millis();

void setup()
{
  while (!Serial);
  Serial.begin(115200);
  Serial2.begin(115200);
  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);
  IridiumSerial.begin(19200); // Start the serial port connected to the satellite modem

  // Begin satellite modem operation
  Serial.println(F("Starting modem..."));
  int result = modem.begin();
  if (result != ISBD_SUCCESS) {
    Serial.print(F("Begin failed: error "));
    Serial.println(result);
  }
}

void loop()
{
  if (millis() - timer > 2000) {
    timer = millis();
    // Get the IMEI
    char IMEI[16];
    int result = modem.getIMEI(IMEI, sizeof(IMEI));
    if (result != ISBD_SUCCESS)
    {
      Serial.print(F("getIMEI failed: error "));
      Serial.println(result);
      Serial2.print(F("getIMEI failed: error "));
      Serial2.println(result);
    }
    Serial.print(F("IMEI is "));
    Serial.println(IMEI);
    Serial2.print(F("IMEI is "));
    Serial2.println(IMEI);
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
