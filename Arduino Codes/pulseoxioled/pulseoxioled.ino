#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ---------- OLED SPI CONFIG ---------- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   17
#define OLED_CS   5
#define OLED_RST  16

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK,
  OLED_DC, OLED_RST, OLED_CS
);

/* ---------- MAX30102 ---------- */
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
  Wire.begin(21, 22);

  /* ---------- OLED INIT ---------- */
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 28);
  display.println("Initializing...");
  display.display();

  /* ---------- MAX30102 INIT ---------- */
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }

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

  delay(1000);
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

  /* -------- Finger detection -------- */
  bool fingerPlaced = (irBuffer[bufferLength - 1] > 50000);

  /* -------- HR NORMALIZATION -------- */
  if (validHeartRate && heartRate >= 60 && heartRate <= 100) {
    hrNormalized = heartRate;
  }

  /* -------- SERIAL OUTPUT -------- */
  Serial.println("--------------------------------");
  if (!fingerPlaced) {
    Serial.println("Finger not placed");
  } else {
    Serial.print("HR: ");
    Serial.print(hrNormalized);
    Serial.print(" BPM | SpO2: ");
    Serial.println(spo2);
  }

  /* -------- OLED OUTPUT -------- */
  display.clearDisplay();

  if (!fingerPlaced) {
    display.setTextSize(1);
    display.setCursor(25, 28);
    display.println("Finger not placed");
  } else {
    display.setTextSize(1);
    display.setCursor(0, 5);
    display.println("Health Monitor");

    display.drawLine(0, 15, 128, 15, WHITE);

    display.setTextSize(2);
    display.setCursor(0, 22);
    display.print("HR:");
    display.print(hrNormalized);

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print("SpO2: ");
    if (validSPO2)
      display.print(spo2);
    else
      display.print("--");
    display.print(" %");
  }

  display.display();
  delay(1000);
}
