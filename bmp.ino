#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ---- WLAN ----
#define WIFI_SSID     "Telematica WLAN 6D4C"
#define WIFI_PASSWORD "xxxxx"

// ---- MQTT ----
#define MQTT_SERVER   "192.168.0.171"
#define MQTT_PORT     1883
#define MQTT_CLIENTID "esp8266-bme280"
#define MQTT_USER     "mqtt"
#define MQTT_PASS     "xxxxxx"
#define MQTT_TOPIC_TEMP     "home/bme280/aussen/temperature"
#define MQTT_TOPIC_HUM      "home/bme280/aussen/humidity"
#define MQTT_TOPIC_PRESS    "home/bme280/aussen/pressure"
#define MQTT_TOPIC_INTERVAL "home/bme280/aussen/interval" // Intervall-Topic

// ---- BME280 ----
#define SDA_PIN D2 // GPIO4
#define SCL_PIN D1 // GPIO5
#define BME_ADDR 0x76

Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient mqtt(espClient);

uint16_t intervalMinutes = 5; // Standardwert
bool intervalUpdated = false;



// ---- WLAN verbinden ----
void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WIFI] Verbinde mit ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Verbunden, IP: " + WiFi.localIP().toString());
}


// ---- MQTT verbinden ----
void mqttConnect() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Verbinde ... ");
    if (mqtt.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS)) {
      Serial.println("OK");
      mqtt.subscribe(MQTT_TOPIC_INTERVAL);
    } else {
      Serial.print("Fehler, state=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED aus

  Serial.begin(115200);
  delay(3000);
  
  Serial.printf("[INFO] Intervall: %u Minuten\n", intervalMinutes);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(BME_ADDR)) {
    Serial.println("[BME280] Sensor nicht gefunden! Adresse oder Verkabelung prüfen.");
    delay(2000);
    return;
  }

  wifiConnect();
  mqttConnect();

  // Messwerte lesen
  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float press = bme.readPressure() / 100.0F; // hPa

  Serial.printf("[BME280] T=%.1f °C, H=%.1f %%, P=%.1f hPa\n", temp, hum, press);

  char buf[16];
  dtostrf(temp, 4, 1, buf);
  mqtt.publish(MQTT_TOPIC_TEMP, buf, true);

  dtostrf(hum, 4, 1, buf);
  mqtt.publish(MQTT_TOPIC_HUM, buf, true);

  dtostrf(press, 6, 1, buf);
  mqtt.publish(MQTT_TOPIC_PRESS, buf, true);

  // Warten für evtl. Intervalländerung
  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    mqtt.loop();
    delay(10);
  }

  mqtt.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.printf("[SLEEP] Schlafe jetzt %u Minuten...\n", intervalMinutes);
  ESP.deepSleep((uint64_t)intervalMinutes * 60ULL * 1000000ULL); // µs
}

void loop() {
  // leer
}
