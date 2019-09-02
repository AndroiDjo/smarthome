#include <Arduino.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
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
int rawSize = 0;
int rawIndex = 0;
long splitTimer = 0;
uint16_t* cmdraw = NULL;

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

void splitCommand(const JsonDocument& doc) {
  if (doc.containsKey("rawsize")) {
    splitTimer = millis();
    rawSize = doc["rawsize"];
    rawIndex = 0;
    if (cmdraw) {
      delete [] cmdraw;
      cmdraw = NULL;
    }
    cmdraw = new uint16_t[rawSize];
  }
  if (cmdraw) {
    int arraySize = doc["split"].size();
    for (int i = 0; i< arraySize; i++) {
      cmdraw[rawIndex] = doc["split"][i];
      rawIndex++;
    }
    if (doc.containsKey("rawend") || millis() - splitTimer > 1000) {
      irsend.sendRaw(cmdraw, rawSize, 38);
      delete [] cmdraw;
      cmdraw = NULL;
    }
  }
}

void parseRequest(const JsonDocument& doc) {
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

 if (doc.containsKey("split")) {
    splitCommand(doc);
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
    Serial.println("Failed to write to config file");
   }
   destFile.close();
}

// загрузка json параметров из SPIFFS
void loadSettings() {
  Serial.println("loadSettings");
   File file = SPIFFS.open(configFile, "r");
   StaticJsonDocument<256> doc;
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
  StaticJsonDocument<512> doc;
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
      client.subscribe("all/light");
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
  Serial.begin(115200);

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
