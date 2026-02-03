#include <Wire.h>
#include <MPU6050.h>
#include <math.h>

MPU6050 mpu;

// Thresholds
#define FREE_FALL_THRESHOLD 0.3     // g
#define IMPACT_THRESHOLD    2.5     // g
#define FALL_TIME_WINDOW    500     // ms

bool fallDetected = false;  
bool freeFallDetected = false;
unsigned long freeFallTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected successfully");
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw values to g (±2g → 16384 LSB/g)
  float accX = ax / 16384.0;
  float accY = ay / 16384.0;
  float accZ = az / 16384.0;

  float accMagnitude = sqrt(
    accX * accX +
    accY * accY +
    accZ * accZ
  );

  // Stage 1: Free fall detection
  if (accMagnitude < FREE_FALL_THRESHOLD && !freeFallDetected) {
    freeFallDetected = true;
    freeFallTime = millis();
    Serial.println("Free fall detected");
  }

  // Stage 2: Impact detection
  if (freeFallDetected &&
      accMagnitude > IMPACT_THRESHOLD &&
      (millis() - freeFallTime) < FALL_TIME_WINDOW) {

    fallDetected = true;
    freeFallDetected = false;
    Serial.println("!!! FALL DETECTED !!!");
  }

  // Reset if time window exceeded
  if (freeFallDetected &&
      (millis() - freeFallTime) > FALL_TIME_WINDOW) {
    freeFallDetected = false;
  }


  // Print values
  Serial.print("Acc Magnitude: ");
  Serial.print(accMagnitude, 2);
  Serial.print(" g | Fall: ");
  Serial.println(fallDetected);

  // Reset fall flag after reporting
  if (fallDetected) {
    delay(2000);
    fallDetected = false;
  }

  delay(100);
}
