#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

const int RELAY_PIN = 0;
const int jsonSize = 512;
const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля
const char* clientName = "esp01_backlight_hall"; // название MQTT клиента

struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

bool saveSettingsCallback = false;
bool lightStateCallback = false;
bool shouldSaveConfig = false;
bool power = true;

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void savePrivateParams() {
   StaticJsonDocument<256> doc;
   doc["mqtt_server"] = mqtt.host;
   doc["mqtt_login"] = mqtt.login;
   doc["mqtt_pass"] = mqtt.pass;
   doc["mqtt_port"] = mqtt.port;
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
    digitalWrite(RELAY_PIN, LOW); // реле включено
  } else {
    digitalWrite(RELAY_PIN, HIGH); // реле выключено
  }
  lightStateCallback = true;
  saveSettingsCallback = true;
}

void parseRequest(const JsonDocument& doc) {
  if (doc.containsKey("state")) {
    bool powerState = (doc["state"] == "ON");
    setPower(powerState);
  }
}

void loadSettings() {
   File file = LittleFS.open(configFile, "r");
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

void saveSettings() {
   saveSettingsCallback = false;
   StaticJsonDocument<256> doc;
   doc["state"] = power ? "ON" : "OFF";
   File destFile = LittleFS.open(configFile, "w");
   if (serializeJson(doc, destFile) == 0) {
    destFile.close();
   }
   destFile.close();
}

void switchState() {
  lightStateCallback = false;
  StaticJsonDocument<256> doc;
  doc["state"] = power ? "ON" : "OFF";
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("backlight/hall/state", buffer, n);
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {
    parseRequest(doc);
  }
}

void reconnect() {
  while (!client.connected()) {
    yield();
    delay(100);
    if (client.connect(clientName, mqtt.login, mqtt.pass)) {
      client.subscribe("all/modules");
      client.subscribe("all/light");
      client.subscribe("backlight/hall");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  delay(2000);

  if (!LittleFS.begin()) {
    return;
  }
  loadSettings();

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  WiFiManagerParameter custom_mqtt_title("<br>MQTT configuration:");
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 20);
  WiFiManagerParameter custom_mqtt_username("user", "mqtt user", "", 20);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt_password", "", 20);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "", 10);

  wifiManager.addParameter(&custom_mqtt_title);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_port);

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

    savePrivateParams();
    delay(100);
  } else {
    initPrivate();
  }

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
  if (saveSettingsCallback)
    saveSettings();
  if (lightStateCallback) switchState();
  yield();
  delay(1);
}