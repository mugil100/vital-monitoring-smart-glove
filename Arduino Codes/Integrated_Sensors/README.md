# Integrated Sensors Project

This project integrates MAX30102, MPU6050, DS18B20, and SSD1306 OLED into a single ESP32 solution.

## Pin Connections (ESP32)

| Sensor      | Interface | Pin Name | ESP32 Pin |
|-------------|-----------|----------|-----------|
| **MAX30102**| I2C       | SDA      | 21        |
|             | I2C       | SCL      | 22        |
| **MPU6050** | I2C       | SDA      | 21        |
|             | I2C       | SCL      | 22        |
| **SSD1306** | SPI       | MOSI (D1)| 23        |
|             | SPI       | CLK (D0) | 18        |
|             | SPI       | DC       | 17        |
|             | SPI       | CS       | 5         |
|             | SPI       | RES      | 16        |
| **DS18B20** | OneWire   | Data     | 4         |

## Libraries Required
Ensure these libraries are installed in your Arduino IDE:
1.  **Adafruit SSD1306**
2.  **Adafruit GFX Library**
3.  **MAX30105** (by SparkFun)
4.  **MPU6050** (by ElectronicCats or similar)
5.  **DallasTemperature**
6.  **OneWire**

## Important Note
If you encounter an error regarding `spo2_algorithm.h`, ensure that:
1.  You have the **MAX30105** library installed.
2.  If the compiler cannot find the file, copy `spo2_algorithm.h` and `spo2_algorithm.cpp` from the library's `src` or `examples` folder into this sketch folder (`Integrated_Sensors`).
