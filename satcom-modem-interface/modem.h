#define IRIDIUM_SLEEP_PIN 16
#define IRIDIUM_SIGNAL_QUALITY_THRESHOLD 2

bool sendMessage(IridiumSBD &m, String *message) {
  // wake up iridium modem
  digitalWrite(IRIDIUM_SLEEP_PIN, HIGH);
  delay(1000); // TODO: check if this is long enough for modem to wake up
  Serial.print(F("Sending message..."));
  Serial.println(*message);

  // send via Iridum modem
  int signalQualityResult;
  int signalQuality = -1;
  signalQualityResult = m.getSignalQuality(signalQuality);
  if (signalQualityResult == ISBD_SUCCESS) {
    Serial.print("Signal quality: ");
    Serial.println(signalQuality);
    if (signalQuality >= IRIDIUM_SIGNAL_QUALITY_THRESHOLD) {
      Serial.println("Sending message: " + *message);
      Serial.println(F("This might take several minutes."));
      int sendSBDTextResult;
      sendSBDTextResult = m.sendSBDText(((String)*message).c_str());
      if (sendSBDTextResult == ISBD_SUCCESS) {
        return true;
      } else {
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
  return false;
}
