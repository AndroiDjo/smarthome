#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

const uint8_t PIN_LED_R = D5;
const uint8_t PIN_LED_G = D6;
const uint8_t PIN_LED_B = D7;
const uint8_t PIN_PIR = D2;

const int jsonSize = 512;
const char* privateFile = "/private.json";
const char* configFile = "/config.json";
const char* clientName = "nodemcu_rgb_kitchen";
const int fadeSteps = 50;
const int fadeInterval = 10;
const uint8 maxBrightness = 255;

bool shouldSaveConfig = false;
bool saveSettingsCallback = false;
bool lightStateCallback = false;
bool power = true;
uint8 brightness = 255;
int sensorTimeout = 180000;
long roomSensorTime = millis();
int ledpwm = 1023;
int curpwm = 0;
int delayCounter = 0;
int delayLimit = 20;

struct Color {
  uint8 r;
  uint8 g;
  uint8 b;
};

const Color colorOff = {0,0,0};

struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

struct Ota {
  int port;
  char pass[20];
};

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
Ota ota;
Color color;
Color currentColor;

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void savePrivateParams() {
   StaticJsonDocument<256> doc;
   doc["mqtt_server"] = mqtt.host;
   doc["mqtt_login"] = mqtt.login;
   doc["mqtt_pass"] = mqtt.pass;
   doc["mqtt_port"] = mqtt.port;
   doc["ota_pass"] = ota.pass;
   doc["ota_port"] = ota.port;
   File destFile = LittleFS.open(privateFile, "w");
   bool srlzbool = (serializeJson(doc, destFile) == 0);
   if (srlzbool) {
    Serial.println("Failed to write private settings");
   } else {
     Serial.println("Private settings saved succesfully");
   }
   destFile.close();
}

void initPrivate() {
  File file = LittleFS.open(privateFile, "r");
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

void publishState() {
  lightStateCallback = false;
  StaticJsonDocument<512> doc;
  doc["state"] = power ? "ON" : "OFF";
  doc["brightness"] = brightness;
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("rgb/kitchen/state", buffer, n);
}

void fade(Color from, Color to) {
  if ((abs(to.r - from.r) + abs(to.g - from.g) + abs(to.b - from.b)) <= 6) return;
  currentColor = from;
  int curstep = 0;
  Color delta;
  delta.r = (to.r - from.r) / fadeSteps;
  delta.g = (to.g - from.g) / fadeSteps;
  delta.b = (to.b - from.b) / fadeSteps;
  while (curstep < fadeSteps) {
    currentColor.r += delta.r;
    currentColor.g += delta.g;
    currentColor.b += delta.b;
    analogWrite(PIN_LED_R, currentColor.r);
    analogWrite(PIN_LED_G, currentColor.g);
    analogWrite(PIN_LED_B, currentColor.b);
    curstep++;
    delay(fadeInterval);
  }
}

void setLed() {
  if (power) {
      float k = (float)brightness/maxBrightness;
      Color tmpColor = {color.r * k, color.g * k, color.b * k};
      fade(currentColor, tmpColor);
  } else
  {
    fade(currentColor, colorOff);
  }

  lightStateCallback = true;
  saveSettingsCallback = true;
}

void parseRequest(const JsonDocument& doc) {
  if (doc.containsKey("state")) {
    power = (doc["state"] == "ON");
    if (power) {
      roomSensorTime = millis();
    } else {
      roomSensorTime = millis() - sensorTimeout;
    }
    setLed();
  }

  if (doc.containsKey("brightness")) {
    brightness = doc["brightness"];
    roomSensorTime = millis();
    setLed();
  }

  if (doc.containsKey("color")) {
    color.r = doc["color"]["r"];
    color.g = doc["color"]["g"];
    color.b = doc["color"]["b"];
    roomSensorTime = millis();
    setLed();
  }

  if (doc.containsKey("sensor_timeout")) {
    sensorTimeout = doc["sensor_timeout"];
  }
  
}

void loadSettings() {
   File file = LittleFS.open(configFile, "r");
   StaticJsonDocument<512> doc;
   if (file) {
     DeserializationError error = deserializeJson(doc, file);
     if (error) {
      file.close();
     }
     file.close();
     parseRequest(doc);
   }
}

void saveSettings() {
   saveSettingsCallback = false;
   StaticJsonDocument<256> doc;
   doc["state"] = power ? "ON" : "OFF";
   doc["brightness"] = brightness;
   doc["color"]["r"] = color.r;
   doc["color"]["g"] = color.g;
   doc["color"]["b"] = color.b;
   doc["sensor_timeout"] = sensorTimeout;
   File destFile = LittleFS.open(configFile, "w");
   if (serializeJson(doc, destFile) == 0) {
    destFile.close();
   }
   destFile.close();
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {
    parseRequest(doc);
  }
}

void reconnectMqtt() {
  while (!client.connected()) {
    yield();
    delay(100);
    if (client.connect(clientName, mqtt.login, mqtt.pass)) {
      client.subscribe("all/modules");
      client.subscribe("all/light");
      client.subscribe("rgb/kitchen");
    } else {
      delay(5000);
    }
  }
}

void checkSensors() {
  if (digitalRead(PIN_PIR) == HIGH) {
    roomSensorTime = millis();
  }
  long deltaTime = millis() - roomSensorTime;
  if (deltaTime < sensorTimeout) {
    if (!power) {
      power = true;
      setLed();
    }
  } else {
    if (power) {
      power = false;
      setLed();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_PIR, INPUT_PULLUP);

  Serial.println("PIN set complete");

  if (!LittleFS.begin()) {
    return;
  }
  Serial.println("littleFS initialized");
  loadSettings();
  Serial.println("settngs loaded");

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
  // Подключение к роутеру. Если не удалось подключиться - перезагружаемся.
  wifiManager.setConfigPortalTimeout(60);
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
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
    reconnectMqtt();
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

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) reconnectMqtt();
  client.loop();
  if (saveSettingsCallback) saveSettings();
  checkSensors();
  if (lightStateCallback) publishState();
  delayLoop();
}