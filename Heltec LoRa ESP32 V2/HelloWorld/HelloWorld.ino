#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"

#ifdef WIRELESS_STICK_V3
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED); // addr , freq , i2c group , resolution , rst
#else
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
#endif

DisplayUi ui( &display );

void setup() {
  Serial.begin(115200);
  display.init();
  display.setFont(ArialMT_Plain_16);
}

void loop() {
  // put your main code here, to run repeatedly:
  display.clear();
  display.drawString(0, 0, "Hello World!");
  display.display();
}
