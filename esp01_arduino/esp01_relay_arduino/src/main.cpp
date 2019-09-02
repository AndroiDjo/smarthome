#include <Arduino.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

const int jsonSize = 512; // размер буфера для парсинга json
bool power = true; // переменная отвечает за отключение/включение светильника
bool clapSensor = true; // переключение реле по хлопку
bool saveSettingsCallback = false; // колбэк для сохранения настроек
const char* clientName = "esp01_light_hall"; // название MQTT клиента
int relayPin = 0;

// учетные данные Mosquitto
struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля

void initPrivate() {
  File file = SPIFFS.open(privateFile, "r");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    file.close();
  }
  file.close();
  strlcpy(mqtt.host, doc["mqtt_server"], sizeof(mqtt.host));
  strlcpy(mqtt.login, doc["mqtt_login"], sizeof(mqtt.login));
  strlcpy(mqtt.pass, doc["mqtt_pass"], sizeof(mqtt.pass));
  mqtt.port = doc["mqtt_port"];
}

void setPower(bool pwr) {
  power = pwr;
  if (power) {
    digitalWrite(relayPin, LOW); // реле включено
  } else {
    digitalWrite(relayPin, HIGH); // реле выключено
  }
  saveSettingsCallback = true;
}

void parseRequest(const JsonDocument& doc) {
  if (doc.containsKey("power")) {
    setPower(doc["power"]);
  }

  if (doc.containsKey("clap")) {
    saveSettingsCallback = true;
    clapSensor = doc["clap"];
  }

  if (doc.containsKey("hallclap") && clapSensor) {
    setPower(!power);
  }

  // показания датчиков пробросим на сервер
  if (doc.containsKey("hallclap") || doc.containsKey("halltemp")) {
    char buffer[jsonSize];
    size_t n = serializeJson(doc, buffer);
    client.publish("sensor/resp", buffer, n);
  }

  // проброс команд на arduino
  if (doc.containsKey("gettemp") || doc.containsKey("getall") || doc.containsKey("split")) {
    serializeJson(doc, Serial);
  }
}

void serialLoop() {
  if (Serial.available()) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, Serial);
     if (!error) {
       parseRequest(doc);
     }
  }
}

// сохранение параметров в формате json в SPIFFS память
void saveSettings() {
   saveSettingsCallback = false;
   StaticJsonDocument<256> doc;
   doc["power"] = power;
   doc["clap"] = clapSensor;
   File destFile = SPIFFS.open(configFile, "w");
   if (serializeJson(doc, destFile) == 0) {
    destFile.close();
   }
   destFile.close();
}

// загрузка json параметров из SPIFFS
void loadSettings() {
   File file = SPIFFS.open(configFile, "r");
   StaticJsonDocument<256> doc;
   if (file) {
     DeserializationError error = deserializeJson(doc, file);
     if (error) {
      file.close();
     }
     file.close();
     parseRequest(doc);
   }
}

// обработчик MQTT команд
void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {
    parseRequest(doc);
  }
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    yield();
    delay(100);
    if (client.connect(clientName, mqtt.login, mqtt.pass)) {
      client.subscribe("all/modules");
      client.subscribe("all/light");
      client.subscribe("light/hall");
      client.subscribe("sensor/req");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin,OUTPUT);
  delay(2000);

  if (!SPIFFS.begin()) {
    return;
  }
  loadSettings();

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  // Подключение к роутеру. Если не удалось подключиться - перезагружаемся.
  wifiManager.setConfigPortalTimeout(10);
  if (!wifiManager.autoConnect("AP")) {
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  initPrivate();

  // инициализация MQTT брокера
  client.setServer(mqtt.host, mqtt.port);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
}

void loop() {
  if (!client.connected())
    reconnect();
  client.loop();
  serialLoop();
  if (saveSettingsCallback)
    saveSettings();
  yield();
  delay(1);
}
