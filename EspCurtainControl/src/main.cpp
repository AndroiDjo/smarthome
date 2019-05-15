#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <AccelStepper.h>

const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля
int jsonSize = 1024; // размер буфера для парсинга json
// MS1 и MS2 - пины для управления делителем шага мотора
const int MS1 = 13; // D7
const int MS2 = 15; // D8

const int PIN_SLP = 4; // D2 - спящий режим шагового двигателя
const int PIN_PWR = 5; // D1 - выключение шагового двигателя

// учетные данные Mosquitto
struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

bool power = true; // переменная отвечает за отключение/включение мотора
bool is_running = false; // на данный момент крутится
bool is_opened = false; // шторы открыты
bool forward = true; // направление вращения
bool step_callback = false; // после завершения вращения нужен callback
bool step_last_dir = forward; // запоминаем направление вращения перед началом вращения
bool first_load = true; // первая загрузка платы (после ресета)
bool is_error = false; // получена ошибка, требуется ручное вмешательство

bool dynamicstepsize = false; // режим обеспечения плавности путем изменения размера шага
int del1prc; // процент вращения мотора на полном шаге
int del2prc; // процент вращения мотора на полушаге
int del4prc; // процент вращения мотора на четверть шаге
int del8prc; // процент вращения мотора на 1/8 шаге
long curstep = 0; // текущий шаг

int stepmaxspeed = 2000; // максимальная скорость мотора
int stepacceleration = 300; // ускорение вращения
long stepmoveto = 10000; // количество шагов вращения
int stepdel = 1; // делитель шага

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
AccelStepper stepper(1,12,14); // 12(D6) - step, 14(D5) - dir

// что-то пошло не так, требуется исправление ошибки
void somethingWrong(char* msg) {
   is_error = true;
   while (is_error) {
     digitalWrite(2, HIGH);
     delay(1000);
     digitalWrite(2, LOW);
     delay(1000);
     Serial.println(msg);
     if (client.connected()) {
       client.publish("log/error", msg);
       client.loop();
     }
   }
}

// поиск вложенного объекта json
bool containsNestedKey(const JsonObject& obj, const char* key) {
    for (const JsonPair& pair : obj) {
        if (!strcmp(pair.key, key))
            return true;

        if (containsNestedKey(pair.value.as<JsonObject>(), key)) 
            return true;
    }

    return false;
}

// сохранение параметров в формате json в SPIFFS память
void saveJsonParams(JsonObject& json) {
   File file = SPIFFS.open(configFile, "r");
   DynamicJsonDocument doc(jsonSize);
   bool existconfig = false;
   if (file) {   
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      file.close();
      somethingWrong("Failed to dsrlz config file");
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
    somethingWrong("Failed to write to config file");
   }
   destFile.close();
}

// установка значений по умолчанию для динамического переключения делителей шага
void setDynamicStepDefault() {
  int del1prc = 100;
  int del2prc = 100;
  int del4prc = 40;
  int del8prc = 20;
}

// процедура обработки динамического переключения делителей шага
void dynamicStepLoop() {
  int prc;
  int half = stepmoveto / 2;
  if (curstep < half) {
    prc = curstep / half * 100;
    if (prc <= del8prc && stepdel != 8) {
      stepdel = 8;
    } else if (prc > del8prc && prc <= del4prc && stepdel != 4) {
      stepdel = 4;
    } else if (prc > del4prc && prc <= del2prc && stepdel != 2) {
      stepdel = 2;
    } else if (prc > del2prc && prc <= del1prc && stepdel != 1) {
      stepdel = 1;
    }
  } else {
    prc = (curstep - half) / half * 100;
    //if (prc > )
  }
}

// подготовка к движению штор
void moveCurtains() {
  if (first_load && is_running) {
      somethingWrong("need calibrate!");    
  }
  first_load = false;
  if (!power || is_running) {
    Serial.println("!power || is_running");
    return;
  }

  curstep = 0;
  stepper.setMaxSpeed(stepmaxspeed);
  stepper.setAcceleration(stepacceleration);
  if (forward && is_opened) {
    stepper.moveTo(stepper.currentPosition() + stepmoveto);
  } else if (!forward && !is_opened) {
    stepper.moveTo(stepper.currentPosition() - stepmoveto);
  }

  if (dynamicstepsize) {
    if ((!del1prc &&) ) {
      setDynamicStepDefault();
    }
  } else {
    if (stepdel == 2) {
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
    } else if (stepdel == 4) {
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
    } else if (stepdel == 8) {
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, HIGH);
    } else {
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
    }
  }

  if (stepper.distanceToGo() != 0) {
    step_callback = true;
    is_running = true;
    step_last_dir = forward;

    digitalWrite(PIN_SLP, HIGH);
    delay(50);

    StaticJsonDocument<100> doc;
    doc["is_running"] = is_running;
    JsonObject jo = doc.as<JsonObject>();
    saveJsonParams(jo);
  }
}

// обработка json команд
void parseRequest(DynamicJsonDocument& doc) {
  if (doc.containsKey("power")) {
    power = doc["power"];
    if (!is_running) { // на ходу не выключаем мотор
      if (power) {
        digitalWrite(PIN_PWR, LOW); // мотор включен
      } else {
        digitalWrite(PIN_PWR, HIGH); // мотор выключен
      }
    }
  }

  if (doc.containsKey("stepmaxspeed")) {
    stepmaxspeed = doc["stepmaxspeed"];
  }

  if (doc.containsKey("stepacceleration")) {
    stepacceleration = doc["stepacceleration"];
  }

  if (doc.containsKey("stepmoveto")) {
    stepmoveto = doc["stepmoveto"];
  }

  if (doc.containsKey("stepdel")) {
    stepdel = doc["stepdel"];
  }

  if (doc.containsKey("is_running")) {
    is_running = doc["is_running"];
  }

  if (doc.containsKey("is_opened")) {
    is_opened = doc["is_opened"];
  }

  if (doc.containsKey("is_error")) {
    is_error = doc["is_error"];
  }

  if (doc.containsKey("dynamicstepsize")) {
    dynamicstepsize = doc["dynamicstepsize"];
  }

  if(doc.containsKey("curtain")) {
     forward = doc["curtain"];
     moveCurtains();
  }
}

// загрузка json параметров из SPIFFS
void loadJsonParams() {
   File file = SPIFFS.open(configFile, "r");
   DynamicJsonDocument doc(jsonSize);
   if (file) {
     DeserializationError error = deserializeJson(doc, file);
     if (error) {
      file.close();
      somethingWrong("loadJsonParams: Failed to dsrlz config file");
     }
     file.close();
     serializeJsonPretty(doc, Serial);
     parseRequest(doc);
   } else {
     Serial.println("loadJsonParams: config file not found");
   }
}

// чтение учетных данных из SPIFFS
void initPrivate() {
  File file = SPIFFS.open(privateFile, "r");
  StaticJsonDocument<256> doc;
  if (!file) {
    somethingWrong("Failed to open private file");
  } 
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    file.close();
    somethingWrong("Failed to dsrlz private file");
  }
  file.close();
  strlcpy(mqtt.host, doc["mqtt_server"], sizeof(mqtt.host));
  strlcpy(mqtt.login, doc["mqtt_login"], sizeof(mqtt.login));
  strlcpy(mqtt.pass, doc["mqtt_pass"], sizeof(mqtt.pass));
  mqtt.port = doc["mqtt_port"];
}

// обработчик MQTT команд
void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    somethingWrong("Failed to dsrlz mqtt message");
  }

  parseRequest(doc);
  JsonObject jobj = doc.as<JsonObject>();
  saveJsonParams(jobj);
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("nodemcu_curtain_hall", mqtt.login, mqtt.pass)) {
      Serial.println("connected");
      client.subscribe("all/modules");
      client.subscribe("curtain/hall");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// функция-callback вызываемая после завершения вращения мотора
void stepperCallback() {
  is_running = false;
  is_opened = !step_last_dir;
  step_callback = false;

  digitalWrite(PIN_SLP, LOW);
  
  StaticJsonDocument<100> doc;
  doc["is_running"] = is_running;
  doc["is_opened"] = is_opened;
  JsonObject jo = doc.as<JsonObject>();
  saveJsonParams(jo);
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(PIN_SLP, OUTPUT);
  pinMode(PIN_PWR, OUTPUT);
  delay(10);
  digitalWrite(PIN_SLP, LOW);
  digitalWrite(PIN_PWR, LOW);

  Serial.begin(115200);
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("wifi connected");
  delay(2000);  
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    somethingWrong("Failed to mount file system");
  }
  initPrivate();
  client.setServer(mqtt.host, mqtt.port);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  loadJsonParams();
  digitalWrite(2, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (stepper.distanceToGo() == 0 && step_callback) {
    stepperCallback();
  }
  if (dynamicstepsize) {

  }
  stepper.run();
}
