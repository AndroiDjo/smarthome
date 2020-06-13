
MQTTconfig readMQTTConfig () {
  int eeAddress = 300;
  MQTTconfig MQTTConfig;

  for (int i = 0; i < 32; ++i) MQTTConfig.HOST[i]   = char(EEPROM.read(eeAddress + i)); eeAddress += 32;
  for (int i = 0; i < 32; ++i) MQTTConfig.USER[i]   = char(EEPROM.read(eeAddress + i)); eeAddress += 32;
  for (int i = 0; i < 32; ++i) MQTTConfig.PASSWD[i] = char(EEPROM.read(eeAddress + i)); eeAddress += 32;
  for (int i = 0; i < 10; ++i) MQTTConfig.PORT[i] = char(EEPROM.read(eeAddress + i)); 

  return MQTTConfig;
}

void writeMQTTConfig(const char HOST[32], const char USER[32], const char PASSWD[32], const char PORT[10]) {
  int eeAddress = 300;

  for (int i = 0; i < 32; ++i)  EEPROM.write(eeAddress + i, HOST[i]); eeAddress += 32;
  for (int i = 0; i < 32; ++i)  EEPROM.write(eeAddress + i, USER[i]); eeAddress += 32;
  for (int i = 0; i < 32; ++i)  EEPROM.write(eeAddress + i, PASSWD[i]); eeAddress += 32;
  for (int i = 0; i < 10; ++i)  EEPROM.write(eeAddress + i, PORT[i]); 

  EEPROM.commit();
}

int Get_EFFIDX (String effect) {

  #ifdef ENG

  if (effect.equals("Confetti")) return 0;
  if (effect.equals("Fire")) return 1;
  if (effect.equals("Rainbow vert")) return 2;
  if (effect.equals("Rainbow Horiz")) return 3;
  if (effect.equals("Color change")) return 4;
  if (effect.equals("3D Madness")) return 5;
  if (effect.equals("3D clouds")) return 6;
  if (effect.equals("3D lava")) return 7;
  if (effect.equals("3D plasma")) return 8;
  if (effect.equals("3D rainbow")) return 9;
  if (effect.equals("3D peacock")) return 10;
  if (effect.equals("3D zebra")) return 11;
  if (effect.equals("3D forest")) return 12;
  if (effect.equals("3D ocean")) return 13;
  if (effect.equals("Color")) return 14;
  if (effect.equals("Snowfall")) return 15;
  if (effect.equals("Matrix")) return 16;
  if (effect.equals("Fireflies")) return 17;
  if (effect.equals("Aquarium")) return 18;
  if (effect.equals("Starfall")) return 19;
  if (effect.equals("Paintball")) return 20;
  if (effect.equals("Spiral")) return 21;
  if (effect.equals("Warm Light")) return 22;
  if (effect.equals("Pendulum")) return 23;
  if (effect.equals("Twinkles")) return 24;
  if (effect.equals("Police Strobo")) return 25;
  if (effect.equals("Incremental Drift Rose")) return 26;
  if (effect.equals("Pride")) return 27;
  if (effect.equals("Dемо")) return 28;

  #else

  if (effect.equals("Конфетти")) return 0;
  if (effect.equals("Огонь")) return 1;
  if (effect.equals("Радуга верт.")) return 2;
  if (effect.equals("Радуга гориз.")) return 3;
  if (effect.equals("Смена цвета")) return 4;
  if (effect.equals("Безумие 3D")) return 5;
  if (effect.equals("Облака 3D")) return 6;
  if (effect.equals("Лава 3D")) return 7;
  if (effect.equals("Плазма 3D")) return 8;
  if (effect.equals("Радуга 3D")) return 9;
  if (effect.equals("Павлин 3D")) return 10;
  if (effect.equals("Зебра 3D")) return 11;
  if (effect.equals("Лес 3D")) return 12;
  if (effect.equals("Океан 3D")) return 13;
  if (effect.equals("Цвет")) return 14;
  if (effect.equals("Снегопад")) return 15;
  if (effect.equals("Матрица")) return 16;
  if (effect.equals("Светлячки")) return 17;
  if (effect.equals("Аквариум")) return 18;
  if (effect.equals("Звездопад")) return 19;
  if (effect.equals("Пейнтбол")) return 20;
  if (effect.equals("Спираль")) return 21;
  if (effect.equals("Теплый свет")) return 22;
  if (effect.equals("Маятник")) return 23;
  if (effect.equals("Мерцание")) return 24;
  if (effect.equals("Полицейская сирена")) return 25;
  if (effect.equals("Дрейф")) return 26;
  if (effect.equals("Стая")) return 27;
  if (effect.equals("Демо")) return 28;

  #endif

}

String Get_EFFName (int eff_idx) {

  #ifdef ENG

  switch (eff_idx) {
    case 0: return "Confetti";
    case 1: return "Fire";
    case 2: return "Rainbow vert";
    case 3: return "Rainbow Horiz";
    case 4: return "Color change";
    case 5: return "3D Madness";
    case 6: return "3D clouds";
    case 7: return "3D lava";
    case 8: return "3D plasma";
    case 9: return "3D rainbow";
    case 10: return "3D peacock";
    case 11: return "3D zebra";
    case 12: return "3D forest";
    case 13: return "3D ocean";
    case 14: return "Color";
    case 15: return "Snowfall";
    case 16: return "Matrix";
    case 17: return "Fireflies";
    case 18: return "Aquarium";
    case 19: return "Starfall";
    case 20: return "Paintball";
    case 21: return "Spiral";
    case 22: return "Warm Light";
    case 23: return "Pendulum";
    case 24: return "Twinkles";
    case 25: return "Police Strobo";
    case 26: return "Incremental Drift Rose";
    case 27: return "Pride";
    case 28: return "Demo";
  }

  #else

  switch (eff_idx) {
    case 0: return "Конфетти";
    case 1: return "Огонь";
    case 2: return "Радуга верт.";
    case 3: return "Радуга гориз.";
    case 4: return "Смена цвета";
    case 5: return "Безумие 3D";
    case 6: return "Облака 3D";
    case 7: return "Лава 3D";
    case 8: return "Плазма 3D";
    case 9: return "Радуга 3D";
    case 10: return "Павлин 3D";
    case 11: return "Зебра 3D";
    case 12: return "Лес 3D";
    case 13: return "Океан 3D";
    case 14: return "Цвет";
    case 15: return "Снегопад";
    case 16: return "Матрица";
    case 17: return "Светлячки";
    case 18: return "Аквариум";
    case 19: return "Звездопад";
    case 20: return "Пейнтбол";
    case 21: return "Спираль";
    case 22: return "Теплый свет";
    case 23: return "Маятник";
    case 24: return "Мерцание";
    case 25: return "Полицейская сирена";
    case 26: return "Дрейф";
    case 27: return "Стая";
    case 28: return "Демо";
  }

  #endif

}

void MQTTUpdateState () {
  
   mqttclient.publish(String("homeassistant/light/"+clientId+"/status").c_str(), ONflag ? "ON" : "OFF", true);
   mqttclient.publish(String("homeassistant/light/"+clientId+"/brightness/status").c_str(), String(modes[currentMode].brightness).c_str(), true);
   mqttclient.publish(String("homeassistant/light/"+clientId+"/effect/status").c_str(), Get_EFFName(currentMode).c_str(), true);
   mqttclient.publish(String("homeassistant/light/"+clientId+"/effect/speed/status").c_str(), String(modes[currentMode].speed).c_str(), true);
   mqttclient.publish(String("homeassistant/light/"+clientId+"/effect/scale/status").c_str(), String(modes[currentMode].scale).c_str(), true);

   char sRGB[15];
   sprintf(sRGB, "%d,%d,%d", r, g, b);
   mqttclient.publish(String("homeassistant/light/"+clientId+"/rgb/status").c_str(), sRGB, true);

}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  String Payload = "";

  #ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  #endif

  for (int i = 0; i < length; i++) Payload += (char)payload[i];
  #ifdef DEBUG
  Serial.println(Payload);
  #endif

  if (String(topic) == "homeassistant/light/"+clientId+"/switch") {

      Serial.print("Command arrived: ");
      Serial.println(Payload);

      ONflag = (Payload == "ON") ? true : false;
      changePower();
      sendCurrent();
      MQTTUpdateState();
      
  }

  if (String(topic) == "homeassistant/light/"+clientId+"/brightness/set") {

      Serial.print("Command arrived: brightness "); Serial.println(Payload);

      modes[currentMode].brightness = Payload.toInt();
      FastLED.setBrightness(modes[currentMode].brightness);
      saveEEPROM();
      settChanged = true;
      eepromTimer = millis();
      MQTTUpdateState();

  }

  if (String(topic) == "homeassistant/light/"+clientId+"/effect/set") {

      Serial.print("Command arrived: effect set to "); Serial.println(Payload);

      if (Payload == "Демо") { 

          demo = true; 
          currentMode = random(0, MODE_AMOUNT-1);

          if (!epilepsy) {
          while (currentMode == 4) currentMode = random(0, MODE_AMOUNT-1);          
        }
       } else {
      
          demo = false;
          currentMode = Get_EFFIDX(Payload);
       }

      saveEEPROM();
      loadingFlag = true;
      FastLED.clear();
      delay(1);
      sendCurrent();
      FastLED.setBrightness(modes[currentMode].brightness);
      MQTTUpdateState();
      
  }

  if (String(topic) == "homeassistant/light/"+clientId+"/rgb/set") {

      Serial.print("Command arrived: rgb "); Serial.println(Payload);
      
      r = getValue(Payload, ',', 0).toInt();
      g = getValue(Payload, ',', 1).toInt();
      b = getValue(Payload, ',', 2).toInt();

      currentMode = 14;
      saveEEPROM();
      loadingFlag = true;
      delay(1);
      sendCurrent();
      FastLED.setBrightness(modes[currentMode].brightness);
      MQTTUpdateState();

  }

  if (String(topic) == "homeassistant/light/"+clientId+"/effect/speed/set") {

      Serial.print("Command arrived: speed "); Serial.println(Payload);

      modes[currentMode].speed = Payload.toInt();
      saveEEPROM();
      loadingFlag = true;
      settChanged = true;
      eepromTimer = millis();
      MQTTUpdateState();

  }

  if (String(topic) == "homeassistant/light/"+clientId+"/effect/scale/set") {

      Serial.print("Command arrived: scale "); Serial.println(Payload);

      if (currentMode == 17 && Payload.toInt() > 100) Payload = "100";
      
      modes[currentMode].scale = Payload.toInt();
      saveEEPROM();
      loadingFlag = true;
      settChanged = true;
      eepromTimer = millis();
      MQTTUpdateState();

  }

}

uint32_t timing = 0;
uint32_t mqtt_timeout = 5000;
uint8_t mqtt_reconnection_count = 0;

void MQTTreconnect() {
  if (USE_MQTT && (millis() - timing > mqtt_timeout)) {

      MQTTconfig MQTTConfig = readMQTTConfig();

      mqttclient.setServer(MQTTConfig.HOST, String(MQTTConfig.PORT).toInt());
      mqttclient.setCallback(MQTTcallback);

      if ((millis() - timing > mqtt_timeout) && !mqttclient.connected()) {

          timing = millis();
          
          if (!WiFi.isConnected()) WiFi.reconnect();
      
          Serial.printf("Attempting MQTT connection to %s on port %s as %s...", MQTTConfig.HOST, MQTTConfig.PORT, MQTTConfig.USER);

          // подключаемся к MQTT серверу
          if (mqttclient.connect(clientId.c_str(), MQTTConfig.USER, MQTTConfig.PASSWD, String("homeassistant/light/"+clientId+"/state").c_str(), 0,  true, "offline")) {
          
            Serial.println("connected!");
            mqttclient.publish(String("homeassistant/light/"+clientId+"/state").c_str(), "online", true);

            mqtt_timeout = 5000;
            mqtt_reconnection_count = 0;

            #ifdef DEBUG
            //mqttclient.subscribe(String("homeassistant/light/"+clientId+"/config").c_str());
            #endif

            HomeAssistantSendDiscoverConfig();

            // подписываемся на топики
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/switch").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/brightness/set").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/effect/set").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/rgb/set").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/effect/speed/set").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/effect/scale/set").c_str());
            mqttclient.subscribe(String("homeassistant/light/"+clientId+"/state").c_str());

            MQTTUpdateState();
          
        } else {

          mqtt_reconnection_count += 1;
          mqtt_timeout *= 2;

          if (mqtt_reconnection_count >= 9) {

            Serial.println("Сan not establish a connection, resetting ESP...");
            ESP.restart(); //ESP.reset();

            //mqtt_timeout = 5000;
            //mqtt_reconnection_count = 0;
            
          }
          
          Serial.print("failed, rc=");
          Serial.print(mqttclient.state());
          Serial.printf(" try again in %d seconds\n", mqtt_timeout/1000);
        }
      }
    }
  }

void HomeAssistantSendDiscoverConfig() {

  DynamicJsonDocument hass_discover(1024);
  
  hass_discover["~"] = "homeassistant/light/"+clientId;
  hass_discover["name"] = "Gyver Lamp "+ clientId; // name
  hass_discover["uniq_id"] = String(ESP.getChipId(), HEX); // unique_id
  hass_discover["avty_t"] = "~/state";         // availability_topic
  hass_discover["pl_avail"] = "online";        // payload_available
  hass_discover["pl_not_avail"] = "offline";   // payload_not_available

  hass_discover["bri_cmd_t"] = "~/brightness/set";     // brightness_command_topic
  hass_discover["bri_stat_t"] = "~/brightness/status"; // brightness_state_topic
  hass_discover["bri_scl"] = 255;

  hass_discover["cmd_t"] = "~/switch"; // command_topic
  hass_discover["stat_t"] = "~/status"; // state_topic
  
  hass_discover["fx_cmd_t"] = "~/effect/set";     // effect_command_topic
  hass_discover["fx_stat_t"] = "~/effect/status"; // effect_state_topic

  hass_discover["rgb_cmd_t"] = "~/rgb/set";     // rgb_command_topic
  hass_discover["rgb_stat_t"] = "~/rgb/status"; // rgb_state_topic

  String hass_discover_str;
  serializeJson(hass_discover, hass_discover_str);

  #ifdef ENG
  const char eff_list[] = R"=====(, "fx_list": ["Confetti", "Fire", "Rainbow vert", "Rainbow Horiz", "Color change", "3D Madness", "3D clouds", "3D lava", "3D plasma", "3D rainbow", "3D peacock", "3D zebra", "3D forest", "3D ocean", "Color", "Snowfall", "Matrix", "Fireflies",  "Aquarium", "Starfall", "Paintball", "Spiral", "Warm Light", "Pendulum", "Twinkles", "Police Strobo", "Incremental Drift Rose", "Pride", "Demo"] })=====";  // effect_list
  #else
  const char eff_list[] = R"=====(, "fx_list": ["Конфетти", "Огонь", "Радуга верт.", "Радуга гориз.", "Смена цвета", "Безумие 3D", "Облака 3D", "Лава 3D", "Плазма 3D", "Радуга 3D", "Павлин 3D", "Зебра 3D", "Лес 3D", "Океан 3D", "Цвет", "Снегопад", "Матрица", "Светлячки",  "Аквариум", "Звездопад", "Пейнтбол", "Спираль", "Теплый свет", "Маятник", "Мерцание", "Полицейская сирена", "Дрейф", "Стая", "Демо"] })=====";  // effect_list
  #endif
  const char dev_reg_tpl[] = R"=====(, "device": {"ids": ["%s"], "name": "Gyver Lamp", "mf": "Alex Gyver", "mdl": "Gyver Lamp v2", "sw": "1.5.5 MQTT"})=====";  // device reg
  char dev_reg[256];

  sprintf(dev_reg, dev_reg_tpl, String(ESP.getChipId(), HEX).c_str());
  hass_discover_str = hass_discover_str.substring(0, hass_discover_str.length() - 1);

  hass_discover_str += dev_reg;
  hass_discover_str += eff_list;

  #ifdef DEBUG
  //Serial.println(hass_discover_str);
  //mqttclient.publish(String("homeassistant/light/"+clientId+"/config").c_str(), "");
  #endif

  if (mqttclient.beginPublish(String("homeassistant/light/"+clientId+"/config").c_str(), hass_discover_str.length(), true)) {

    mqttclient.print(hass_discover_str.c_str());
    mqttclient.endPublish() ? Serial.println("Success sent discover message") : Serial.println("Field to send discover message");
    
  } else {
 
   Serial.println("Field to start discover message"); 
  }
  
  // уровень wifi сигнала
  DynamicJsonDocument hass_discover_signal_sensor(1024);
  String hass_discover_signal_sensor_str;

  hass_discover_signal_sensor["~"] = "homeassistant/sensor/"+clientId+"W";
  hass_discover_signal_sensor["device_class"] = "signal_strength";
  hass_discover_signal_sensor["name"] = "Signal Strength "+clientId;
  hass_discover_signal_sensor["state_topic"] = "~/WiFi/RSSI_pct";        
  hass_discover_signal_sensor["unit_of_measurement"] = "%"; 
  hass_discover_signal_sensor["uniq_id"] = "W"+String(ESP.getChipId(), HEX); // unique_id
  hass_discover_signal_sensor["avty_t"] = String("homeassistant/light/"+clientId+"/state");  // availability_topic
  hass_discover_signal_sensor["pl_avail"] = "online";        // payload_available
  hass_discover_signal_sensor["pl_not_avail"] = "offline";   // payload_not_available

  serializeJson(hass_discover_signal_sensor, hass_discover_signal_sensor_str);

  const char dev_reg_tpl_s[] = R"=====(, "device": {"ids": ["%s"]} })=====";  // device reg
  char dev_reg_s[256];

  sprintf(dev_reg_s, dev_reg_tpl_s, String(ESP.getChipId(), HEX).c_str());
  hass_discover_signal_sensor_str = hass_discover_signal_sensor_str.substring(0, hass_discover_signal_sensor_str.length() - 1);

  hass_discover_signal_sensor_str += dev_reg_s;

  if (mqttclient.beginPublish(String("homeassistant/sensor/"+clientId+"W/config").c_str(), hass_discover_signal_sensor_str.length(), true)) {

    mqttclient.print(hass_discover_signal_sensor_str.c_str());
    mqttclient.endPublish() ? Serial.println("Success sent WiFi signal discover message") : Serial.println("Field to send WiFi signal discover message");
    
  } else {
 
   Serial.println("Field to start WiFi signal discover message"); 
  }

  // Время непрерывной работы
  DynamicJsonDocument hass_discover_uptime_sensor(1024);
  String hass_discover_uptime_sensor_str;

  hass_discover_uptime_sensor["~"] = "homeassistant/sensor/"+clientId+"U";
  hass_discover_uptime_sensor["ic"] = "mdi:timer";
  hass_discover_uptime_sensor["name"] = "Uptime "+clientId; 
  hass_discover_uptime_sensor["state_topic"] = "~/uptime";        
  hass_discover_uptime_sensor["unit_of_measurement"] = "s"; 
  hass_discover_uptime_sensor["uniq_id"] = "U"+String(ESP.getChipId(), HEX); // unique_id
  hass_discover_uptime_sensor["avty_t"] = String("homeassistant/light/"+clientId+"/state");   // availability_topic
  hass_discover_uptime_sensor["pl_avail"] = "online";        // payload_available
  hass_discover_uptime_sensor["pl_not_avail"] = "offline";   // payload_not_available

  serializeJson(hass_discover_uptime_sensor, hass_discover_uptime_sensor_str);

  hass_discover_uptime_sensor_str = hass_discover_uptime_sensor_str.substring(0, hass_discover_uptime_sensor_str.length() - 1);
  hass_discover_uptime_sensor_str += dev_reg_s;

  if (mqttclient.beginPublish(String("homeassistant/sensor/"+clientId+"U/config").c_str(), hass_discover_uptime_sensor_str.length(), true)) {

    mqttclient.print(hass_discover_uptime_sensor_str.c_str());
    mqttclient.endPublish() ? Serial.println("Success sent Uptime signal discover message") : Serial.println("Field to send Uptime signal discover message");
    
  } else {
 
   Serial.println("Field to start Uptime signal discover message"); 
  }

}

void infoCallback() {
    mqttclient.publish(String("homeassistant/sensor/"+clientId+"U/uptime").c_str(), String(millis()/1000).c_str(), true);
    mqttclient.publish(String("homeassistant/sensor/"+clientId+"W/WiFi/RSSI").c_str(), String(WiFi.RSSI()).c_str(), true);    
    mqttclient.publish(String("homeassistant/sensor/"+clientId+"W/WiFi/RSSI_pct").c_str(), String(2*(WiFi.RSSI()+100)).c_str(), true);    
    mqttclient.publish(String("homeassistant/sensor/"+clientId+"W/WiFi/channel").c_str(), String(WiFi.channel()).c_str(), true);    
    mqttclient.publish(String("homeassistant/light/"+clientId+"/ResetReason").c_str(), String(ESP.getResetReason()).c_str(), true);    
    mqttclient.publish(String("homeassistant/sensor/"+clientId+"/VCC").c_str(), String((float)ESP.getVcc()/1000.0f).c_str(), true);
    mqttclient.publish(String("homeassistant/light/"+clientId+"/BootCount").c_str(), String(boot_count).c_str(), true);    
    mqttclient.publish(String("homeassistant/light/"+clientId+"/DemoMode").c_str(), String(demo).c_str(), true);

    if (boot_count > 1) { boot_count = 0; EEPROM.write(410, boot_count); EEPROM.commit(); }

}

void demoCallback() {

    if (demo) { 

      currentMode = random(0, MODE_AMOUNT-1);
      if (!epilepsy) {
          while (currentMode == 4) currentMode = random(0, MODE_AMOUNT-1);          
        }
      
      loadingFlag = true;
      FastLED.clear();
      delay(1);
      sendCurrent();
      FastLED.setBrightness(modes[currentMode].brightness);
      MQTTUpdateState();
   } 

}
