#include <Arduino.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <PubSubClient.h>

#define DHTPIN 12     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

int soundSensor = 14;
int led = 2;
int clap = 0;
long detection_range_start = 0;
long detection_range = 0;
boolean status_lights = false;
const int jsonSize = 1024; // размер буфера для парсинга json
bool power = true; // переменная отвечает за отключение/включение светильника
bool clapSensor = true; // переключение реле по хлопку
int delayCounter = 0;
int delayLimit = 20;
bool tempSensorCallback = false; // колбэк для публикации данных с датчика температуры
bool saveSettingsCallback = false; // колбэк для сохранения настроек
const char* clientName = "nodemcu_lightcloud"; // название MQTT клиента
int relayPin = 4;
const uint16_t kIrLed = 5;  

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of data captured by IRrecvDumpV2.ino
uint16_t powerOff[229] = {3132, 3064,  3094, 4560,  576, 1720,  526, 580,  576, 1720,  524, 580,  580, 554,  578, 1718,  552, 1718,  524, 580,  556, 1742,  550, 554,  578, 556,  554, 1742,  526, 1744,  526, 1718,  552,
580,  552, 578,  554, 578,  554, 580,  554, 580,  554, 576,  554, 580,  576, 556,  554, 580,  578, 554,  554, 580,  552, 578,  578, 554,  554, 578,  552, 580,  578, 554,  556, 576,  556, 578,  554, 578,  554, 580,  554, 580,  552, 580,  556, 576,  552, 580,  558, 576,  552, 580,  552, 580,  554, 1718,  550, 1722,  548, 580,  552, 580,  552, 580,  556, 578,  552, 580,  552, 578,  554, 580,  554, 580,  552, 580,  554, 578,  554, 580,
 554, 578,  554, 578,  552, 580,  554, 580,  550, 1722,  546, 582,  550, 582,  554, 580,  560, 572,  552, 580,  554, 578,  576, 556,  554, 578,  554, 580,  578, 554,  554, 578,  558, 576,  552, 580,  554, 578,  552, 580,  552, 580,  578, 556,  554, 578,  554, 580,  556, 578,  554, 578,  552, 578,  554, 580,  554, 578,  556, 578,  552, 578,  554, 578,  582, 552,  552, 580,  556, 578,  554, 578,  552, 580,  554, 578,  556, 578,  552,
580,  554, 580,  552, 580,  552, 580,  554, 580,  550, 580,  552, 580,  552, 578,  554, 1720,  550, 580,  552, 1718,  550, 1722,  548, 1720,  546, 584,  550, 582,  552, 580,  552, 1720,  544, 1722,  526, 1746,  544};

uint16_t temp24[229] = {3042, 3162,  3064, 4598,  554, 1750,  526, 588,  556, 1746,  526, 586,  556, 584,  556, 1746,  526, 1750,  526, 586,  558, 1744,  526, 586,  558, 584,  556, 588,  556, 1746,  526, 1746,  526, 586,  556, 582,  556, 582,  558, 582,  556, 584,  556, 584,  558, 582,  558, 582,  556, 582,  556, 582,  556, 584,  556, 584,  556, 584,  556, 584,  556, 584,  556, 582,  556, 584,  558, 584,  556, 582,  558, 1746,  526, 586,  556, 584,  556, 584,  556, 584,  556, 584,  556, 584,  558, 582,  558, 1746,  526, 1750,  526, 588,  556, 584,  556, 582,  558, 584,  556, 582,  558, 582,  558, 582,  558, 582,  558, 582,  558, 584,  558, 582,
 556, 584,  558, 584,  556, 584,  556, 584,  556, 1746,  526, 586,  558, 580,  556, 580,  556, 580,  556, 582,  556, 584,  556, 582,  558, 584,  556, 582,  558, 584,  556, 582,  558, 582,  556, 584,  556, 584,  558, 582,  558, 582,  556, 584,  556, 584,  556, 584,  556, 584,  556, 582,  556, 584,  556, 584,  556, 584,  558, 582,  556, 584,  558, 582,  558, 584,  556, 584,  556, 582,  558, 582,  556, 582,  556, 582,  556, 584,  558,
584,  556, 582,  558, 582,  556, 584,  558, 584,  556, 584,  558, 582,  558, 584,  556, 582,  558, 584,  556, 1746,  526, 1750,  526, 1750,  526, 1750,  526, 1750,  526, 584,  558, 580,  556, 1744,  528, 1750,  526};

uint16_t temp25[229] = {3072, 3126,  3072, 4584,  554, 1744,  528, 582,  558, 1740,  528, 582,  558, 578,  558, 1740,  528, 1744,  528, 582,  558, 1742,  528, 580,  558, 576,  580, 1718,  528, 1744,  528, 1746,  528,
580,  558, 576,  558, 578,  558, 576,  558, 578,  558, 576,  558, 578,  584, 552,  558, 578,  558, 580,  578, 554,  560, 574,  560, 576,  558, 578,  558, 576,  558, 576,  582, 552,  560, 576,  560, 576,  560, 1714,  552, 582,  558, 578,  560, 576,  560, 576,  558, 578,  558, 578,  582, 554,  584, 1714,  528, 1722,  550, 580,  558, 578,  558, 578,  558, 576,  558, 578,  582, 552,  582, 552,  558, 578,  558, 578,  558, 578,  558, 576,  560, 576,  558, 578,  580, 554,  558, 576,  558, 1740,  528, 580,  560, 576,  582, 554,  582, 554,  558, 576,  560, 576,  558, 576,  558, 576,  584, 552,  582, 552,  560, 576,  558, 576,  560, 576,  582, 554,  560, 576,  582, 554,  558, 576,  558, 582,  554, 576,  558, 576,  582, 554,  560, 576,  560, 576,  582, 554,  558, 576,  582, 554,  560, 576,  558, 576,  558, 578,  582, 552,  584, 552,  582, 554,  560, 576,  582, 552,  560, 576,  582, 554,  582, 554,  560, 576,  582, 554,  580, 558,  580, 552,  582, 552,  584, 552,  560, 576,  582, 554,  584, 552,  558, 578,  582, 552,  584, 552,  582, 554,  582, 554,  558, 1718,  552, 580,  560};

uint16_t temp26[229] = {3076, 3148,  3076, 4588,  532, 1772,  504, 608,  530, 1746,  528, 608,  530, 606,  530, 1768,  504, 1746,  548, 586,  530, 1746,  528, 606,  548, 1724,  550, 584,  530, 1748,  526, 1770,  504, 608,  530, 608,  530, 606,  550, 588,  530, 608,  530, 606,  532, 608,  530, 608,  530, 608,  528, 608,  530, 608,  530, 608,  530, 606,  532, 606,  530, 608,  530, 608,  530, 608,  550, 586,  530, 608,  528, 1770,  504, 608,  530, 608,  530, 608,  530, 606,  530, 608,  530, 608,  530, 606,  530, 1746,  528, 1744,  530, 614,  530, 612,  530, 608,  530, 608,  528, 608,  530, 606,  530, 604,  532, 606,  530, 606,  530, 604,  532, 606,  530, 610,  530, 608,  530, 606,  530, 606,  530, 1748,  526, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 606,  530, 608,  530, 608,  528, 608,  532, 608,  530, 608,  532, 604,  530, 608,  530, 608,
 530, 606,  532, 606,  530, 606,  530, 610,  530, 608,  530, 608,  530, 606,  530, 608,  530, 608,  530, 606,  532, 608,  530, 608,  530, 606,  532, 606,  530, 606,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  530, 608,  528, 608,  530, 608,  530, 606,  530, 1748,  530, 608,  532, 606,  532, 606,  532, 608,  530, 1746,  528, 606,  532, 1742,  530, 606,  530, 608,  530};

// учетные данные Mosquitto
struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

// учетные данные OTA
struct Ota {
  int port;
  char pass[20];
};

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
Ota ota;
const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля

void initPrivate() {
  File file = SPIFFS.open(privateFile, "r");
  StaticJsonDocument<256> doc;
  if (!file) {
    Serial.println("Failed to open private file");
  } 
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    file.close();
    Serial.println("Failed to dsrlz private file");
  }
  file.close();
  strlcpy(mqtt.host, doc["mqtt_server"], sizeof(mqtt.host));
  strlcpy(mqtt.login, doc["mqtt_login"], sizeof(mqtt.login));
  strlcpy(mqtt.pass, doc["mqtt_pass"], sizeof(mqtt.pass));
  mqtt.port = doc["mqtt_port"];
  strlcpy(ota.pass, doc["ota_pass"], sizeof(ota.pass));
  ota.port = doc["ota_port"];
}

void setPower(bool pwr) {
  power = pwr;
  if (power) {
    digitalWrite(relayPin, HIGH); // реле включено
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(relayPin, LOW); // реле выключено
    digitalWrite(led, LOW);
  }
  saveSettingsCallback = true;
}

void parseRequest(DynamicJsonDocument& doc) {
  if (doc.containsKey("power")) {
    setPower(doc["power"]);
  }

  if (doc.containsKey("clap")) {
    saveSettingsCallback = true;
    clapSensor = doc["clap"];
  }

  if (doc.containsKey("gettemp") || doc.containsKey("getall")) {
    tempSensorCallback = true;
  }

  if (doc.containsKey("sploff")) {
    irsend.sendRaw(powerOff, 229, 38);
  } else if (doc.containsKey("spltemp24")) {
    irsend.sendRaw(temp24, 229, 38);
  } else if (doc.containsKey("spltemp25")) {
    irsend.sendRaw(temp25, 229, 38);
  } else if (doc.containsKey("spltemp26")) {
    irsend.sendRaw(temp26, 229, 38);
  }
}

// сохранение параметров в формате json в SPIFFS память
void saveSettings() {
   saveSettingsCallback = false;
   DynamicJsonDocument doc(jsonSize);
   doc["power"] = power;
   doc["clap"] = clapSensor;
   File destFile = SPIFFS.open(configFile, "w");
   if (serializeJson(doc, destFile) == 0) {
    destFile.close();
    Serial.println("Failed to write to config file");
   }
   destFile.close();
}

// загрузка json параметров из SPIFFS
void loadSettings() {
  Serial.println("loadSettings");
   File file = SPIFFS.open(configFile, "r");
   DynamicJsonDocument doc(jsonSize);
   if (file) {
     DeserializationError error = deserializeJson(doc, file);
     if (error) {
      file.close();
      Serial.println("loadJsonParams: Failed to dsrlz config file");
     }
     file.close();
     serializeJsonPretty(doc, Serial);
     parseRequest(doc);
   } else {
     Serial.println("loadJsonParams: config file not found");
   }
}

// обработчик MQTT команд
void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("Failed to dsrlz mqtt message");
  }
  parseRequest(doc);
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    yield();
    delay(100);
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientName, mqtt.login, mqtt.pass)) {
      Serial.print(clientName);
      Serial.println(" connected");
      client.subscribe("all/modules");
      client.subscribe("lightcloud/childroom");
      client.subscribe("sensor/req");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void getTemp() {
  tempSensorCallback = false;
  StaticJsonDocument<256> doc;
  sensors_event_t event;
  const char* clientid = "cloudtemp";
  doc[clientid]["kind"] = "temperature";
  doc[clientid]["location"] = "childroom";
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    doc[clientid]["temperature"] = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    doc[clientid]["humidity"] = event.relative_humidity;
  }
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("sensor/resp", buffer, n);
  delay(1000);
}

void clapLoop() {
  int status_sensor = digitalRead(soundSensor);
  if (status_sensor == 1)
  {
    if (clap == 0)
    {
      detection_range_start = detection_range = millis();
      clap++;
    }
    else if (clap > 0 && millis()-detection_range >= 50)
    {
      detection_range = millis();
      clap++;
    }
  }
  if (millis()-detection_range_start >= 400)
  {
    if (clap == 2)
    {
      StaticJsonDocument<256> doc;
      const char* clientid = "cloudclap";
      doc[clientid]["kind"] = "sound";
      doc[clientid]["location"] = "childroom";
      char buffer[512];
      size_t n = serializeJson(doc, buffer);
      client.publish("sensor/resp", buffer, n);

      if (!status_lights)
      {
        status_lights = true;
        setPower(true);
      }
      else if (status_lights)
      {
        status_lights = false;
        setPower(false);
      }
    }
    clap = 0;
  }
}

void delayLoop() {
  delayCounter++;
  if (delayCounter > delayLimit) {
    delayCounter = 0;
    yield();
    delay(1);
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  delay(2000);
  Serial.println("Mounting FS...");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  
  pinMode(soundSensor, INPUT);
  pinMode(led,OUTPUT);
  pinMode(relayPin,OUTPUT);
  irsend.begin();
  dht.begin();
  loadSettings();

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  // Подключение к роутеру. Если не удалось подключиться - перезагружаемся.
  wifiManager.setConfigPortalTimeout(10);
  if (!wifiManager.autoConnect("AutoConnectAP", "password123")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  initPrivate();

  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  // инициализация обновления по воздуху
  ArduinoOTA.setPort(ota.port);
  ArduinoOTA.setPassword(ota.pass);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }

    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  // инициализация MQTT брокера
  client.setServer(mqtt.host, mqtt.port);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected())
    reconnect();
  client.loop();
  if (saveSettingsCallback)
    saveSettings();
  if (clapSensor)
    clapLoop();
  if (tempSensorCallback)
    getTemp();
  delayLoop();
}
