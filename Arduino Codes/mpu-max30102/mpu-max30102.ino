#include <Wire.h>
#include <MPU6050.h>
#include <math.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

/* ================= MPU6050 ================= */
MPU6050 mpu;

#define FREE_FALL_THRESHOLD 0.3
#define IMPACT_THRESHOLD    2.5
#define FALL_TIME_WINDOW    500

bool fallDetected = false;
bool freeFallDetected = false;
unsigned long freeFallTime = 0;
unsigned long lastMPURead = 0;

/* ================= MAX30102 ================= */
MAX30105 particleSensor;

uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength = 100;

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

int hrNormalized = 0;

/* ================= Timing ================= */
unsigned long lastMAXRead = 0;

/* ================= Setup ================= */
void setup() {
  Serial.begin(115200);
  Wire.begin();

  /* ---------- MPU6050 ---------- */
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }
  Serial.println("MPU6050 connected");

  /* ---------- MAX30102 ---------- */
  Serial.println("Initializing MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }

  byte ledBrightness = 50;
  byte sampleAverage = 1;
  byte ledMode = 2;
  byte sampleRate = 100;
  int pulseWidth = 69;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode,
                       sampleRate, pulseWidth, adcRange);

  Serial.println("Place finger on sensor...");
}

/* ================= Loop ================= */
void loop() {

  // Run MPU at 500 Hz (every 2ms) — catches impact spikes
  if (micros() - lastMPURead >= 2000) {
    lastMPURead = micros();
    readMPU6050();
  }

  // Run MAX30102 at 10 Hz (every 100ms)
  readMAX30102_100ms();
}

/* ================= MPU6050 ================= */
void readMPU6050() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float accX = ax / 16384.0;
  float accY = ay / 16384.0;
  float accZ = az / 16384.0;

  float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);

  if (accMagnitude < FREE_FALL_THRESHOLD && !freeFallDetected) {
    freeFallDetected = true;
    freeFallTime = millis();
    Serial.println("Free fall detected");
  }

  if (freeFallDetected &&
      accMagnitude > IMPACT_THRESHOLD &&
      (millis() - freeFallTime) < FALL_TIME_WINDOW) {

    fallDetected = true;
    freeFallDetected = false;
    Serial.println("!!! FALL DETECTED !!!");
  }

  if (freeFallDetected && (millis() - freeFallTime) > FALL_TIME_WINDOW)
    freeFallDetected = false;

  if (fallDetected) {
    Serial.println("EMERGENCY FALL ALERT!");
    fallDetected = false;
  }
}

/* ================= MAX30102 ================= */
void readMAX30102_100ms() {
  if (millis() - lastMAXRead < 100) return;   // 0.1 sec
  lastMAXRead = millis();

  /* shift old data */
  for (int i = 0; i < 90; i++) {
    redBuffer[i] = redBuffer[i + 10];
    irBuffer[i] = irBuffer[i + 10];
  }

  /* collect 10 new samples */
  for (int i = 90; i < 100; i++) {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    bufferLength,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );

  bool fingerPlaced = (irBuffer[99] > 50000);

  if (validHeartRate && heartRate >= 60 && heartRate <= 100)
    hrNormalized = heartRate;

  Serial.println("------ VITALS ------");

  if (!fingerPlaced) {
    Serial.println("Finger not placed");
  } else {
    if (hrNormalized > 0) {
      Serial.print("HR   : "); Serial.print(hrNormalized); Serial.println(" BPM");
    } else {
      Serial.println("HR   : Calculating...");
    }

    if (validSPO2) {
      Serial.print("SpO2 : "); Serial.print(spo2); Serial.println(" %");
    } else {
      Serial.println("SpO2 : Invalid");
    }
  }
}
