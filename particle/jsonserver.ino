// Particle based variable server, that receives a JSON variable update
// from an M0/M4 acting as I2C master. 
//
// 10k pullups are required if there are no other I2C devices on the
// same bus with pull resistors. 

//     +-----------------------------------------------------+
//     |                                                     |
//     |       +------------+                                |
//     |       |            |                                |
//     +-------+            |                                |
//             |            |                                |
//     +-------+            |                                |
//     |       |            |                                | 3V
//     |       |  PARTICLE  |                                |
//     |       |            |                                |
//     |       |            +-------------+                  |
//     |       |            |             |                  |
//     |       |            +---------+   |                  |
//     |       +------------+         |   |        +----+    |
// GND |                              +---+--------+    +----+
//     |                              |   |        +----+    |
//     |                              |   |          10K     |
//     |                              |   |                  |
//     |                              |   |        +----+    |
//     |       +------------+         |   +--------+    +----+
//     |       |            |         |   |        +----+
//     |       |            |         |   |          10K
//     |       |            |         |   |
//     |       |            |         |   |
//     +-------+            |         |   |
//             |   M0/M4    |         |   |
//             |            |         |   |
//             |            +---------+---+ SCL
//             |            |         |
//             |            +---------+ SDA
//             +------------+

#include <ArduinoJson.h>
#include <Wire.h>

const byte bufsize = 128;
const byte i2caddr = 4;

int detected = 0;
DynamicJsonDocument doc(bufsize);
char buf[bufsize] = {0};

void recvHandler(int _i) {
  char *p = buf;
  while (Wire.available()) {
    *(p++) = Wire.read();
  }
  DeserializationError error = deserializeJson(doc, buf, sizeof(buf));
  if (!error) {
    // Continuously increment, so as not to lose detections if an individual update fails.
    // int will eventually rollover, which the external infrastructure must handle.
    int new_detected = doc["detected"];
    detected += new_detected;
  }
}

void setup() {
  // https://docs.particle.io/cards/firmware/cloud-functions/particle-variable/
  // Best practice is to declare in setup().
  // In theory the push/event API is a closer match, but this may block for seconds/minutes.
  Particle.variable("detected", detected);
  Wire.begin(i2caddr);
  Wire.onReceive(recvHandler);
}

void loop() {
}
