#include <Wire.h>
#include <SPI.h>
#include <math.h>

/* ================= OLED SPI ================= */
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_CS   5
#define OLED_DC   16
#define OLED_RST  17

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RST, OLED_CS
);

/* ================= MAX30102 ================= */
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

uint32_t irBuffer[100];
uint32_t redBuffer[100];
uint8_t sampleIndex = 0;

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
int hrNormalized = 0;

/* ================= MPU6050 ================= */
#include <MPU6050.h>
MPU6050 mpu;

#define FREE_FALL_THRESHOLD 0.3
#define IMPACT_THRESHOLD    2.5
#define FALL_WINDOW_MS      500

bool freeFall = false;
bool fallDetected = false;
unsigned long freeFallTime = 0;

/* ================= DS18B20 ================= */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
float bodyTemp = 0;

/* ================= TIMING ================= */
unsigned long last100ms = 0;
unsigned long last1s = 0;

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  SPI.begin(18, -1, 23);

  /* OLED */
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED failed");
    while (1);
  }
  display.clearDisplay();
  display.display();

  /* MAX30102 */
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }
  particleSensor.setup(50, 1, 2, 100, 69, 4096);

  /* MPU6050 */
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 not found");
    while (1);
  }

  /* DS18B20 */
  tempSensor.begin();

  Serial.println("System Ready");
}

/* ================= 100 ms SAMPLING ================= */
void sampleSensors() {

  /* MAX30102 */
  if (particleSensor.available() && sampleIndex < 100) {
    redBuffer[sampleIndex] = particleSensor.getRed();
    irBuffer[sampleIndex]  = particleSensor.getIR();
    particleSensor.nextSample();
    sampleIndex++;
  }

  /* MPU6050 */
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float accX = ax / 16384.0;
  float accY = ay / 16384.0;
  float accZ = az / 16384.0;
  float accMag = sqrt(accX*accX + accY*accY + accZ*accZ);

  if (accMag < FREE_FALL_THRESHOLD && !freeFall) {
    freeFall = true;
    freeFallTime = millis();
  }

  if (freeFall && accMag > IMPACT_THRESHOLD &&
      millis() - freeFallTime < FALL_WINDOW_MS) {
    fallDetected = true;
    freeFall = false;
  }

  if (freeFall && millis() - freeFallTime > FALL_WINDOW_MS) {
    freeFall = false;
  }

  /* DS18B20 */
  tempSensor.requestTemperatures();
  bodyTemp = tempSensor.getTempCByIndex(0);
}

/* ================= 1 SECOND PROCESS ================= */
void processAndDisplay() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  bool fingerPlaced = (irBuffer[99] > 50000);

  display.setCursor(0, 0);

  if (!fingerPlaced) {
    display.println("Finger not placed");
    hrNormalized = 0;
  } else {
    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, 100, redBuffer,
      &spo2, &validSPO2,
      &heartRate, &validHeartRate
    );

    if (validHeartRate && heartRate >= 60 && heartRate <= 100)
      hrNormalized = heartRate;

    display.print("HR: ");
    display.println(hrNormalized);

    display.print("SpO2: ");
    display.println(validSPO2 ? spo2 : 0);
  }

  display.print("Temp: ");
  display.print(bodyTemp, 1);
  display.println(" C");

  display.print("Fall: ");
  display.println(fallDetected ? "YES" : "NO");

  display.display();

  fallDetected = false;
  sampleIndex = 0;
}

/* ================= LOOP ================= */
void loop() {
  unsigned long now = millis();

  if (now - last100ms >= 100) {
    last100ms = now;
    sampleSensors();
  }

  if (now - last1s >= 1000) {
    last1s = now;
    if (sampleIndex >= 80) {
      processAndDisplay();
    }
  }
}
