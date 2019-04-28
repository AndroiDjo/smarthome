#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <AccelStepper.h>

const char* privateFile = "/private.json";
const char* configFile = "/config.json";
int jsonSize = 1024;
const int MS1 = 13;
const int MS2 = 15;

struct Mqtt {
  char host[20];
  int port;
  char login[20];
  char pass[20];
};

boolean power = true;
boolean is_running = false;
boolean forward;

int stepmaxspeed = 2000;
int stepacceleration = 300;
long stepmoveto = 10000;
int stepdel = 1;

WiFiClient espClient;
PubSubClient client(espClient);
Mqtt mqtt;
AccelStepper stepper(1,12,14); // 12(D6) - step, 14(D5) - dir

void somethingWrong(char* msg) {
   while (true) {
     digitalWrite(2, HIGH);
     delay(1000);
     digitalWrite(2, LOW);
     delay(1000);
     Serial.println(msg);
     if (client.connected()) {
       client.publish("log/error", msg);
     }
   }
}

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

void moveCurtains() {
  if (!power || is_running) {
    Serial.println("!power || is_running");
    return;
  }

  stepper.setMaxSpeed(stepmaxspeed);
  stepper.setAcceleration(stepacceleration);
  if (forward) {
    stepper.moveTo(stepmoveto);
  } else {
    stepper.moveTo(0);
  }

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

  Serial.println(stepper.currentPosition());
  Serial.println(forward);
  Serial.println(stepmoveto);
  Serial.print("stepper.distanceToGo()=");
  Serial.println(stepper.distanceToGo());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    somethingWrong("Failed to dsrlz mqtt message");
  }

  if (doc.containsKey("power")) {
    power = doc["power"];
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

  if(doc.containsKey("curtain")) {
     forward = doc["curtain"];
     moveCurtains();
  }
}

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

void setup() {
  Serial.begin(115200);
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("wifi connected");
  
  pinMode(2, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  delay(50);
  digitalWrite(2, LOW);
  delay(2000);
  
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    somethingWrong("Failed to mount file system");
  }
  initPrivate();

  client.setServer(mqtt.host, mqtt.port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  stepper.run();
}