#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
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
bool lightStateCallback = false;
const char* clientName = "nodemcu_lightcloud"; // название MQTT клиента
int relayPin = 4;
const uint16_t kIrLed = 5;  
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
bool shouldSaveConfig = false;
bool acCallback = false;
enum acMode {off, cool, heat};
acMode currentAcMode = acMode::off;
uint8_t acTemp = 25;

// команды сплита
uint16_t rCold25[229] = {3098, 3010,  3096, 4360,  616, 1556,  524, 550,  594, 1564,  616, 452,  504, 572,  596, 1560,  618, 1586,  472, 554,  616, 1556,  580, 492,  596, 462,  616, 1554,  580, 496,  620, 436,  616, 1556,  520, 1656,  596, 454,  616, 456,  504, 572,  594, 460,  614, 1560,  522, 552,  594, 1560,  620, 452,  504, 574,  592, 458,  614, 456,  550, 526,  594, 460,  614, 454,  584, 492,  586, 472,  614, 452,  618, 1562,  560, 1598,  616, 1556,  500, 1678,  596, 450,  584, 1596,  586, 468,  616, 454,  582, 1596,  586, 1568,  618, 452,  502, 1676,  596, 1556,  576, 496,  594, 462,  614, 454,  588, 488,  582, 476,  612, 456,  616, 456,  522, 552,  594, 460,  614, 454,  578, 498,  620, 438,  616, 1558,  544, 528,  622, 434,  616, 452,  574, 500,  558, 502,  610, 456,  612, 462,  504, 556,  612, 456,  616, 456,  524, 552,  594, 460,  614, 454,  578, 496,  620, 438,  616, 452,  616, 458,  532, 542,  594, 458,  616, 456,  502, 572,  596, 460,  616, 454,  548, 526,  590, 466,  616, 454,  582, 494,  502, 554,  614, 456,  616, 456,  504, 572,  594, 460,  614, 456,  552, 524,  594, 462,  614, 454,  586, 488,  562, 496,  612, 456,  616, 456,  504, 572,  594, 460,  614, 456,  504, 572,  594, 1564,  614, 452,  504, 1674,  596, 454,  618, 456,  504, 1676,  596, 452,  616, 1560,  562, 494,  616};  // UNKNOWN 61952764
uint16_t rCold26[229] = {3024, 3076,  3112, 4356,  514, 1696,  542, 486,  602, 1586,  576, 476,  606, 452,  600, 1586,  492, 1670,  566, 486,  604, 1586,  578, 474,  606, 1586,  580, 454,  540, 532,  604, 458,  510, 1670,  492, 1672,  574, 480,  602, 476,  566, 490,  604, 474,  574, 1618,  582, 450,  602, 1582,  492, 560,  606, 458,  512, 560,  606, 458,  540, 532,  606, 454,  568, 506,  606, 456,  600, 472,  606, 456,  570, 1614,  492, 560,  604, 1588,  580, 470,  570, 1622,  580, 450,  600, 1582,  492, 560,  606, 1586,  580, 1582,  580, 470,  606, 1556,  610, 1556,  604, 474,  536, 516,  604, 476,  492, 560,  606, 476,  538, 514,  606, 476,  494, 560,  606, 458,  510, 560,  606, 458,  512, 560,  606, 1560,  608, 468,  566, 488,  606, 476,  538, 516,  606, 476,  492, 562,  606, 458,  510, 560,  606, 476,  492, 560,  606, 460,  510, 560,  606, 458,  510, 560,  606, 456,  542, 532,  606, 454,  570, 504,  606, 456,  600, 474,  606, 456,  602, 472,  606, 452,  598, 478,  606, 454,  602, 474,  606, 450,  604, 474,  602, 458,  602, 474,  572, 484,  604, 474,  566, 490,  604, 474,  572, 482,  606, 474,  566, 488,  606, 476,  538, 516,  604, 476,  492, 560,  606, 458,  510, 560,  604, 476,  512, 542,  606, 460,  510, 560,  606, 1558,  608, 1556,  602, 474,  606, 1558,  608, 452,  564, 508,  604};  // HAIER_AC_YRW02
uint16_t rHeat25[229] = {3030, 3066,  3074, 4394,  604, 1582,  546, 508,  606, 1586,  582, 452,  512, 560,  606, 1588,  580, 1582,  578, 472,  606, 1588,  580, 448,  572, 1614,  492, 560,  606, 458,  540, 532,  606, 1560,  608, 1584,  578, 470,  542, 514,  604, 476,  536, 518,  606, 1566,  602, 470,  566, 1626,  580, 448,  602, 474,  602, 458,  600, 478,  570, 486,  602, 476,  564, 490,  604, 476,  538, 516,  606, 476,  492, 1674,  564, 1596,  606, 450,  570, 504,  604, 456,  600, 1588,  520, 1640,  492, 560,  606, 1588,  580, 1582,  580, 470,  600, 1564,  608, 1580,  582, 470,  494, 560,  606, 460,  564, 508,  604, 460,  574, 498,  606, 454,  598, 478,  606, 452,  602, 476,  604, 1562,  608, 450,  574, 502,  606, 452,  602, 476,  604, 450,  604, 476,  600, 456,  604, 476,  570, 484,  606, 476,  564, 490,  606, 476,  538, 516,  606, 476,  492, 562,  604, 460,  510, 560,  606, 458,  596, 476,  606, 456,  602, 472,  604, 456,  602, 474,  606, 454,  602, 474,  574, 484,  604, 476,  566, 490,  604, 476,  540, 516,  604, 476,  494, 560,  604, 460,  510, 560,  606, 458,  510, 562,  606, 456,  542, 532,  606, 454,  602, 472,  606, 454,  602, 476,  576, 482,  602, 474,  570, 488,  604, 474,  542, 514,  604, 1564,  602, 470,  574, 1594,  606, 450,  568, 506,  606, 454,  574, 1594,  594, 476,  606};  // UNKNOWN 8787E78A
uint16_t rHeat26[229] = {3060, 3040,  3046, 4420,  608, 1582,  576, 474,  608, 1586,  582, 450,  540, 534,  606, 1448,  720, 1556,  606, 470,  540, 1650,  582, 448,  604, 474,  572, 1590,  610, 448,  604, 474,  606, 1558,  608, 1582,  582, 452,  512, 560,  606, 456,  540, 532,  606, 1556,  612, 452,  512, 1698,  538, 488,  606, 476,  538, 516,  606, 476,  492, 562,  606, 460,  510, 560,  606, 454,  568, 506,  606, 454,  596, 1588,  492, 1668,  494, 560,  606, 460,  508, 1672,  568, 486,  604, 476,  540, 514,  606, 1562,  604, 1582,  584, 466,  606, 1586,  582, 1552,  608, 454,  510, 560,  606, 460,  512, 560,  608, 456,  542, 532,  606, 454,  582, 494,  608, 452,  600, 476,  606, 1560,  608, 450,  566, 508,  606, 454,  574, 502,  604, 454,  602, 474,  572, 484,  604, 474,  568, 488,  606, 476,  538, 514,  608, 474,  494, 560,  608, 458,  510, 560,  606, 458,  572, 500,  606, 456,  606, 468,  606, 454,  602, 474,  606, 450,  604, 474,  576, 478,  606, 474,  570, 486,  606, 474,  540, 518,  602, 476,  492, 562,  606, 456,  540, 534,  606, 454,  568, 506,  606, 454,  576, 500,  606, 452,  602, 474,  606, 452,  602, 476,  602, 452,  606, 474,  538, 516,  606, 476,  492, 562,  606, 458,  512, 1670,  540, 1620,  608, 450,  604, 474,  570, 1596,  610, 448,  604, 1582,  548, 504,  606, 458,  602};  // UNKNOWN CEF6B96E
uint16_t rOff[229] = {3022, 3080,  3104, 4358,  558, 1622,  608, 448,  598, 1588,  490, 562,  604, 460,  510, 1696,  538, 1624,  582, 448,  600, 1586,  492, 560,  606, 460,  510, 1672,  600, 452,  604, 476,  568, 486,  606, 474,  540, 514,  606, 474,  514, 540,  604, 474,  540, 1650,  582, 448,  600, 1586,  518, 532,  606, 458,  566, 506,  606, 456,  512, 560,  606, 456,  566, 506,  606, 458,  596, 476,  604, 456,  540, 532,  606, 454,  572, 1612,  492, 560,  606, 1560,  604, 472,  604, 450,  604, 476,  574, 1590,  610, 1550,  610, 454,  554, 1652,  578, 1556,  610, 450,  540, 532,  606, 456,  568, 506,  606, 458,  600, 472,  606, 454,  568, 506,  606, 454,  572, 502,  606, 456,  600, 472,  606, 1558,  608, 452,  510, 560,  606, 460,  508, 560,  606, 458,  512, 560,  606, 458,  538, 534,  606, 458,  510, 560,  606, 456,  512, 560,  606, 456,  566, 508,  604, 458,  596, 476,  606, 456,  542, 532,  606, 454,  570, 504,  606, 454,  598, 478,  606, 456,  570, 506,  604, 454,  598, 478,  606, 452,  602, 474,  606, 454,  596, 480,  604, 452,  602, 474,  606, 452,  602, 476,  600, 458,  602, 476,  606, 450,  604, 474,  604, 452,  604, 474,  574, 484,  602, 476,  564, 1600,  608, 448,  574, 1610,  492, 1670,  608, 1554,  606, 1552,  608, 470,  538, 516,  604, 1562,  604, 472,  604, 1560,  608};  // UNKNOWN C969258D


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
  lightStateCallback = true;
}

void parseRequest(const JsonDocument& doc) {
  if (doc.containsKey("power")) {
    // Deprecated, use "state" instead!
    setPower(doc["power"]);
  }

  if (doc.containsKey("clap")) {
    saveSettingsCallback = true;
    clapSensor = doc["clap"];
  }

  if (doc.containsKey("gettemp") || doc.containsKey("getall")) {
    tempSensorCallback = true;
  }

  if (doc.containsKey("mode")) {
    if (doc["mode"] == "off") {
      currentAcMode = acMode::off;
    } else if (doc["mode"] == "cool") {
      currentAcMode = acMode::cool;
    } else if (doc["mode"] == "heat") {
      currentAcMode = acMode::heat;
    }
  }

  if (doc.containsKey("temp")) {
    acTemp = doc["temp"];
  }

  if (doc.containsKey("state")) {
    bool powerState = (doc["state"] == "ON");
    setPower(powerState);
  }
}

// сохранение параметров в формате json в LittleFS память
void saveSettings() {
   saveSettingsCallback = false;
   StaticJsonDocument<256> doc;
   doc["power"] = power;
   doc["clap"] = clapSensor;
   File destFile = LittleFS.open(configFile, "w");
   if (serializeJson(doc, destFile) == 0) {
    destFile.close();
    Serial.println("Failed to write to config file");
   }
   destFile.close();
}

// загрузка json параметров из LittleFS
void loadSettings() {
  Serial.println("loadSettings");
   File file = LittleFS.open(configFile, "r");
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
  /*Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();*/

  bool isAcCommand = false;
  StaticJsonDocument<512> doc;
  if (strcmp(topic, "childroom/ac/mode/set")==0 || strcmp(topic, "childroom/ac/temp/set")==0) {
    isAcCommand = true;
    payload[length] = '\0';
    String payloadStr = String((char*)payload);
    if (strcmp(topic, "childroom/ac/mode/set")==0) {
      doc["mode"] = payloadStr;
      saveSettingsCallback = true;
    } else if (strcmp(topic, "childroom/ac/temp/set")==0) {
      doc["temp"] = payloadStr;
      saveSettingsCallback = true;
    }
  } else {
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
      Serial.println("Failed to dsrlz mqtt message");
    }
  }

  parseRequest(doc);
  acCallback = isAcCommand;
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
      client.subscribe("childroom/ac/mode/set");
      client.subscribe("childroom/ac/temp/set");
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    doc["temperature"] = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    doc["humidity"] = event.relative_humidity;
  }
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("sensor/childroom/temp", buffer, n);
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

void acLoop() {
  if (acCallback) {
    acCallback = false;
    switch (currentAcMode) {
      case off:
        irsend.sendRaw(rOff, 229, 38);
        break;
      case cool:
        if (acTemp==25) {
          irsend.sendRaw(rCold25, 229, 38);
        } else if (acTemp==26) {
          irsend.sendRaw(rCold26, 229, 38);
        }
        break;
      case heat:
        if (acTemp==25) {
          irsend.sendRaw(rHeat25, 229, 38);
        } else if (acTemp==26) {
          irsend.sendRaw(rHeat26, 229, 38);
        }
        break;
    }
  }
}

void switchState() {
  lightStateCallback = false;
  StaticJsonDocument<256> doc;
  doc["state"] = power ? "ON" : "OFF";
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("lightcloud/childroom/state", buffer, n);
}

void saveConfigCallback () {
  Serial.println("should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);

  delay(2000);
  Serial.println("Mounting FS...");

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  
  pinMode(soundSensor, INPUT);
  pinMode(led,OUTPUT);
  pinMode(relayPin,OUTPUT);
  irsend.begin();
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
  if (!wifiManager.autoConnect()) {
  //if (!wifiManager.autoConnect("AutoConnectAP", "password123")) {
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

  loadSettings();
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected())
    reconnect();
  client.loop();
  if (saveSettingsCallback)
    saveSettings();
  acLoop();
  if (clapSensor)
    clapLoop();
  if (lightStateCallback) switchState();
  if (tempSensorCallback)
    getTemp();
  delayLoop();
}
