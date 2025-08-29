#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// ---- WLAN ----
#define WIFI_SSID     "TP-Link_5DD5"
#define WIFI_PASSWORD "xxxxx"

// ---- MQTT ----
#define MQTT_SERVER   "192.168.0.171"
#define MQTT_PORT     1883
#define MQTT_CLIENTID "esp8266-dht22"
#define MQTT_USER     "mqtt"
#define MQTT_PASS     "xxxx"
#define MQTT_TOPIC_TEMP "home/dht22/garage/temperature"
#define MQTT_TOPIC_HUM  "home/dht22/garage/humidity"

// ---- DHT ----
#define DHT_PIN 16   // GPIO16 = D0 am NodeMCU
DHTesp dht;

// ---- Objekte ----
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---- WLAN verbinden ----
void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WIFI] Verbinde mit ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
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
    } else {
      Serial.print("Fehler, state=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // LED an (bei ESP8266 ist LOW = an)
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[ESP8266+DHT22+MQTT] Start");

  // DHT
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // WLAN & MQTT
  wifiConnect();
  mqttConnect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }
  if (!mqtt.connected()) {
    mqttConnect();
  }
  mqtt.loop();

  // Alle 10 Sekunden Werte senden
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 10000) {
    lastSend = millis();

    float temp = dht.getTemperature();
    float hum  = dht.getHumidity();

    Serial.printf("[DHT] T=%.1f °C, H=%.1f %%\n", temp, hum);

    // MQTT Publish
    char buf[16];
    dtostrf(temp, 4, 1, buf);
    mqtt.publish(MQTT_TOPIC_TEMP, buf, true);

    dtostrf(hum, 4, 1, buf);
    mqtt.publish(MQTT_TOPIC_HUM, buf, true);

    Serial.println("[MQTT] Werte gesendet.");
  }
}
