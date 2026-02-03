// OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Define pins
#define I2C_A_SDA 21 // GPIO8
#define I2C_A_SCL 22 // GPIO9


// OLED parameters
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // Change if required
#define ROTATION 0           // Rotates text on OLED 1=90 degrees, 2=180 degrees

// Define display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
   Wire.begin(I2C_A_SDA, I2C_A_SCL);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Display1 SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  
// Show splash screen
  display.display();

  delay(2000); // Pause for 2 seconds

  // Display settings
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.setRotation(ROTATION);      // Set screen rotation

  display.print("Display1 ");


  display.display();


}


void loop() {
  // put your main code here, to run repeatedly:

}
