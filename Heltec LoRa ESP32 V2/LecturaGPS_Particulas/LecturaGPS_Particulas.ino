#include <HPMA115_Compact.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"
#include <TinyGPSMinus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// Configuración de pines del multiplexor
int muxA = 0;   // Pin de control A
int muxB = 23;  // Pin de control B

// Pines para activación de sensores
int on_gps = 33;   // Activación GPS
int on_aire = 32;  // Activación sensor de partículas
int ledTest = 25;  // LED de estado

// OLED Configuration
#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);  // addr , freq , i2c group , resolution , rst
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst
#endif
DisplayUi ui(&display);

const char* ssid = "MOVISTAR-WIFI6-6D30";
const char* password = "yRnkMmVsMRquU7mVsm3s";
const char* serverUrl = "http://212.227.87.151:8080/save-particle-data";
const char* serverUrlStatus = "http://212.227.87.151:8080/set-esp32-status";

// Configuración de la zona horaria y servidor NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // Ajuste para tu zona horaria (GMT+1 = 3600s)
const int daylightOffset_sec = 3600;  // Ajuste para horario de verano

WiFiClient client;

// Sensores y datos
HPMA115_Compact hpm = HPMA115_Compact();
TinyGPSMinus gps;  // Inicialización de TinyGPSMinus

// UART
HardwareSerial sensorSerial(2);  // UART2 compartida para GPS y partículas

// Variables de datos
int pm10 = 0;
int pm25 = 0;
char lat[9];
char lon[10];
float latitudeValue = 37.260330;
float longitudeValue = -6.946500;
char latDirection;
char lonDirection;

void setup() {
  Serial.begin(115200);

  // Configuración UART compartida
  sensorSerial.begin(9600, SERIAL_8N1, 2, 17);  // RX=2, TX=17

  // Configuración del OLED
  display.init();
  display.clear();
  display.display();
  display.setContrast(255);

  // Configuración de pines
  pinMode(on_gps, OUTPUT);
  pinMode(on_aire, OUTPUT);
  pinMode(muxA, OUTPUT);
  pinMode(muxB, OUTPUT);
  pinMode(ledTest, OUTPUT);

  // Inicialización de sensores
  digitalWrite(on_gps, HIGH);  // Desactiva GPS
  digitalWrite(on_aire, LOW);  // Desactiva sensor partículas
  digitalWrite(ledTest, LOW);  // Apaga LED
  delay(1000);

  // Inicialización pantalla
  display.clear();
  display.display();
  Serial.println("Sistema inicializado.");

  // Inicialización de sensores
  hpm.begin(&sensorSerial);

  //Parametros para la conexion WiFi
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  Serial.println("\nConnecting");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(500);
  }

  Serial.println("You're connected to the network");
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.getHostname());
  Serial.println();

  // Configurar y sincronizar tiempo NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Tiempo sincronizado con NTP");
  delay(2000);

  /*Serial.println("Seleccionando GPS...");
  selectGPS(); 
  readGPS();
  while (latitudeValue == 0.0000000 || longitudeValue == 0.0000000 || latitudeValue == 99.99 || longitudeValue == 99.99) {
    Serial.println("Esperando datos GPS válidos...");
    readGPS();  // Volver a leer GPS
  }*/

  Serial.println("Seleccionando sensor de partículas...");
  selectParticles();
}

void loop() {

  Serial.println("Leyendo partículas...");
  readParticles();

  while (pm10 == 0 || pm25 == 0) {
    Serial.println("Esperando datos de partículas válidos...");
    readParticles();  // Volver a leer GPS
  }

  Serial.println("Mostrando datos en OLED...");
  displayOled();

  if (WiFi.status() == WL_CONNECTED) {
    //Petición http para revisar status del ESP-32
    HTTPClient httpSTATUS;

    httpSTATUS.begin(serverUrlStatus);
    httpSTATUS.addHeader("Content-Type", "application/json");
    String body = "{";
    body += "\"status\":\"CONNECTED\"";
    body += "}";

    //Serial.println("JSON STATUS Enviado:");
    //Serial.println(body);
    //Serial.println("Sending HTTP POST request for STATUS...");
    int httpResponseCodeStatus = httpSTATUS.POST(body);

    // Verificar la respuesta del servidor
    if (httpResponseCodeStatus > 0) {
      String responseStatus = httpSTATUS.getString();
      //Serial.println("Respuesta del servidor: " + responseStatus);
    } else {
      //Serial.println("Error en la petición: " + String(httpResponseCodeStatus));
    }
    httpSTATUS.end();
    
  } else {
    Serial.println("WiFi Disconnected");
  }

  static unsigned long lastSensorDataMillis = 0;
  if (WiFi.status() == WL_CONNECTED && millis() - lastSensorDataMillis >= 30000) {

    lastSensorDataMillis = millis();

    //Petición HTTP para enviar los datos de los sensores
    HTTPClient http;

    // Configurar la URL del servidor
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Crear el cuerpo JSON
    String timestamp = getCurrentTimestamp();  // Obtener hora actual
    String jsonBody = "{";
    jsonBody += "\"measurementTimestamp\":\"" + String(timestamp) + "\",";
    jsonBody += "\"pm10\":" + String(pm10) + ",";
    jsonBody += "\"pm25\":" + String(pm25) + ",";
    jsonBody += "\"latitude\":\"" + String(latitudeValue, 6) + "\",";
    jsonBody += "\"longitude\":\"" + String(longitudeValue, 6) + "\"";
    jsonBody += "}";

    Serial.println("JSON Enviado:");
    Serial.println(jsonBody);

    // Enviar solicitud POST
    Serial.println("Sending HTTP POST request...");
    int httpResponseCode = http.POST(jsonBody);

    // Verificar respuesta
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();    // Finalizar conexión
  } else {
    //Serial.println("WiFi Disconnected");
  }
  delay(1000);  // Esperar 1 segundos antes de enviar nuevamente
}

void selectGPS() {
  // Configuración del multiplexor para GPS (00)
  digitalWrite(muxA, LOW);
  digitalWrite(muxB, LOW);
  digitalWrite(on_gps, LOW);   // Activa GPS
  digitalWrite(on_aire, LOW);  // Desactiva sensor partículas
  delay(2000);                 // Esperar a que el GPS esté listo
}

void readGPS() {

  // Activar el GPS
  digitalWrite(on_gps, LOW);  // Encender GPS
  Serial.println("Leyendo GPS...");

  // Leer datos del GPS
  while (sensorSerial.available()) {
    char c = sensorSerial.read();
    gps.encode(c);  // Decodificar los datos recibidos
  }

  strcpy(lat, gps.get_latitude());
  strcpy(lon, gps.get_longitude());

  latDirection = lat[strlen(lat) - 1];  // 'N' o 'S'
  lonDirection = lon[strlen(lon) - 1];  // 'E' o 'W'

  // Crear nuevas cadenas eliminando el carácter de dirección
  char latitudeWithoutDir[16];
  char longitudeWithoutDir[16];

  strncpy(latitudeWithoutDir, lat, strlen(lat) - 1);
  latitudeWithoutDir[strlen(lat) - 1] = '\0';

  strncpy(longitudeWithoutDir, lon, strlen(lon) - 1);
  longitudeWithoutDir[strlen(lon) - 1] = '\0';

  latitudeValue = convertToDecimal(latitudeWithoutDir, latDirection);
  longitudeValue = convertToDecimal(longitudeWithoutDir, lonDirection);

  Serial.print("Latitud sin convertir: ");
  Serial.println(lat);
  Serial.print("Longitud sin convertir: ");
  Serial.println(lon);

  Serial.print("Latitud (decimal): ");
  Serial.println(latitudeValue, 6);
  Serial.print("Longitud (decimal): ");
  Serial.println(longitudeValue, 6);

  snprintf(lat, sizeof(lat), "%.6f", latitudeValue);
  snprintf(lon, sizeof(lon), "%.6f", longitudeValue);
}

void selectParticles() {
  // Configuración del multiplexor para el sensor de partículas (01)
  digitalWrite(muxA, HIGH);
  digitalWrite(muxB, LOW);
  digitalWrite(on_gps, HIGH);   // Desactiva GPS
  digitalWrite(on_aire, HIGH);  // Activa sensor partículas
  delay(6000);
}

void readParticles() {

  Serial.println("Iniciando comunicación con el sensor de partículas...");
  hpm.stopAutoSend();  // Detener autoenvío (si estaba activo)
  delay(50);

  hpm.readParticleMeasurementResults();
  delay(50);
  hpm.isNewDataAvailable();

  pm10 = hpm.getPM10();
  pm25 = hpm.getPM25();

  // Verificar si los datos son válidos
  if (pm10 < 0 || pm25 < 0) {
    Serial.println("Advertencia: Datos inválidos recibidos del sensor de partículas.");
    pm10 = 0;
    pm25 = 0;
  }

  Serial.print("PM10: ");
  Serial.print(pm10);
  Serial.print(" µg/m3, PM2.5: ");
  Serial.print(pm25);
  Serial.println(" µg/m3");
}

void displayOled() {
  // Verificar que el objeto OLED esté inicializado
  if (!display.init()) {
    Serial.println("Error: El OLED no está inicializado.");
    return;
  }

  if (pm10 < 0 || pm25 < 0) {
    Serial.println("Error: Datos de partículas no válidos.");
    pm10 = 0;
    pm25 = 0;
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);

  //latitudeValue = atof(lat);
  //longitudeValue = atof(lon);

  // Información a mostrar
  String gpsInfo = "Lat: " + String(latitudeValue, 5) + " Lon: " + String(longitudeValue, 5);
  String particlesInfo = "PM10: " + String(pm10) + " PM2.5: " + String(pm25);
  Serial.println(gpsInfo);

  // Mostrar en la pantalla
  display.drawString(10, 5, gpsInfo);         // Datos GPS
  display.drawString(10, 30, particlesInfo);  // Datos de partículas
  display.display();
}

String getCurrentTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error: No se pudo obtener la hora actual.");
    return "";
  }

  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  // Mostrar fecha y hora
  Serial.print("Fecha y hora actual: ");
  Serial.println(String(timestamp));
  return String(timestamp);
}

float convertToDecimal(const char* rawCoordinate, char direction) {
  // Convertir a String para manipular más fácilmente
  String coordinateStr = String(rawCoordinate);

  // Separar grados y minutos
  int dotIndex = coordinateStr.indexOf('.');
  int degreesLength = (dotIndex > 2) ? dotIndex - 2 : dotIndex - 1;  // Verifica si son 2 o 3 dígitos de grados
  int degrees = coordinateStr.substring(0, degreesLength).toInt();   // Extraer grados
  float minutes = coordinateStr.substring(degreesLength).toFloat();  // Extraer minutos

  // Convertir a formato decimal
  float decimal = degrees + (minutes / 60.0);

  // Aplicar el signo según la dirección
  if (direction == 'S' || direction == 'W') {
    decimal *= -1;
  }

  return decimal;
}
