#line 2 "test.ino"

#include <Arduino.h>
#include <AUnit.h>
#include <IridiumSBD.h>
#include "../satcom-modem-interface/modem.h"
#include "../satcom-modem-interface/timediff.h"

using aunit::TestRunner;

test(sendMessage, check) {
  IridiumSBD modem;
  String test_str = "test";
  assertEqual(false, sendMessage(modem, &test_str));
}

test(timeExpired, check) {
  unsigned long x = millis();
  assertEqual(false, timeExpired(&x, 500, false));
  delay(501);
  assertEqual(true, timeExpired(&x, 500, true));
}

test(timeDiff, check) {
  unsigned long rollover = timeDiff(ULONG_MAX, 1);
  assertEqual((unsigned long)1, rollover);
  unsigned long nonrollover = timeDiff(10, 20);
  assertEqual((unsigned long)10, nonrollover);
}

void setup() {
}

void loop() {
  TestRunner::run();
}
