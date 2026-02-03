#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

/* ---------- Buffers ---------- */
uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength = 100;

/* ---------- Output variables ---------- */
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

/* ---------- Stored stable HR ---------- */
int hrNormalized = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("Initializing MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring.");
    while (1);
  }

  /* Sensor configuration (UNCHANGED) */
  byte ledBrightness = 50;
  byte sampleAverage = 1;
  byte ledMode = 2;
  byte sampleRate = 100;
  int pulseWidth = 69;
  int adcRange = 4096;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  Serial.println("Place finger on sensor...");
}

void loop() {

  /* -------- Fill buffer -------- */
  for (int i = 0; i < bufferLength; i++) {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  /* -------- Calculate HR & SpO2 -------- */
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    bufferLength,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );

  /* -------- Finger detection (OUTPUT ONLY) -------- */
  bool fingerPlaced = (irBuffer[bufferLength - 1] > 50000);

  /* -------- HR NORMALIZATION ONLY -------- */
  if (validHeartRate && heartRate >= 60 && heartRate <= 100) {
    hrNormalized = heartRate;
  }

  /* -------- Output -------- */
  Serial.println("--------------------------------");

  if (!fingerPlaced) {
    Serial.println("Finger not placed");
  } else {
    if (hrNormalized > 0) {
      Serial.print("Heart Rate : ");
      Serial.print(hrNormalized);
      Serial.println(" BPM");
    } else {
      Serial.println("Heart Rate : Calculating...");
    }

    if (validSPO2) {
      Serial.print("SpO2       : ");
      Serial.print(spo2);
      Serial.println(" %");
    } else {
      Serial.println("SpO2       : Invalid");
    }
  }

  delay(1000);
}
