#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля
const int jsonSize = 1024; // размер буфера для парсинга json

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

WiFiClient espClient;
PubSubClient client(espClient);
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

void saveJsonParams(JsonObject& json) {
   bool existconfig = false;
   DynamicJsonDocument doc(jsonSize);
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

// обработчик MQTT команд
void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("Failed to dsrlz mqtt message");
  }

  //parseRequest(doc);
  JsonObject jobj = doc.as<JsonObject>();
  saveJsonParams(jobj);
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("nodemcu_bathroom", mqtt.login, mqtt.pass)) {
      Serial.println("nodemcu_bathroom connected");
      client.subscribe("all/modules");
      client.subscribe("light/bathroom");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
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
  //loadJsonParams();
}

void loop() {
    ArduinoOTA.handle();
    delay(50);
}