#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <AccelStepper.h>
#include <ArduinoOTA.h>

const char* privateFile = "/private.json"; // файл для хранения учетных данных
const char* configFile = "/config.json"; // файл для хранения настроек модуля
const int jsonSize = 1024; // размер буфера для парсинга json
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

// учетные данные OTA
struct Ota {
  int port;
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
bool motor_sleep = false; // переводить мотор в спящий режим после вращения
bool is_sleep = true; // мотор находится в спящем режиме
bool rewriteconfig = false; // перезаписать конфигурационный файл
bool debug = false; // режим отладки

bool dynamicstepsize = false; // режим обеспечения плавности путем изменения размера шага
int del4prc; // процент вращения мотора на четверть шаге
int del8prc; // процент вращения мотора на 1/8 шаге

int stepmaxspeed = 2000; // максимальная скорость мотора
int stepacceleration = 300; // ускорение вращения
long stepmoveto = 10000; // количество шагов вращения
int stepdel = 8; // делитель шага

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
Ota ota;
AccelStepper stepper(1,12,14); // 12(D6) - step, 14(D5) - dir

// что-то пошло не так, требуется исправление ошибки
void somethingWrong(const char* msg) {
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

void smsg(const char* msg) {
  if (debug) {
    Serial.println(msg);
  }
}

// сохранение параметров в формате json в SPIFFS память
void saveJsonParams(JsonObject& json, bool rewrite) {
   
   bool existconfig = false;
   DynamicJsonDocument doc(jsonSize);
   if (!rewrite) {
      File file = SPIFFS.open(configFile, "r");
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
   }

   File destFile = SPIFFS.open(configFile, "w");
   bool srlzbool;
   if (existconfig) {
     srlzbool = (serializeJson(doc, destFile) == 0);
   } else {
     smsg("config file not found, create new");
     srlzbool = (serializeJson(json, destFile) == 0);
   }
   if (srlzbool) {
    destFile.close();
    somethingWrong("Failed to write to config file");
   }
   destFile.close();
}

void setStepDel() {
  if (stepdel == 4) {
    digitalWrite(MS1, LOW);
    digitalWrite(MS2, HIGH);
  } else {
    digitalWrite(MS1, HIGH);
    digitalWrite(MS2, HIGH);
  }
}

// установка значений по умолчанию для динамического переключения делителей шага
void setDynamicStepDefault() {
  smsg("setDynamicStepDefault");
  del4prc = 40;
  del8prc = 20;
}

// процедура обработки динамического переключения делителей шага
void dynamicStepLoop() {
  float curdistance = (float)(stepmoveto - abs(stepper.distanceToGo()));
  float prc;
  float half = (float)stepmoveto / 2.f;
  int stepdelbefore = stepdel;
  if (curdistance < half) {
    prc = curdistance / half * 100.f;
    if (prc <= del8prc && stepdel != 8) {
      stepdel = 8;
    } else if (prc > del8prc && stepdel != 4) {
      stepdel = 4;
    } 
  } else {
    prc = (curdistance - half) / half * 100.f;
    if (prc <= 100 - del8prc && stepdel != 4) {
      stepdel = 4;
    } else if (prc > 100 - del8prc && stepdel != 8) {
      stepdel = 8;
    } 
  }

  if (stepdel != stepdelbefore) {
    setStepDel();
  }
}

// подготовка к движению штор
void moveCurtains() {
  if (first_load && is_running) {
      somethingWrong("need calibrate!");    
  }
  first_load = false;
  if (!power || is_running) {
    smsg("!power || is_running");
    return;
  }

  stepper.setMaxSpeed(stepmaxspeed);
  stepper.setAcceleration(stepacceleration);
  if (forward && is_opened) {
    stepper.moveTo(stepper.currentPosition() + stepmoveto);
  } else if (!forward && !is_opened) {
    stepper.moveTo(stepper.currentPosition() - stepmoveto);
  }

  if (dynamicstepsize) {
    if ((!del4prc && !del8prc) ||
		  del4prc < del8prc) {
      setDynamicStepDefault();
    }
  } else {
    setStepDel();
  }

  if (stepper.distanceToGo() != 0) {
    smsg("Start moving");
    if (debug) {
      Serial.println(stepper.distanceToGo());
    }
    step_callback = true;
    is_running = true;
    step_last_dir = forward;

    if (is_sleep) {
      digitalWrite(PIN_SLP, HIGH);
      is_sleep = false;
      delay(50);
    }

    StaticJsonDocument<100> doc;
    doc["is_running"] = is_running;
    JsonObject jo = doc.as<JsonObject>();
    saveJsonParams(jo, false);
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

  if (doc.containsKey("motor_sleep")) {
    motor_sleep = doc["motor_sleep"];
  }

  if (doc.containsKey("del4prc")) {
    del4prc = doc["del4prc"];
  }

  if (doc.containsKey("del8prc")) {
    del8prc = doc["del8prc"];
  }

  if (doc.containsKey("rewriteconfig")) {
    rewriteconfig = doc["rewriteconfig"];
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
     smsg("loadJsonParams: config file not found");
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
  strlcpy(ota.pass, doc["ota_pass"], sizeof(ota.pass));
  ota.port = doc["ota_port"];
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
  saveJsonParams(jobj, rewriteconfig);
}

// переподключение к MQTT брокеру
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("nodemcu_curtain_childroom", mqtt.login, mqtt.pass)) {
      smsg("nodemcu_curtain_childroom connected");
      client.subscribe("all/modules");
      client.subscribe("curtain/childroom");
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
  smsg("Stop moving");
  is_running = false;
  is_opened = !step_last_dir;
  step_callback = false;

  if (motor_sleep) {
    digitalWrite(PIN_SLP, LOW);
    is_sleep = true;
  }
  
  StaticJsonDocument<100> doc;
  doc["is_running"] = is_running;
  doc["is_opened"] = is_opened;
  JsonObject jo = doc.as<JsonObject>();
  saveJsonParams(jo, false);
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
  // Подключение к роутеру. Если не удалось подключиться - перезагружаемся.
  wifiManager.setConfigPortalTimeout(10);
  if (!wifiManager.autoConnect("AutoConnectAP", "password123")) {
    smsg("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  smsg("wifi connected");
  delay(2000);  

  smsg("Mounting FS...");
  if (!SPIFFS.begin()) {
    somethingWrong("Failed to mount file system");
  }
  initPrivate();

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
  digitalWrite(2, LOW);
  setStepDel();
}

void loop() {
  if (stepper.distanceToGo() == 0) {
    ArduinoOTA.handle();
    if (!client.connected()) {
      reconnect();
    }
    if (step_callback) {
      stepperCallback();
    }
    client.loop();
    delay(10);
  } else {
    if (dynamicstepsize) {
      dynamicStepLoop();
    }
  }
  stepper.run();
}
