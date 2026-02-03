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
#include <WiFi.h>
#include <HTTPClient.h>

// -------- WiFi CONFIG --------
const char* ssid = "Ryuzaki";
const char* password = "11111111";

// -------- BACKEND CONFIG --------
// Your PC's Local IP
const char* serverUrl = "http://10.33.20.114:5000/api/vitals";
const char* soldierId = "Soldier_1";

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

// Global Acceleration for Sending
float g_accX = 0.0, g_accY = 0.0, g_accZ = 0.0;

// ------------------------- TEMPERATURE SETTINGS (OneWire) -------------------------
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTempC = 0.0;

// ------------------------- TIMING VARIABLES -------------------------
unsigned long lastDeciSecond = 0; // For 0.1s loop (10Hz)
unsigned long lastSecond = 0;     // For 1.0s loop (1Hz)

// ------------------------- BACKEND FUNCTIONS -------------------------
void sendToBackend() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Build JSON Payload manually
    String payload = "{";
    payload += "\"soldierId\":\"" + String(soldierId) + "\",";
    payload += "\"hr\":" + String(displayHR) + ",";
    payload += "\"spo2\":" + String(displaySpO2) + ",";
    payload += "\"temp\":" + String(currentTempC) + ",";
    payload += "\"accX\":" + String(g_accX, 2) + ",";
    payload += "\"accY\":" + String(g_accY, 2) + ",";
    payload += "\"accZ\":" + String(g_accZ, 2) + ",";
    payload += "\"fall\":" + String(fallDetectedFlag ? 1 : 0);
    payload += "}";

    // Serial.println("Sending: " + payload); 

    int httpCode = http.POST(payload);

    if (httpCode > 0) {
      if (httpCode != 200) Serial.printf("HTTP Code: %d\n", httpCode);
    } else {
      Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

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

  // 3. Init WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  display.println("Connecting WiFi...");
  display.display();
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    display.println("WiFi OK");
  } else {
    Serial.println("\nWiFi Failed");
    display.println("WiFi Failed");
  }
  display.display();

  // 4. Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
  }

  // 5. Initialize DS18B20
  sensors.begin();
  // Set to asynchronous mode to not block loop
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); // Request first reading

  // 6. Initialize MAX30102
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
    
    // Update Globals for Backend
    g_accX = accX;
    g_accY = accY;
    g_accZ = accZ;

    float accMagnitude = sqrt(accX*accX + accY*accY + accZ*accZ);

    // Fall Logic:
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
    } else {
      display.println("Status: OK");
    }
    display.display();
    
    // 4. Send to Backend
    sendToBackend();
  }
}
