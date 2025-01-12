const int ledPin = 25;
const int frequency = 5000;
const int resolution = 8;

void setup() {
  Serial.begin(115200);
  ledcAttach(ledPin, frequency, resolution);
}

void loop() {
  // Incrementar el ciclo de trabajo gradualmente para aumentar la intensidad del LED
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(ledPin, dutyCycle);
    delay(10);
  }

  // Disminuir el ciclo de trabajo gradualmente para reducir la intensidad del LED
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(ledPin, dutyCycle);
    delay(10);
  }
}
