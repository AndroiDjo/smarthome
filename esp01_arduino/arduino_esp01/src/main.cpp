#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <IRremote.h>

bool tempSensorCallback = false; // колбэк для публикации данных с датчика температуры
#define DHTPIN 12     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);
int soundSensor = 11;
int clap = 0;
long detection_range_start = 0;
long detection_range = 0;
IRsend irsend;
uint16_t* cmdraw = NULL;
int rawSize = 0;
int rawIndex = 0;
long splitTimer = 0;

void getTemp() {
  tempSensorCallback = false;
  StaticJsonDocument<256> doc;
  sensors_event_t event;
  const char* clientid = "halltemp";
  doc[clientid]["kind"] = "temperature";
  doc[clientid]["location"] = "hall";
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    doc[clientid]["temperature"] = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    doc[clientid]["humidity"] = event.relative_humidity;
  }
  serializeJson(doc, Serial);
  delay(1000);
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
  if (doc.containsKey("gettemp") || doc.containsKey("getall")) {
    tempSensorCallback = true;
  }

  if (doc.containsKey("split")) {
    splitCommand(doc);
  }
}

void serialLoop() {
  if (Serial.available()) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, Serial);
     if (!error) {
       parseRequest(doc);
     }
  }
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
      const char* clientid = "hallclap";
      doc[clientid]["kind"] = "sound";
      doc[clientid]["location"] = "hall";
      serializeJson(doc, Serial);
    }
    clap = 0;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(soundSensor, INPUT);
  dht.begin();
}

void loop() {
  clapLoop();
  serialLoop();
  if (tempSensorCallback)
    getTemp();
}