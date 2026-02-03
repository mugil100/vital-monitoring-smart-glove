#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 sensor;

void setup() {
  Serial.begin(115200);
  delay(100);
  Wire.begin(21, 22); // SDA, SCL for ESP32
  Serial.println();
  Serial.println("Waiting for MAX30102...");
}

void loop() {
  // Keep trying until the sensor responds
  if (!sensor.begin()) {            // use default begin() from SparkFun library
    Serial.println("MAX30102 NOT FOUND. Retrying in 1s...");
    delay(1000);
    return;                         // try again
  }

  Serial.println("MAX30102 FOUND. Initializing...");
  sensor.setup();                   // default configuration
  sensor.setPulseAmplitudeRed(0x1F);
  sensor.setPulseAmplitudeIR(0x1F);
  Serial.println("MAX30102 initialized. Reading...");

  // Read loop — will return to top (and retry) if sensor disappears
  uint32_t lastBeat = 0;
  while (true) {
    long irValue = sensor.getIR();  // SparkFun method
    Serial.print("IR = ");
    Serial.print(irValue);

    if (checkForBeat(irValue)) {
      uint32_t now = millis();
      uint32_t delta = now - lastBeat;
      if (lastBeat != 0 && delta > 0) {
        float bpm = 60.0f / (delta / 1000.0f);
        Serial.print("\tBeat detected. BPM = ");
        Serial.print(bpm);
      } else {
        Serial.print("\tBeat detected. BPM = ---");
      }
      lastBeat = now;
    }

    Serial.println();
    delay(25);

    // simple disconnect detection: if sensor returns 0 repeatedly
    static int zeroCount = 0;
    if (irValue <= 0) zeroCount++; else zeroCount = 0;
    if (zeroCount > 50) {          // ~50 * 25ms = ~1.25s of zero readings
      Serial.println("No valid readings — sensor may be disconnected. Returning to scan.");
      break;
    }
  }

  // small delay then outer loop will retry sensor.begin()
  delay(500);
}
