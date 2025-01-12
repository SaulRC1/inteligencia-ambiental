#include <Arduino.h>
#include <Wire.h>
#include <HPMA115_Compact.h>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"
#include <SoftwareSerial.h>

#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);  // addr , freq , i2c group , resolution , rst
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst
#endif

//#define UART_TX 17
//#define UART_RX 2

DisplayUi ui(&display);

const int HPM_PIN = 32;

const int MUX_A_PIN = 0;
const int MUX_B_PIN = 23;
int UART_RX = 2;
int UART_TX = 17;

SoftwareSerial hpmSerial(UART_RX, UART_TX);
HPMA115_Compact hpm = HPMA115_Compact();

void setup() {
  Serial.begin(HPMA115_BAUD);
  hpmSerial.begin(HPMA115_BAUD);
  hpm.begin(&hpmSerial);
  display.init();
  display.setFont(ArialMT_Plain_10);
}

void loop() {
  int valorMUX_A = analogRead(MUX_A_PIN);
  Serial.println("Valor MUX_A: " + String(valorMUX_A));

  int valorMUX_B = analogRead(MUX_B_PIN);
  /*Serial.println("Valor MUX_B: " + String(valorMUX_B));

  //int valorSER_RXD_PIN = analogRead(SER_RXD_PIN);
  Serial.println("Valor SER_RXD: " + String(valorSER_RXD_PIN));

  //int valorSER_TXD_PIN = analogRead(SER_TXD_PIN);
  Serial.println("Valor SER_TXD: " + String(valorSER_TXD_PIN));
*/
  display.clear();
  display.drawString(0, 0, "Valor MUX_A: " + String(valorMUX_A));
  display.drawString(0, 10, "Valor MUX_B: " + String(valorMUX_B));
 // display.drawString(0, 20, "Valor SER_RXD: " + String(valorSER_RXD_PIN));
  //display.drawString(0, 30, "Valor SER_TXD: " + String(valorSER_TXD_PIN));
  display.display();

  if (hpm.isNewDataAvailable()) 
  {
    Serial.print("AQI: ");
    Serial.print(hpm.getAQI());
    Serial.print("  PM 2.5 = ");
    Serial.print(hpm.getPM25());
    Serial.println();
  }

  delay(2000);
}
