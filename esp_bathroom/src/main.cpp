#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11

const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля
const uint8_t LED_PIN = D6;
const uint8_t FAN_PIN = D7;
const uint8_t HALL_PIN = D4;
const uint8_t TEMP_PIN = D3;
const uint8_t MOTION_ROOM_PIN = D2;
const uint8_t MOTION_DOOR_PIN = D1;

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

bool shouldSaveConfig = false;
bool tempSensorCallback = false; // колбэк для публикации данных с датчика температуры
bool power = true;
bool powerlow = false;
bool fan = false;
bool manualMode = false;
int ledpwm = 1023;
int ledpwmLow = 100;
int curpwm = 0;
int delayCounter = 0;
int delayLimit = 20;
uint32_t delayMS;
long roomSensorTime = millis();
long doorSensorTime = millis();
int shortInterval = 10000;
int longInterval = 120000;
int fadeSteps = 50;
int fadeInterval = 10;
int fanDelay = 3000;
long fanLastDisableTime = millis();

WiFiClient espClient;
PubSubClient client(espClient);
DHT_Unified dht(TEMP_PIN, DHTTYPE);
Mqtt mqtt;
Ota ota;

void savePrivateParams() {
   bool existconfig = false;
   StaticJsonDocument<256> doc;
   doc["mqtt_server"] = mqtt.host;
   doc["mqtt_login"] = mqtt.login;
   doc["mqtt_pass"] = mqtt.pass;
   doc["mqtt_port"] = mqtt.port;
   doc["ota_pass"] = ota.pass;
   doc["ota_port"] = ota.port;
   File destFile = SPIFFS.open(privateFile, "w");
   bool srlzbool = (serializeJson(doc, destFile) == 0);
   if (srlzbool) {
    Serial.println("Failed to write private settings");
   } else {
     Serial.println("Private settings saved succesfully");
   }
   destFile.close();
}

void saveConfigCallback () {
  Serial.println("should save config");
  shouldSaveConfig = true;
}

// чтение учетных данных из SPIFFS
void initPrivate() {
  File file = SPIFFS.open(privateFile, "r");
  StaticJsonDocument<256> doc;
  if (!file) {
    Serial.println("Failed to open private file");
  } 
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to dsrlz private file");
  }
  file.close();
  strlcpy(mqtt.host, doc["mqtt_server"], sizeof(mqtt.host));
  strlcpy(mqtt.login, doc["mqtt_login"], sizeof(mqtt.login));
  strlcpy(mqtt.pass, doc["mqtt_pass"], sizeof(mqtt.pass));
  mqtt.port = doc["mqtt_port"];
  strlcpy(ota.pass, doc["ota_pass"], sizeof(ota.pass));
  ota.port = doc["ota_port"];
  Serial.println("Private settings loaded successfully");
}

void fade(int from, int to) {
  if (from == to || abs(to - from) <= 5) return;
  curpwm = from;
  int curstep = 0;
  int delta = (to - from) / fadeSteps;
  while (curstep < fadeSteps) {
    curpwm += delta;
    analogWrite(LED_PIN, curpwm);
    curstep++;
    delay(fadeInterval);
  }
}

void switchLed() {
  if (power) {
    if (powerlow) {
      fade(curpwm, ledpwmLow);
    } else {
      fade(curpwm, ledpwm);
    }
  } else
  {
    fade(curpwm, 0);
  }
}

void switchFan() {
  if (fan) {
    digitalWrite(FAN_PIN, 1);
  } else {
    digitalWrite(FAN_PIN, 0);
  }
}

void checkLowLight(long deltaTime, int timeInterval) {
  if (power && !fan && 
     (timeInterval - deltaTime) < (timeInterval / 10) &&
     ledpwm > ledpwmLow)   {
    powerlow = true;
  } else {
    powerlow = false;
  }
}

void checkSensors() {
  if (manualMode) return;
  if (digitalRead(MOTION_ROOM_PIN) == HIGH) {
    roomSensorTime = millis();
  }
  long deltaTime = millis() - roomSensorTime;
  power = (deltaTime < longInterval);
  checkLowLight(deltaTime, longInterval);

  if (digitalRead(MOTION_DOOR_PIN) == HIGH) {
    doorSensorTime = millis();
  }
  if (!power) {
    deltaTime = millis() - doorSensorTime;
    power = (deltaTime < shortInterval);
    checkLowLight(deltaTime, shortInterval);
  }

  if (digitalRead(HALL_PIN) == LOW) {
    if (millis() - fanLastDisableTime > fanDelay) {
      fan = true;
      doorSensorTime = millis();
    } else {
      fan = false;
    }
  } else {
    fan = false;
    fanLastDisableTime = millis();
  }
}

void getTemp() {
  tempSensorCallback = false;
  StaticJsonDocument<256> doc;
  sensors_event_t event;
  const char* clientid = "bathroomtemp";
  doc[clientid]["kind"] = "temperature";
  doc[clientid]["location"] = "bathroom";
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

// обработка json команд
void parseRequest(const JsonDocument& doc) {
  if (doc.containsKey("ledpwm")) {
    ledpwm = doc["ledpwm"];
  }

  if (doc.containsKey("fan")) {
    fan = doc["fan"];
  }

  if (doc.containsKey("manualmode")) {
    manualMode = doc["manualmode"];
  }

  if (doc.containsKey("gettemp") || doc.containsKey("getall")) {
    tempSensorCallback = true;
  }

  if (doc.containsKey("shorti")) {
    shortInterval = doc["shorti"];
  }

  if (doc.containsKey("longi")) {
    longInterval = doc["longi"];
  }

  if (doc.containsKey("power")) {
    power = doc["power"];
  }
}

void saveJsonParams(const JsonObject& json) {
   bool existconfig = false;
   StaticJsonDocument<512> doc;
   File file = SPIFFS.open(configFile, "r");
   if (file) {
      DeserializationError error = deserializeJson(doc, file);
      if (error) {
        file.close();
        Serial.println("Failed to dsrlz config file");
      }
      file.close();
      for (JsonPair kv : json) {
        doc[kv.key()] = kv.value();
      }
      existconfig = true;
   }

   File destFile = SPIFFS.open(configFile, "w");
   bool srlzbool;
   if (existconfig) {
     srlzbool = (serializeJson(doc, destFile) == 0);
   } else {
     Serial.println("config file not found, create new");
     srlzbool = (serializeJson(json, destFile) == 0);
   }
   if (srlzbool) {
    destFile.close();
    Serial.println("Failed to write to config file");
   }
   destFile.close();
}

// загрузка json параметров из SPIFFS
void loadJsonParams() {
   File file = SPIFFS.open(configFile, "r");
   StaticJsonDocument<512> doc;
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
  } else {
    serializeJsonPretty(doc, Serial);
  }

  parseRequest(doc);
  JsonObject jobj = doc.as<JsonObject>();
  saveJsonParams(jobj);
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    yield();
    delay(100);
    Serial.print("Attempting MQTT connection...");
    if (client.connect("nodemcu_bathroom", mqtt.login, mqtt.pass)) {
      Serial.println("nodemcu_bathroom connected");
      client.subscribe("all/modules");
      client.subscribe("all/light");
      client.subscribe("light/bathroom");
      client.subscribe("sensor/req");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
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
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HALL_PIN, INPUT);
  pinMode(MOTION_ROOM_PIN, INPUT);
  pinMode(MOTION_DOOR_PIN, INPUT);
  dht.begin();
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  WiFiManagerParameter custom_mqtt_title("<br>MQTT configuration:");
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 20);
  WiFiManagerParameter custom_mqtt_username("user", "mqtt user", "", 20);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt_password", "", 20);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "", 10);
  WiFiManagerParameter custom_ota_title("<br>OTA configuration:");
  WiFiManagerParameter custom_ota_password("otapassword", "ota_password", "", 20);
  WiFiManagerParameter custom_ota_port("otaport", "ota port", "", 10);

  wifiManager.addParameter(&custom_mqtt_title);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_ota_title);
  wifiManager.addParameter(&custom_ota_password);
  wifiManager.addParameter(&custom_ota_port);

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(60);
  if (!wifiManager.autoConnect("AutoConnectAP", "password123")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  delay(1000);  
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
  }

  if (shouldSaveConfig) {
    strcpy(mqtt.host, custom_mqtt_server.getValue());
    strcpy(mqtt.login, custom_mqtt_username.getValue());
    strcpy(mqtt.pass, custom_mqtt_password.getValue());
    mqtt.port = String(custom_mqtt_port.getValue()).toInt();
    strcpy(ota.pass, custom_ota_password.getValue());
    ota.port = String(custom_ota_port.getValue()).toInt();
    
    savePrivateParams();
    delay(100);
  } else {
    initPrivate();
  }

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
  loadJsonParams();
}

void loop() {
    ArduinoOTA.handle();
    if (!client.connected())
    reconnect();
    client.loop();
    if (tempSensorCallback) getTemp();
    checkSensors();
    switchLed();
    switchFan();
    delayLoop();
}