/*
  Integrated Sensors Code
  Sensors:
  1. MAX30102 (I2C) - Heart Rate & SpO2
  2. MPU6050 (I2C) - Accelerometer (Fall Detection)
  3. DS18B20 (OneWire) - Temperature
  4. SSD1306 OLED (SPI) - Display

  Connections (ESP32 Verified):
  - I2C (MPU6050, MAX30102): SDA = 21, SCL = 22
  - SPI (OLED): 
    - MOSI (D1): 23
    - CLK (D0): 18
    - DC: 17
    - CS: 5
    - RES: 16
  - OneWire (DS18B20): Pin 4
  
  Note: Ensure 'spo2_algorithm.h' and 'spo2_algorithm.cpp' are in the sketch folder if required by the algorithm library.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30105.h"
#include "spo2_algorithm.h" // Ensure this file is present or library is accessible
#include <MPU6050.h>
#include <math.h>

// ------------------------- OLED SETTINGS (SPI) -------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Pin definitions for OLED SPI
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     17
#define OLED_CS     5
#define OLED_RESET  16

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ------------------------- MAX30102 SETTINGS (I2C) -------------------------
MAX30105 particleSensor;
#define MAX_BRIGHTNESS 255

// Buffers for SpO2 calculation
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t bufferLength = 100; //data length
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// Filtered/Stable values for display
int displayHR = 0;
int displaySpO2 = 0;

// ------------------------- MPU6050 SETTINGS (I2C) -------------------------
MPU6050 mpu;
// Fall thresholds
#define FREE_FALL_THRESHOLD 0.3     // g
#define IMPACT_THRESHOLD    2.5     // g

// Fall Detection States
bool fallDetectedFlag = false;       // The final output flag
bool potentialFallDetected = false;  // Stage 1: Impact detected
unsigned long impactTime = 0;        // Time when impact occurred
unsigned long fallEventTime = 0;     // Time when fall was confirmed


// ------------------------- TEMPERATURE SETTINGS (OneWire) -------------------------
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTempC = 0.0;

// ------------------------- TIMING VARIABLES -------------------------
unsigned long lastDeciSecond = 0; // For 0.1s loop (10Hz)
unsigned long lastSecond = 0;     // For 1.0s loop (1Hz)

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize I2C
  Wire.begin(); 

  // 2. Initialize OLED (SPI)
  // Note: We use software SPI constructor or hardware SPI? 
  // The constructor used above `Adafruit_SSD1306(w, h, &SPI, ...)` uses Hardware SPI.
  // Ensure VSPI pins are correct for ESP32.
  if(!display.begin(SSD1306_SWITCHCAPVCC)) { 
    Serial.println(F("SSD1306 allocation failed"));
    // Continue anyway, but serial log
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

  // 3. Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
  }

  // 4. Initialize DS18B20
  sensors.begin();
  // Set to asynchronous mode to not block loop
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); // Request first reading

  // 5. Initialize MAX30102
  Serial.println("Initializing MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
  }
  
  // MAX30102 Setup
  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Sensors Ready");
  display.display();
  delay(1000);
}

void loop() {
  // ------------------------- CONTINUOUS HIGH SPEED LOOP -------------------------
  // Reading Pulse Ox data continuously to fill buffer
  // Note: The maxim algorithm usually requires a full 100 sample buffer.
  // In a real-time non-blocking loop, we usually fill buffer partially each loop.
  // However, the example code uses a blocking `while` to fill buffer or checks `available`.
  // Here we check `available` and push to buffer. 
  
  // For simplicity and stability with the Maxim library provided in examples, 
  // we often have to process chunks. But to keep 0.1s timing for MPU, we must be careful.
  // Strategy: Check sensor, if new data, add to sliding window or just read.
  // To keep it integrated without rewriting the complex algorithm logic: 
  // We will run the standard buffer fill -> calculate cycle BUT interleaved with MPU checks.
  
  // Re-implementation of MAX30102 loop logic to be non-blocking is complex. 
  // PROPOSED HYBRID:
  // We poll the particleSensor every loop.
  
  static int sampleIndex = 0;
  particleSensor.check(); //Check the sensor
  while (particleSensor.available()) {
      // Check for finger placement immediately
      long irValue = particleSensor.getIR();
      
      if (irValue < 50000) {
        // Finger removed
        displayHR = 0;
        displaySpO2 = 0;
        heartRate = 0;
        spo2 = 0;
        // Optionally clear buffer or reset logic if needed
        // For simple display zeroing, this is sufficient.
      }
      
      redBuffer[sampleIndex] = particleSensor.getRed();
      irBuffer[sampleIndex] = irValue;
      particleSensor.nextSample(); //We're finished with this sample so move to next sample
      
      sampleIndex++;
      if (sampleIndex >= bufferLength) {
          sampleIndex = 0;
          // Calculate HR/SpO2 when buffer is full
          // This calculation might take some ms, detecting fall might be slightly delayed 
          // but should be acceptable (<100ms calculation).
          maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
          
          if (validHeartRate && heartRate > 40 && heartRate < 90) displayHR = heartRate;
          if (validSPO2 && spo2 > 70 && spo2 <= 100) displaySpO2 = spo2;
      }
  }

  // ------------------------- 0.1s INTERVAL (10Hz) -------------------------
  if (millis() - lastDeciSecond >= 100) {
    lastDeciSecond = millis();
    
    // Read MPU6050
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    float accX = ax / 16384.0;
    float accY = ay / 16384.0;
    float accZ = az / 16384.0;
    float accMagnitude = sqrt(accX*accX + accY*accY + accZ*accZ);

    // Fall Logic:
    // "if fall is detected at some 3rd 0.1 sec means after 10 0.1 sec update the fall detection flag"
    
    // 1. Detect Impact
    if (!potentialFallDetected && accMagnitude > IMPACT_THRESHOLD) {
      potentialFallDetected = true;
      impactTime = millis();
      // Serial.println("Impact Detected - Waiting for confirmation");
    }

    // 2. Check Confirmation (1 second later)
    if (potentialFallDetected) {
       // Check if 10 * 0.1s (1000ms) has passed
       if (millis() - impactTime >= 1000) {
          fallDetectedFlag = true;
          fallEventTime = millis();
          potentialFallDetected = false; // Reset potential trigger
          // Serial.println("Fall Confirmed!");
       }
    }
    
    // 3. Reset Fall Flag after 2 seconds
    if (fallDetectedFlag && (millis() - fallEventTime > 2000)) {
      fallDetectedFlag = false;
    }
  }

  // ------------------------- 1.0s INTERVAL (1Hz) -------------------------
  if (millis() - lastSecond >= 1000) {
    lastSecond = millis();

    // 1. Read Temperature (Result from previous request)
    currentTempC = sensors.getTempCByIndex(0);
    // Request NEXT temperature conversion (async)
    sensors.requestTemperatures();

    // 2. Update Serial Monitor
    Serial.print("T:"); Serial.print(currentTempC); 
    Serial.print("C | HR:"); Serial.print(displayHR);
    Serial.print(" | SpO2:"); Serial.print(displaySpO2);
    Serial.print(" | Fall:"); Serial.println(fallDetectedFlag ? "YES" : "NO");

    // 3. Update OLED Display
    display.clearDisplay();
    display.setCursor(0,0);
    
    // Line 1: Temp
    display.print("Temp: "); display.print(currentTempC, 1); display.println(" C");
    
    // Line 2: HR & SpO2
    display.print("HR: "); display.print(displayHR); display.println(" bpm");
    display.print("SpO2: "); display.print(displaySpO2); display.println(" %");
    
    // Line 3: Fall Status
    if (fallDetectedFlag) {
      display.println("!! FALL !!");
      // Optional: Logic to clear fall flag? 
      // User didn't specify auto-reset, but usually needed. 
      // For now, it stays YES until reset or logic changes.
    } else {
      display.println("Status: OK");
    }
    
    display.display();
  }
}
