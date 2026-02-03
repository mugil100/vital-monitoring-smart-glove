void setup() {
  Serial.begin(115200);
  Serial.println("Reading RAW GPS NMEA Data...");

  // UART2 for GPS: RX=16, TX=17
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial.print(c);  // Print every raw character
  }
}