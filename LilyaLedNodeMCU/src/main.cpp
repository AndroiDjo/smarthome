#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "FastLED.h"
#include <GyverButton.h>

const int jsonSize = 512;
const char* privateFile = "/private.json";
const char* configFile = "/config.json";
const char* clientName = "nodemcu_rgb_childroom";

bool shouldSaveConfig = false;
bool saveSettingsCallback = false;
bool lightStateCallback = false;
bool power = true;
int delayCounter = 0;
int delayLimit = 20;

/*
  Скетч создан на основе FASTSPI2 EFFECTS EXAMPLES автора teldredge (www.funkboxing.com)
  А также вот этой статьи https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#cylon
  Доработан, переведён и разбит на файлы 2017 AlexGyver
  Смена выбранных режимов кнопкой. Кнопка подключена на D2 и GND
*/

#define LED_COUNT 19          // число светодиодов в кольце/ленте
#define LED_DT D1             // пин, куда подключен DIN ленты
#define BTN_PIN D2

GButton touch(BTN_PIN, LOW_PULL, NORM_OPEN);

int max_bright = 255;          // максимальная яркость (0 - 255)

byte fav_modes[] = {2, 11, 14, 25, 27, 30, 41};  // список "любимых" режимов
byte num_modes = sizeof(fav_modes);         // получить количество "любимых" режимов (они все по 1 байту..)
unsigned long change_time, last_change, last_bright;
int new_bright;

volatile byte ledMode = 3;
/*
  Стартовый режим
  0 - все выключены
  1 - все включены
  3 - кольцевая радуга
  888 - демо-режим
*/

// цвета мячиков для режима
byte ballColors[3][3] = {
  {0xff, 0, 0},
  {0xff, 0xff, 0xff},
  {0   , 0   , 0xff}
};

// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------
bool _BTN_CONNECTED = true;
int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR

volatile uint32_t btnTimer;
volatile byte modeCounter;
volatile boolean changeFlag;
// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------

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
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("rgb/childroom/state", buffer, n);
}

void parseRequest(const JsonDocument& doc) {
  // if (doc.containsKey("state")) {
  //   power = (doc["state"] == "ON");
  //   if (power) {
  //     roomSensorTime = millis();
  //   } else {
  //     roomSensorTime = millis() - sensorTimeout;
  //   }
  //   setLed();
  // }

  // if (doc.containsKey("brightness")) {
  //   brightness = doc["brightness"];
  //   roomSensorTime = millis();
  //   setLed();
  // }

  // if (doc.containsKey("color")) {
  //   color.r = doc["color"]["r"];
  //   color.g = doc["color"]["g"];
  //   color.b = doc["color"]["b"];
  //   roomSensorTime = millis();
  //   setLed();
  // }

  // if (doc.containsKey("sensor_timeout")) {
  //   sensorTimeout = doc["sensor_timeout"];
  // }
  
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
      client.subscribe("rgb/childroom");
    } else {
      Serial.println("Error on connecting to MQTT");
      delay(5000);
    }
  }
}

void checkSensors() {
  // if (digitalRead(PIN_PIR) == HIGH) {
  //   roomSensorTime = millis();
  // }
  // long deltaTime = millis() - roomSensorTime;
  // if (deltaTime < sensorTimeout) {
  //   if (!power) {
  //     power = true;
  //     setLed();
  //   }
  // } else {
  //   if (power) {
  //     power = false;
  //     setLed();
  //   }
  // }
}

void delayLoop() {
  delayCounter++;
  if (delayCounter > delayLimit) {
    delayCounter = 0;
    yield();
    delay(1);
  }
}

//------------------------------------- UTILITY FXNS --------------------------------------
boolean safeDelay(int delTime) {
  uint32_t thisTime = millis();
  while (millis() - thisTime <= delTime) {
    if (changeFlag) {
      changeFlag = false;
      return true;
    }
  }
  return false;
}
//---SET THE COLOR OF A SINGLE RGB LED
void set_color_led(int adex, int cred, int cgrn, int cblu) {
  leds[adex].setRGB( cred, cgrn, cblu);
}

//---FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}

//---FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  }
  else {
    r = 0;
  }
  return r;
}

//---FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}

void copy_led_array() {
  for (int i = 0; i < LED_COUNT; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  FastLED.show();
}

//------------------------LED EFFECT FUNCTIONS------------------------
void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i] = CHSV(ahue, thissat, 255);
  }
}

void rainbow_fade() {                         //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(ihue, thissat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void color_bounce() {                        //-m5-BOUNCE COLOR (SINGLE LED)
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void color_bounceFADE() {                    //-m6-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  int iL1 = adjacent_cw(idex);
  int iL2 = adjacent_cw(iL1);
  int iL3 = adjacent_cw(iL2);
  int iR1 = adjacent_ccw(idex);
  int iR2 = adjacent_ccw(iR1);
  int iR3 = adjacent_ccw(iR2);
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    else if (i == iL1) {
      leds[i] = CHSV(thishue, thissat, 150);
    }
    else if (i == iL2) {
      leds[i] = CHSV(thishue, thissat, 80);
    }
    else if (i == iL3) {
      leds[i] = CHSV(thishue, thissat, 20);
    }
    else if (i == iR1) {
      leds[i] = CHSV(thishue, thissat, 150);
    }
    else if (i == iR2) {
      leds[i] = CHSV(thishue, thissat, 80);
    }
    else if (i == iR3) {
      leds[i] = CHSV(thishue, thissat, 20);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void ems_lightsONE() {                    //-m7-EMERGENCY LIGHTS (TWO COLOR SINGLE LED)
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idexR) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    else if (i == idexB) {
      leds[i] = CHSV(thathue, thissat, 255);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void ems_lightsALL() {                  //-m8-EMERGENCY LIGHTS (TWO COLOR SOLID)
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  leds[idexR] = CHSV(thishue, thissat, 255);
  leds[idexB] = CHSV(thathue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void flicker() {                          //-m9-FLICKER EFFECT
  int random_bright = random(0, 255);
  int random_delay = random(10, 100);
  int random_bool = random(0, random_bright);
  if (random_bool < 10) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thishue, thissat, random_bright);
    }
    LEDS.show();
    if (safeDelay(random_delay)) return;
  }
}

void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    ibright = ibright - 1;
    if (ibright <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void pulse_one_color_all_rev() {           //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, isat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void fade_vertical() {                    //-m12-FADE 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  ibright = ibright + 10;
  if (ibright > 255) {
    ibright = 0;
  }
  leds[idexA] = CHSV(thishue, thissat, ibright);
  leds[idexB] = CHSV(thishue, thissat, ibright);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
  int temprand;
  for (int i = 0; i < LED_COUNT; i++ ) {
    temprand = random(0, 100);
    if (temprand > 50) {
      leds[i].r = 255;
    }
    if (temprand <= 50) {
      leds[i].r = 0;
    }
    leds[i].b = 0; leds[i].g = 0;
  }
  LEDS.show();
}

void rule30() {                          //-m13-1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
  if (bouncedirection == 0) {
    random_red();
    bouncedirection = 1;
  }
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void random_march() {                   //-m14-RANDOM MARCH CCW
  copy_led_array();
  int iCCW;
  leds[0] = CHSV(random(0, 255), 255, 255);
  for (int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    leds[idex].r = ledsX[iCCW][0];
    leds[idex].g = ledsX[iCCW][1];
    leds[idex].b = ledsX[iCCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void rwb_march() {                    //-m15-R,W,B MARCH CCW
  copy_led_array();
  int iCCW;
  idex++;
  if (idex > 2) {
    idex = 0;
  }
  switch (idex) {
    case 0:
      leds[0].r = 255;
      leds[0].g = 0;
      leds[0].b = 0;
      break;
    case 1:
      leds[0].r = 255;
      leds[0].g = 255;
      leds[0].b = 255;
      break;
    case 2:
      leds[0].r = 0;
      leds[0].g = 0;
      leds[0].b = 255;
      break;
  }
  for (int i = 1; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    leds[i].r = ledsX[iCCW][0];
    leds[i].g = ledsX[iCCW][1];
    leds[i].b = ledsX[iCCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void radiation() {                   //-m16-SORT OF RADIATION SYMBOLISH-
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N6; i++ ) {    //-HACKY, I KNOW...
    tcount = tcount + .02;
    if (tcount > 3.14) {
      tcount = 0.0;
    }
    ibright = int(sin(tcount) * 255);
    int j0 = (i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, ibright);
    leds[j1] = CHSV(thishue, thissat, ibright);
    leds[j2] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void color_loop_vardelay() {                    //-m17-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
  idex++;
  if (idex > LED_COUNT) {
    idex = 0;
  }
  int di = abs(TOP_INDEX - idex);
  int t = constrain((10 / di) * 10, 10, 500);
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(0, thissat, 255);
    }
    else {
      leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
    }
  }
  LEDS.show();
  if (safeDelay(t)) return;
}

void white_temps() {                            //-m18-SHOW A SAMPLE OF BLACK BODY RADIATION COLOR TEMPERATURES
  int N9 = int(LED_COUNT / 9);
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i >= 0 && i < N9) {
      leds[i].r = 255;  //-CANDLE - 1900
      leds[i].g = 147;
      leds[i].b = 41;
    }
    if (i >= N9 && i < N9 * 2) {
      leds[i].r = 255;  //-40W TUNG - 2600
      leds[i].g = 197;
      leds[i].b = 143;
    }
    if (i >= N9 * 2 && i < N9 * 3) {
      leds[i].r = 255;  //-100W TUNG - 2850
      leds[i].g = 214;
      leds[i].b = 170;
    }
    if (i >= N9 * 3 && i < N9 * 4) {
      leds[i].r = 255;  //-HALOGEN - 3200
      leds[i].g = 241;
      leds[i].b = 224;
    }
    if (i >= N9 * 4 && i < N9 * 5) {
      leds[i].r = 255;  //-CARBON ARC - 5200
      leds[i].g = 250;
      leds[i].b = 244;
    }
    if (i >= N9 * 5 && i < N9 * 6) {
      leds[i].r = 255;  //-HIGH NOON SUN - 5400
      leds[i].g = 255;
      leds[i].b = 251;
    }
    if (i >= N9 * 6 && i < N9 * 7) {
      leds[i].r = 255;  //-DIRECT SUN - 6000
      leds[i].g = 255;
      leds[i].b = 255;
    }
    if (i >= N9 * 7 && i < N9 * 8) {
      leds[i].r = 201;  //-OVERCAST SKY - 7000
      leds[i].g = 226;
      leds[i].b = 255;
    }
    if (i >= N9 * 8 && i < LED_COUNT) {
      leds[i].r = 64;  //-CLEAR BLUE SKY - 20000
      leds[i].g = 156;
      leds[i].b = 255;
    }
  }
  LEDS.show();
  delay(100);
}

void sin_bright_wave() {        //-m19-BRIGHTNESS SINE WAVE
  for (int i = 0; i < LED_COUNT; i++ ) {
    tcount = tcount + .1;
    if (tcount > 3.14) {
      tcount = 0.0;
    }
    ibright = int(sin(tcount) * 255);
    leds[i] = CHSV(thishue, thissat, ibright);
    LEDS.show();
    if (safeDelay(thisdelay)) return;
  }
}

void pop_horizontal() {        //-m20-POP FROM LEFT TO RIGHT UP THE RING
  int ix;
  if (bouncedirection == 0) {
    bouncedirection = 1;
    ix = idex;
  }
  else if (bouncedirection == 1) {
    bouncedirection = 0;
    ix = horizontal_index(idex);
    idex++;
    if (idex > TOP_INDEX) {
      idex = 0;
    }
  }
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == ix) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    else {
      leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
    }
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void quad_bright_curve() {      //-m21-QUADRATIC BRIGHTNESS CURVER
  int ax;
  for (int x = 0; x < LED_COUNT; x++ ) {
    if (x <= TOP_INDEX) {
      ax = x;
    }
    else if (x > TOP_INDEX) {
      ax = LED_COUNT - x;
    }
    int a = 1; int b = 1; int c = 0;
    int iquad = -(ax * ax * a) + (ax * b) + c; //-ax2+bx+c
    int hquad = -(TOP_INDEX * TOP_INDEX * a) + (TOP_INDEX * b) + c;
    ibright = int((float(iquad) / float(hquad)) * 255);
    leds[x] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void flame() {                                    //-m22-FLAMEISH EFFECT
  int idelay = random(0, 35);
  float hmin = 0.1; float hmax = 45.0;
  float hdif = hmax - hmin;
  int randtemp = random(0, 3);
  float hinc = (hdif / float(TOP_INDEX)) + randtemp;
  int ihue = hmin;
  for (int i = 0; i <= TOP_INDEX; i++ ) {
    ihue = ihue + hinc;
    leds[i] = CHSV(ihue, thissat, 255);
    int ih = horizontal_index(i);
    leds[ih] = CHSV(ihue, thissat, 255);
    leds[TOP_INDEX].r = 255; leds[TOP_INDEX].g = 255; leds[TOP_INDEX].b = 255;
    LEDS.show();
    if (safeDelay(idelay)) return;
  }
}

void rainbow_vertical() {                        //-m23-RAINBOW 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + thisstep;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, thissat, 255);
  leds[idexB] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void pacman() {                                  //-m24-REALLY TERRIBLE PACMAN CHOMPING EFFECT
  int s = int(LED_COUNT / 4);
  lcount++;
  if (lcount > 5) {
    lcount = 0;
  }
  if (lcount == 0) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      set_color_led(i, 255, 255, 0);
    }
  }
  if (lcount == 1 || lcount == 5) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      set_color_led(i, 255, 255, 0);
    }
    leds[s].r = 0; leds[s].g = 0; leds[s].b = 0;
  }
  if (lcount == 2 || lcount == 4) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      set_color_led(i, 255, 255, 0);
    }
    leds[s - 1].r = 0; leds[s - 1].g = 0; leds[s - 1].b = 0;
    leds[s].r = 0; leds[s].g = 0; leds[s].b = 0;
    leds[s + 1].r = 0; leds[s + 1].g = 0; leds[s + 1].b = 0;
  }
  if (lcount == 3) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      set_color_led(i, 255, 255, 0);
    }
    leds[s - 2].r = 0; leds[s - 2].g = 0; leds[s - 2].b = 0;
    leds[s - 1].r = 0; leds[s - 1].g = 0; leds[s - 1].b = 0;
    leds[s].r = 0; leds[s].g = 0; leds[s].b = 0;
    leds[s + 1].r = 0; leds[s + 1].g = 0; leds[s + 1].b = 0;
    leds[s + 2].r = 0; leds[s + 2].g = 0; leds[s + 2].b = 0;
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void random_color_pop() {                         //-m25-RANDOM COLOR POP
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  one_color_all(0, 0, 0);
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void ems_lightsSTROBE() {                  //-m26-EMERGENCY LIGHTS (STROBE LEFT/RIGHT)
  int thishue = 0;
  int thathue = (thishue + 160) % 255;
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = 0 ; i < TOP_INDEX; i++ ) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    LEDS.show(); if (safeDelay(thisdelay)) return;
    one_color_all(0, 0, 0);
    LEDS.show(); if (safeDelay(thisdelay)) return;
  }
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = TOP_INDEX ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thathue, thissat, 255);
    }
    LEDS.show(); if (safeDelay(thisdelay)) return;
    one_color_all(0, 0, 0);
    LEDS.show(); if (safeDelay(thisdelay)) return;
  }
}

void rgb_propeller() {                           //-m27-RGB PROPELLER
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, 255);
    leds[j1] = CHSV(ghue, thissat, 255);
    leds[j2] = CHSV(bhue, thissat, 255);
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void kitt() {                                     //-m28-KNIGHT INDUSTIES 2000
  int rand = random(0, TOP_INDEX);
  for (int i = 0; i < rand; i++ ) {
    leds[TOP_INDEX + i] = CHSV(thishue, thissat, 255);
    leds[TOP_INDEX - i] = CHSV(thishue, thissat, 255);
    LEDS.show();
    if (safeDelay(thisdelay / rand)) return;
  }
  for (int i = rand; i > 0; i-- ) {
    leds[TOP_INDEX + i] = CHSV(thishue, thissat, 0);
    leds[TOP_INDEX - i] = CHSV(thishue, thissat, 0);
    LEDS.show();
    if (safeDelay(thisdelay / rand)) return;
  }
}

void matrix() {                                   //-m29-ONE LINE MATRIX
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(thishue, thissat, 255);
  }
  else {
    leds[0] = CHSV(thishue, thissat, 0);
  }
  copy_led_array();
  for (int i = 1; i < LED_COUNT; i++ ) {
    leds[i].r = ledsX[i - 1][0];
    leds[i].g = ledsX[i - 1][1];
    leds[i].b = ledsX[i - 1][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void strip_march_cw() {                        //-m50-MARCH STRIP CW
  copy_led_array();
  int iCW;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    leds[i].r = ledsX[iCW][0];
    leds[i].g = ledsX[iCW][1];
    leds[i].b = ledsX[iCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void strip_march_ccw() {                        //-m51-MARCH STRIP CCW
  copy_led_array();
  int iCCW;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    leds[i].r = ledsX[iCCW][0];
    leds[i].g = ledsX[iCCW][1];
    leds[i].b = ledsX[iCCW][2];
  }
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void new_rainbow_loop() {                      //-m88-RAINBOW FADE FROM FAST_SPI2
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
  if (safeDelay(thisdelay)) return;
}

void demo_modeB() {
  int r = 10;
  one_color_all(0, 0, 0); LEDS.show();
  thisdelay = 35;
  for (int i = 0; i < r * 10; i++) {
    random_color_pop();
  }
  for (int i = 0; i < r / 2; i++) {
    ems_lightsSTROBE();
  }
  thisdelay = 50;
  for (int i = 0; i < r * 10; i++) {
    rgb_propeller();
  }
  one_color_all(0, 0, 0); LEDS.show();
  thisdelay = 100; thishue = 0;
  for (int i = 0; i < r * 3; i++) {
    kitt();
  }
  one_color_all(0, 0, 0); LEDS.show();
  thisdelay = 30; thishue = 95;
  for (int i = 0; i < r * 25; i++) {
    matrix();
  }
  one_color_all(0, 0, 0); LEDS.show();
}

void demo_modeA() {
  int r = 10;
  thisdelay = 20; thisstep = 10; thishue = 0; thissat = 255;
  one_color_all(255, 255, 255); LEDS.show(); if (safeDelay(1200)) return;
  for (int i = 0; i < r * 25; i++) {
    rainbow_fade();
  }
  for (int i = 0; i < r * 20; i++) {
    rainbow_loop();
  }
  for (int i = 0; i < r * 20; i++) {
    random_burst();
  }
  for (int i = 0; i < r * 12; i++) {
    color_bounce();
  }
  thisdelay = 40;
  for (int i = 0; i < r * 12; i++) {
    color_bounceFADE();
  }
  for (int i = 0; i < r * 6; i++) {
    ems_lightsONE();
  }
  for (int i = 0; i < r * 5; i++) {
    ems_lightsALL();
  }
  thishue = 160; thissat = 50;
  for (int i = 0; i < r * 40; i++) {
    flicker();
  }
  one_color_all(0, 0, 0); LEDS.show();
  thisdelay = 15; thishue = 0; thissat = 255;
  for (int i = 0; i < r * 50; i++) {
    pulse_one_color_all();
  }
  for (int i = 0; i < r * 40; i++) {
    pulse_one_color_all_rev();
  }
  thisdelay = 60; thishue = 180;
  for (int i = 0; i < r * 5; i++) {
    fade_vertical();
  }
  random_red();
  thisdelay = 100;
  for (int i = 0; i < r * 5; i++) {
    rule30();
  }
  thisdelay = 40;
  for (int i = 0; i < r * 8; i++) {
    random_march();
  }
  thisdelay = 80;
  for (int i = 0; i < r * 5; i++) {
    rwb_march();
  }
  one_color_all(0, 0, 0); ; LEDS.show();
  thisdelay = 60; thishue = 95;
  for (int i = 0; i < r * 15; i++) {
    radiation();
  }
  for (int i = 0; i < r * 15; i++) {
    color_loop_vardelay();
  }
  for (int i = 0; i < r * 5; i++) {
    white_temps();
  }
  thisdelay = 35; thishue = 180;
  for (int i = 0; i < r; i++) {
    sin_bright_wave();
  }
  thisdelay = 100; thishue = 0;
  for (int i = 0; i < r * 5; i++) {
    pop_horizontal();
  }
  thisdelay = 100; thishue = 180;
  for (int i = 0; i < r * 4; i++) {
    quad_bright_curve();
  }
  one_color_all(0, 0, 0); LEDS.show();
  for (int i = 0; i < r * 3; i++) {
    flame();
  }
  thisdelay = 50;
  for (int i = 0; i < r * 10; i++) {
    pacman();
  }
  thisdelay = 50; thisstep = 15;
  for (int i = 0; i < r * 12; i++) {
    rainbow_vertical();
  }
  thisdelay = 100;
  for (int i = 0; i < r * 3; i++) {
    strip_march_ccw();
  }
  for (int i = 0; i < r * 3; i++) {
    strip_march_cw();
  }
  demo_modeB();
  thisdelay = 5;
  for (int i = 0; i < r * 120; i++) {
    new_rainbow_loop();
  }
  one_color_all(255, 0, 0); LEDS.show(); if (safeDelay(1200)) return;
  one_color_all(0, 255, 0); LEDS.show(); if (safeDelay(1200)) return;
  one_color_all(0, 0, 255); LEDS.show(); if (safeDelay(1200)) return;
  one_color_all(255, 255, 0); LEDS.show(); if (safeDelay(1200)) return;
  one_color_all(0, 255, 255); LEDS.show(); if (safeDelay(1200)) return;
  one_color_all(255, 0, 255); LEDS.show(); if (safeDelay(1200)) return;
}

//-----------------------------плавное заполнение цветом-----------------------------------------
void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    setPixel(i, red, green, blue);
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
}
//-----------------------------------бегающие светодиоды-----------------------------------
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {

  for (int i = 0; i < LED_COUNT - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }

  if (safeDelay(ReturnDelay)) return;

  for (int i = LED_COUNT - EyeSize - 2; i > 0; i--) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }

  if (safeDelay(ReturnDelay)) return;
}

//---------------------------------линейный огонь-------------------------------------
void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  if (safeDelay(SpeedDelay)) return;
}

//-------------------------------newKITT---------------------------------------
void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = ((LED_COUNT - EyeSize) / 2); i >= 0; i--) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
  if (safeDelay(ReturnDelay)) return;
}

void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i <= ((LED_COUNT - EyeSize) / 2); i++) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
  if (safeDelay(ReturnDelay)) return;
}

void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i < LED_COUNT - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
  if (safeDelay(ReturnDelay)) return;
}

void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = LED_COUNT - EyeSize - 2; i > 0; i--) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
  if (safeDelay(ReturnDelay)) return;
}

void NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

//-------------------------------newKITT---------------------------------------
byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < LED_COUNT; i++) {
      c = Wheel(((i * 256 / LED_COUNT) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2));
    }
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
  }
}

//-------------------------------TwinkleRandom---------------------------------------
void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);

  for (int i = 0; i < Count; i++) {
    setPixel(random(LED_COUNT), random(0, 255), random(0, 255), random(0, 255));
    FastLED.show();
    if (safeDelay(SpeedDelay)) return;
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }

  if (safeDelay(SpeedDelay)) return;
}

//-------------------------------RunningLights---------------------------------------
void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position = 0;

  for (int i = 0; i < LED_COUNT * 2; i++)
  {
    Position++; // = 0; //Position + Rate;
    for (int i = 0; i < LED_COUNT; i++) {
      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //setPixel(i,level,0,0);
      //float level = sin(i+Position) * 127 + 128;
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255)*red,
               ((sin(i + Position) * 127 + 128) / 255)*green,
               ((sin(i + Position) * 127 + 128) / 255)*blue);
    }

    FastLED.show();
    if (safeDelay(WaveDelay)) return;
  }
}

//-------------------------------Sparkle---------------------------------------
void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(LED_COUNT);
  setPixel(Pixel, red, green, blue);
  FastLED.show();
  if (safeDelay(SpeedDelay)) return;
  setPixel(Pixel, 0, 0, 0);
}

//-------------------------------SnowSparkle---------------------------------------
void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red, green, blue);

  int Pixel = random(LED_COUNT);
  setPixel(Pixel, 0xff, 0xff, 0xff);
  FastLED.show();
  if (safeDelay(SparkleDelay)) return;
  setPixel(Pixel, red, green, blue);
  FastLED.show();
  if (safeDelay(SpeedDelay)) return;
}

//-------------------------------theaterChase---------------------------------------
void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, red, green, blue);  //turn every third pixel on
      }
      FastLED.show();
      if (safeDelay(SpeedDelay)) return;
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
      }
    }
  }
}

//-------------------------------theaterChaseRainbow---------------------------------------
void theaterChaseRainbow(int SpeedDelay) {
  byte *c;

  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        c = Wheel( (i + j) % 255);
        setPixel(i + q, *c, *(c + 1), *(c + 2)); //turn every third pixel on
      }
      FastLED.show();
      if (safeDelay(SpeedDelay)) return;
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
      }
    }
  }
}

//-------------------------------Strobe---------------------------------------
void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  for (int j = 0; j < StrobeCount; j++) {
    setAll(red, green, blue);
    FastLED.show();
    if (safeDelay(FlashDelay)) return;
    setAll(0, 0, 0);
    FastLED.show();
    if (safeDelay(FlashDelay)) return;
  }

  if (safeDelay(EndPause)) return;
}

//-------------------------------BouncingBalls---------------------------------------
void BouncingBalls(byte red, byte green, byte blue, int BallCount) {
  float Gravity = -9.81;
  int StartHeight = 1;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  for (int i = 0 ; i < BallCount ; i++) {
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i) / pow(BallCount, 2);
  }

  while (true) {
    if (changeFlag) {
      changeFlag = false;
      return;
    }
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i] / 1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i] / 1000;

      if ( Height[i] < 0 ) {
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();

        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (LED_COUNT - 1) / StartHeight);
    }

    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i], red, green, blue);
    }
    FastLED.show();
    setAll(0, 0, 0);
  }
}

//-------------------------------BouncingColoredBalls---------------------------------------
void BouncingColoredBalls(int BallCount, byte colors[][3]) {
  float Gravity = -9.81;
  int StartHeight = 1;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  for (int i = 0 ; i < BallCount ; i++) {
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i) / pow(BallCount, 2);
  }

  while (true) {
    if (changeFlag) {
      changeFlag = false;
      return;
    }
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i] / 1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i] / 1000;

      if ( Height[i] < 0 ) {
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();

        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (LED_COUNT - 1) / StartHeight);
    }

    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i], colors[i][0], colors[i][1], colors[i][2]);
    }
    FastLED.show();
    setAll(0, 0, 0);
  }
}

void initLED()
{
  LEDS.setBrightness(max_bright);  // ограничить максимальную яркость
  LEDS.addLeds<WS2811, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  one_color_all(0, 0, 0);          // погасить все светодиоды
  LEDS.show();                     // отослать команду

  randomSeed(analogRead(0));
  _BTN_CONNECTED = !digitalRead(BTN_PIN);
  if (!_BTN_CONNECTED) {
    Serial.println("button not connected!");
  }
}

void loopLED() {
  /*
    if (Serial.available() > 0) {     // если что то прислали
      ledMode = Serial.parseInt();    // парсим в тип данных int
      change_mode(ledMode);           // меняем режим через change_mode (там для каждого режима стоят цвета и задержки)
    }
  */
  switch (ledMode) {
    case 999: break;                           // пазуа
    case  2: rainbow_fade(); break;            // плавная смена цветов всей ленты
    case  3: rainbow_loop(); break;            // крутящаяся радуга
    case  4: random_burst(); break;            // случайная смена цветов
    case  5: color_bounce(); break;            // бегающий светодиод
    case  6: color_bounceFADE(); break;        // бегающий паровозик светодиодов
    case  7: ems_lightsONE(); break;           // вращаются красный и синий
    case  8: ems_lightsALL(); break;           // вращается половина красных и половина синих
    case  9: flicker(); break;                 // случайный стробоскоп
    case 10: pulse_one_color_all(); break;     // пульсация одним цветом
    case 11: pulse_one_color_all_rev(); break; // пульсация со сменой цветов
    case 12: fade_vertical(); break;           // плавная смена яркости по вертикали (для кольца)
    case 13: rule30(); break;                  // безумие красных светодиодов
    case 14: random_march(); break;            // безумие случайных цветов
    case 15: rwb_march(); break;               // белый синий красный бегут по кругу (ПАТРИОТИЗМ!)
    case 16: radiation(); break;               // пульсирует значок радиации
    case 17: color_loop_vardelay(); break;     // красный светодиод бегает по кругу
    case 18: white_temps(); break;             // бело синий градиент (?)
    case 19: sin_bright_wave(); break;         // тоже хрень какая то
    case 20: pop_horizontal(); break;          // красные вспышки спускаются вниз
    case 21: quad_bright_curve(); break;       // полумесяц
    case 22: flame(); break;                   // эффект пламени
    case 23: rainbow_vertical(); break;        // радуга в вертикаьной плоскости (кольцо)
    case 24: pacman(); break;                  // пакман
    case 25: random_color_pop(); break;        // безумие случайных вспышек
    case 26: ems_lightsSTROBE(); break;        // полицейская мигалка
    case 27: rgb_propeller(); break;           // RGB пропеллер
    case 28: kitt(); break;                    // случайные вспышки красного в вертикаьной плоскости
    case 29: matrix(); break;                  // зелёненькие бегают по кругу случайно
    case 30: new_rainbow_loop(); break;        // крутая плавная вращающаяся радуга
    case 31: strip_march_ccw(); break;         // чёт сломалось
    case 32: strip_march_cw(); break;          // чёт сломалось
    case 33: colorWipe(0x00, 0xff, 0x00, thisdelay);
      colorWipe(0x00, 0x00, 0x00, thisdelay); break;                                // плавное заполнение цветом
    case 34: CylonBounce(0xff, 0, 0, 4, 10, thisdelay); break;                      // бегающие светодиоды
    case 35: Fire(55, 120, thisdelay); break;                                       // линейный огонь
    case 36: NewKITT(0xff, 0, 0, 8, 10, thisdelay); break;                          // беготня секторов круга (не работает)
    case 37: rainbowCycle(thisdelay); break;                                        // очень плавная вращающаяся радуга
    case 38: TwinkleRandom(20, thisdelay, 1); break;                                // случайные разноцветные включения (1 - танцуют все, 0 - случайный 1 диод)
    case 39: RunningLights(0xff, 0xff, 0x00, thisdelay); break;                     // бегущие огни
    case 40: Sparkle(0xff, 0xff, 0xff, thisdelay); break;                           // случайные вспышки белого цвета
    case 41: SnowSparkle(0x10, 0x10, 0x10, thisdelay, random(100, 1000)); break;    // случайные вспышки белого цвета на белом фоне
    case 42: theaterChase(0xff, 0, 0, thisdelay); break;                            // бегущие каждые 3 (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ КРАТНО 3)
    case 43: theaterChaseRainbow(thisdelay); break;                                 // бегущие каждые 3 радуга (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ КРАТНО 3)
    case 44: Strobe(0xff, 0xff, 0xff, 10, thisdelay, 1000); break;                  // стробоскоп

    case 45: BouncingBalls(0xff, 0, 0, 3); break;                                   // прыгающие мячики
    case 46: BouncingColoredBalls(3, ballColors); break;                            // прыгающие мячики цветные

    case 888: demo_modeA(); break;             // длинное демо
    case 889: demo_modeB(); break;             // короткое демо
  }
}

void change_mode(int newmode) {
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0, 0, 0); LEDS.show(); break; //---ALL OFF
    case 1: one_color_all(255, 255, 255); LEDS.show(); break; //---ALL ON
    case 2: thisdelay = 60; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 20; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 80; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 160; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 30; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 12: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 13: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 14: thisdelay = 80; break;                     //---MARCH RANDOM COLORS
    case 15: thisdelay = 80; break;                     //---MARCH RWB COLORS
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    //---PLACEHOLDER FOR COLOR LOOP VAR DELAY VARS
    case 19: thisdelay = 35; thishue = 180; break;      //---SIN WAVE BRIGHTNESS
    case 20: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 21: thisdelay = 100; thishue = 180; break;     //---QUADRATIC BRIGHTNESS CURVE
    //---PLACEHOLDER FOR FLAME VARS
    case 23: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 24: thisdelay = 50; break;                     //---PACMAN
    case 25: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 26: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 27: thisdelay = 100; thishue = 0; break;        //---RGB PROPELLER
    case 28: thisdelay = 100; thishue = 0; break;       //---KITT
    case 29: thisdelay = 100; thishue = 95; break;       //---MATRIX RAIN
    case 30: thisdelay = 15; break;                      //---NEW RAINBOW LOOP
    case 31: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 32: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 33: thisdelay = 50; break;                     // colorWipe
    case 34: thisdelay = 50; break;                     // CylonBounce
    case 35: thisdelay = 15; break;                     // Fire
    case 36: thisdelay = 50; break;                     // NewKITT
    case 37: thisdelay = 20; break;                     // rainbowCycle
    case 38: thisdelay = 10; break;                     // rainbowTwinkle
    case 39: thisdelay = 50; break;                     // RunningLights
    case 40: thisdelay = 0; break;                      // Sparkle
    case 41: thisdelay = 30; break;                     // SnowSparkle
    case 42: thisdelay = 50; break;                     // theaterChase
    case 43: thisdelay = 50; break;                     // theaterChaseRainbow
    case 44: thisdelay = 100; break;                    // Strobe

    case 101: one_color_all(255, 0, 0); LEDS.show(); break; //---ALL RED
    case 102: one_color_all(0, 255, 0); LEDS.show(); break; //---ALL GREEN
    case 103: one_color_all(0, 0, 255); LEDS.show(); break; //---ALL BLUE
    case 104: one_color_all(255, 255, 0); LEDS.show(); break; //---ALL COLOR X
    case 105: one_color_all(0, 255, 255); LEDS.show(); break; //---ALL COLOR Y
    case 106: one_color_all(255, 0, 255); LEDS.show(); break; //---ALL COLOR Z
  }
  bouncedirection = 0;
  one_color_all(0, 0, 0);
  ledMode = newmode;
}

void setModeLED(byte m) {
  ledMode = m;   
  change_mode(ledMode);
  changeFlag = true;
}

void nextModeLED() {
  if (++modeCounter >= num_modes) modeCounter = 0;
  setModeLED(fav_modes[modeCounter]);
}

void prevModeLED() {
  if (--modeCounter < 0) modeCounter = num_modes-1;
  setModeLED(fav_modes[modeCounter]);
}

void buttonTick() {
  if (!_BTN_CONNECTED) return;

  touch.tick();
  if (touch.isSingle()) {
    nextModeLED();
  }

  if (touch.isDouble()) {
    prevModeLED();
  }

  if (touch.isHolded()) {
    Serial.println("touch.isHolded");
  }
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    return;
  }
  Serial.println("littleFS initialized");

  loadSettings();
  Serial.println("settngs loaded");
  
  initLED();
  Serial.println("initLED complete");

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

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) reconnectMqtt();
  client.loop();
  if (saveSettingsCallback) saveSettings();
  checkSensors();
  if (lightStateCallback) publishState();
  buttonTick();
  loopLED();
  delayLoop();
}