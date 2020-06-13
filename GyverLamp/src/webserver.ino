void webserver() {
  
  http = new(ESP8266WebServer);
  httpUpdater = new(ESP8266HTTPUpdateServer);
  httpUpdater->setup(http);
  
  if (http !=NULL) {

    http->onNotFound(routeNotFound);

    /** главная */
    http->on("/", routeHome);
    
    /** информация о модуле */
    http->on("/info", routeInfo); 

    /** прием конфигурации */
    http->on("/setconfig", routeSetConfig); 
    
    /** получить текущие настройки/конфигурацию */
    http->on("/getconfig", routeGetConfig); 
  
    /** страница настройки параметров подключения */
    http->on("/settings", routeSettings); 

    /** получить текущие настройки/конфигурацию MQTT */
    http->on("/getsettings", routeGetSettings); 

    /** отправка текущих настроек/конфигурации MQTT */
    http->on("/setsettings", routeSetSettings); 

    /** страница настройка будильника */
    http->on("/alarm", routeAlarm); 
    
    /** прием конфигурации будильника */
    http->on("/setalarmconfig", routeSetAlarmConfig); 
    
    /** получить текущие настройки/конфигурацию будильника */
    http->on("/getalarmconfig", routeGetAlarmConfig);

    /** перезазрузка лампы */
    http->on("/reboot", routeReboot);

    /** stub for favicon  **/
    http->on("/favicon.ico", []() {
      http->send(404, F("text/plain"), F("none"));
    });

    http->begin();
    
    Serial.printf("Launched web server at: http://%s.local/\r\n", clientId.c_str());
    
  } else {
    
    Serial.println("Error starting web server. \r\n");
  }
  
}

/**
 * шаблон/отправщик страницы
 */
void responseHtml(String out, String title = "AlexGyver Lamp", int code = 200) {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif
  
  String html;
  
  html = "<html>";
    html += "<head>";
      html += "<title>" + title + "</title>";
      //html += "<meta http-equiv=\"refresh\" content=\"50\" >";
      html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\" />";
      html += "<link rel=\"stylesheet\" href=\"https://demos.jquerymobile.com/1.4.5/css/themes/default/jquery.mobile-1.4.5.min.css\">";
      html += "<link rel=\"stylesheet\" href=\"http://fonts.googleapis.com/css?family=Open+Sans:300,400,700\">";
      html += "<script src=\"https://demos.jquerymobile.com/1.4.5/js/jquery.js\"></script>";
      html += "<script src=\"https://demos.jquerymobile.com/1.4.5/js/jquery.mobile-1.4.5.min.js\"></script>";
    html += "</head>";
    html += "<body>";
      html += "<div data-role='page'>";
        html += "<div data-role='header' class='jqm-header'><h2><a class='ui-link' data-ajax='false' href='/'><img style='width: 100%' src='//i0.wp.com/alexgyver.ru/wp-content/uploads/2018/07/site_mob-1.png'></a></h2></div><!-- /header -->";
        html += "<div role='main' class='ui-content jqm-content' style='padding-bottom: 100px;'>";
        html += "";
        html += out;
        html += "";
        html += "</div>";
        html += "<div data-role='footer' data-theme='b' style='position: fixed;width: 100%;bottom: 0;z-index: 1;'>";
            html += "<div data-role='navbar' data-iconpos='bottom'>";
                 #ifdef ENG
                html += "<ul>";
                    html += "<li><a href='/' data-ajax='false' data-icon='home'>Basic</a></li>"; // сдлеать активной class='ui-btn-active'
                    html += "<li><a href='/alarm' data-ajax='false' data-icon='clock'>Alarm</a></li>";
                    html += "<li><a href='/info' data-ajax='false' data-icon='info'>Info</a></li>";
                    html += "<li><a href='/settings' data-ajax='false' data-icon='gear'>MQTT</a></li>";
                html += "</ul>";
                #else  
                html += "<ul>";
                    html += "<li><a href='/' data-ajax='false' data-icon='home'>Основные</a></li>"; // сдлеать активной class='ui-btn-active'
                    html += "<li><a href='/alarm' data-ajax='false' data-icon='clock'>Будильник</a></li>";
                    html += "<li><a href='/info' data-ajax='false' data-icon='info'>Инфо</a></li>";
                    html += "<li><a href='/settings' data-ajax='false' data-icon='gear'>MQTT</a></li>";
                html += "</ul>";
                #endif
                
            html += "</div>"; // .navbar
        html += "</div>"; // .footer
      html += "</div>"; // .page

      // js функция отправки/получения данных формы
      html += "    <script type=\"text/javascript\">\n";
      html += "    function syncConfig(getconfig = '/getconfig', setconfig = '/setconfig'){\n";
      html += "        $.ajax({url: getconfig, dataType:'json', success: init});\n";
      html += "        function init(config){\n";
      html += "            window.changeReaction = false;\n";
      html += "                let timeouts = {};\n";
      html += "                $('select, input').on('change',(v) => {\n";
      html += "                    /** если данные не пришли, ничего не отправляем/сохраненяем */\n";
      html += "                    if(window.changeReaction === false) return;\n";
      html += "                    let that = $(v.currentTarget), name = that.attr('name'), outData = {};\n";
      html += "                    /** если в очереди на отправку/сохранение есть такой параметр, то снимим предыдущее значение */\n";
      html += "                    if(timeouts[name] != undefined)\n";
      html += "                        clearTimeout(timeouts[name]);\n";
      html += "                    /**\n";
      html += "                     * установим измененный параметр в очередь на отправку/сохранение в ESP\n";
      html += "                     * @type {number}\n";
      html += "                     */\n";
      html += "                    timeouts[name] = setTimeout(() => {\n";
      html += "                        window.changeReaction = false;\n";
      html += "                        outData[name] = that.val();\n";
      html += "                        $.ajax({\n";
      html += "                            url:setconfig,\n";
      html += "                            data: outData,\n";
      html += "                            dataType: 'json',\n";
      html += "                            success: (json) => {\n";
      html += "                                if(json[name] = outData[name])\n";
      html += "                                    window.changeReaction = true;\n";
      html += "                                else\n";
      html += "                                    alert('Не удалось сохранить настройки.');\n";
      html += "                                setConfig(json);\n";
      html += "                            },\n";
      html += "                            error: (event) => alert(`Не удалось сохранить настройки.\nПроизошла ошибка \"${event.status} ${event.statusText}\".`)\n";
      html += "                        });\n";
      html += "                    }, 500);\n";
      html += "                });\n";
      html += "                setConfig(config);\n";
      html += "                /** установим актуальные параметры из ESP */\n";
      html += "                function setConfig(config){\n";
      html += "                  window.changeReaction = false;\n";
      html += "                  for (let key in config){\n";
      html += "                      let that = $(`[name=${key}]`);\n";
      html += "                      that.val(config[key]);\n";
      html += "                      that.trigger('change');\n";
      html += "                  }\n";
      html += "                  /**\n";
      html += "                   * разрешаем вносить изменеия в конфигу\n";
      html += "                   * @type {boolean}\n";
      html += "                   */\n";
      html += "                  window.changeReaction = true;\n";
      html += "                }\n";
      html += "        }\n";
      html += "    }\n";
      html += "    </script>\n";
      
    html += "</body>";
  html += "</html>";
  
  http->sendHeader("Cache-Control","max-age=0, private, must-revalidate");
  http->send(code, "text/html; charset=utf-8", html); 
}

void routeReboot() {

  http->send(200, "text/html; charset=utf-8", "Reboot...");
  delay(3000);
  ESP.restart();
}

void routeInfo() {
  
  MQTTconfig MQTTConfig = readMQTTConfig();
  String out;

  out +="<hr>";
  out += "<table style=\"width:100%\">";

  out +="<tr>";
  #ifdef ENG 
  out +="<td>Wi-Fi</th>"; 
  #else 
  out +="<td>Wi-Fi сеть</th>";
  #endif

  out +="<td>                              </th>";
  out +="<td>"+WiFi.SSID()+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>ID</th>";
  #else
  out +="<td>Идентификатор</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+String(clientId)+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>WiFi signal strength</th>";
  #else
  out +="<td>Уровень WiFi сигнала</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+ String(2*(WiFi.RSSI()+100)) +"%</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Lamp IP address</th>";
  #else
  out +="<td>IP адрес лампы</th>";
  #endif
    
  out +="<td>                              </th>";
  out +="<td>"+WiFi.localIP().toString()+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Gate IP</th>";
  #else
  out +="<td>IP адрес роутера</th>";
  #endif
    
  out +="<td>                              </th>";
  out +="<td>"+WiFi.gatewayIP().toString()+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>MAC address</th>";
  #else
  out +="<td>MAC адрес</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+WiFi.macAddress()+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Wi-Fi channel</th>";
  #else
  out +="<td>Канал связи Wi-Fi</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+String(WiFi.channel())+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Lamp ID</th>";
  #else
  out +="<td>ID лампы</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getChipId(), HEX) +"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Restart Reason</th>";
  #else
  out +="<td>Причина перезагрузки</th>";
  #endif
  out +="<td>                              </th>";
  out +="<td>"+ESP.getResetReason() +"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Core version</th>";
  #else
  out +="<td>Версия ядра</th>";
  #endif
  
  out +="<td>                              </th>";
  out +="<td>"+ESP.getCoreVersion() +"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Chip frequency</th>";
  #else
  out +="<td>Частота чипа</th>";
  #endif  
  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getCpuFreqMHz())+" MHz</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Sketch size</th>";
  #else
  out +="<td>Размер скетча</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getSketchSize()/1024)+" kb</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Memory</th>";
  #else
  out +="<td>Объем памяти</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getFlashChipSize()/1024/8)+" kb</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Free memory for sketch</th>";
  #else
  out +="<td>Свободно памяти для скетча</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getFreeSketchSpace()/1024/8)+" kb</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Free RAM</th>";
  #else
  out +="<td>Свободно оперативной памяти</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+String(ESP.getFreeHeap()/1024)+" kb</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>MQTT host address</th>";
  #else
  out +="<td>Адрес MQTT сервера</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>" + String(MQTTConfig.HOST) + "</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Uptime</th>";
  #else
  out +="<td>Время непрерывной работы</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+String(uptime_formatter::getUptime())+"</th>";
  out +=" </tr>";

  out +="<tr>";
  #ifdef ENG
  out +="<td>Current date and time</th>";
  #else
  out +="<td>Текущая дата и время</th>";
  #endif  
  out +="<td>                              </th>";
  out +="<td>"+ getTimeStampString()+"</th>";
  out +=" </tr>";

  out += "</table><hr>";

  responseHtml(out);

}

/**
 * исключение/вывод ошибки о не найденном пути
 */
void routeNotFound() {
  String out;
  
  out = "Path not found";
  out += "<br />URI: ";
  out += http->uri();
  out += "<br />Method: ";
  out += (http->method() == HTTP_GET) ? "GET" : "POST";
  out += "<br />Arguments: ";
  out += http->args();
  out += "<br /><pre>";
  for (uint8_t i = 0; i < http->args(); i++) {
    out += " " + http->argName(i) + ": " + http->arg(i) + "<br />";
  }
  out += "</pre><hr /><a class='ui-link' data-ajax='false' href=\"/\">Перейти на главную</a>";
  responseHtml(out, "Error 404", 404);
}

void routeSettings(){
  String out;
  
  out = "<form>";
  
  out += "<br>";

  #ifdef ENG
  
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_host'>MQTT Host:</label>";
        out += "<input type='text' name='mqtt_host' id='mqtt_host'>";
      out += "</div>";

      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_port'>MQTT Port:</label>";
        out += "<input type='number' name='mqtt_port' id='mqtt_port'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_user'>MQTT User:</label>";
        out += "<input type='text' name='mqtt_user' id='mqtt_user'>";
      out += "</div>";
    
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_password'>MQTT Password:</label>";
        out += "<input type='password' name='mqtt_password' id='mqtt_password'>";
      out += "</div>";

      out += "<div class='ui-field-contain'>";
        USE_MQTT ? out += "<label for='mqtt_on'>Disable MQTT:</label>" : out += "<label for='mqtt_on'>Enable MQTT:</label>";
        out += "<select name='mqtt_on' id='mqtt_on' data-role='slider' data-mini='true'>";
          out += "<option value='0'>Off</option>";
          out += "<option value='1'>On</option>";
        out += "</select>";
      out += "</div>";

    #else
          
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_host'>Адрес MQTT:</label>";
        out += "<input type='text' name='mqtt_host' id='mqtt_host'>";
      out += "</div>";

      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_port'>Порт MQTT:</label>";
        out += "<input type='number' name='mqtt_port' id='mqtt_port'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_user'>Имя пользователя MQTT:</label>";
        out += "<input type='text' name='mqtt_user' id='mqtt_user'>";
      out += "</div>";
    
      out += "<div class='ui-field-contain'>";
        out += "<label for='mqtt_password'>Пароль MQTT:</label>";
        out += "<input type='password' name='mqtt_password' id='mqtt_password'>";
      out += "</div>";

      out += "<div class='ui-field-contain'>";
        USE_MQTT ? out += "<label for='mqtt_on'>Отключить  MQTT:</label>" : out += "<label for='mqtt_on'>Включить  MQTT:</label>";
        out += "<select name='mqtt_on' id='mqtt_on' data-role='slider' data-mini='true'>";
          out += "<option value='0'>Выкл</option>";
          out += "<option value='1'>Вкл</option>";
        out += "</select>";
      out += "</div>";

  #endif
  
  out += "<br>";
  
  out += "</form>";
  out += "<script type='text/javascript'>$(()=>{syncConfig('/getsettings','/setsettings');});</script>";
  out += "<br>";

  responseHtml(out);

}

// параметры подключения к MQTT

void routeGetSettings() {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif

  MQTTconfig MQTTConfig = readMQTTConfig();
  if (strlen(MQTTConfig.PORT) == 0) strcpy(MQTTConfig.PORT, "1883");
  
  String out;

  out += "{";
  out += "\"status\": \"ok\", ";
  out += "\"mqtt_on\": " + String(USE_MQTT)+ ", ";
  out += "\"mqtt_host\": \"" + String(MQTTConfig.HOST)+ "\", ";
  out += "\"mqtt_port\": " + String(MQTTConfig.PORT)+ ", ";
  out += "\"mqtt_user\": \"" + String(MQTTConfig.USER)+"\", ";
  out += "\"mqtt_password\": \"" + String(MQTTConfig.PASSWD)+"\"";
  out += "}";
  
  http->send(200, "text/json", out);
}

void routeSetSettings() {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif

  if (http->hasArg("mqtt_host")) strcpy(mqtt_server, http->arg("mqtt_host").c_str());
  if (http->hasArg("mqtt_user")) strcpy(mqtt_user, http->arg("mqtt_user").c_str());
  if (http->hasArg("mqtt_password")) strcpy(mqtt_password, http->arg("mqtt_password").c_str());
  if (http->hasArg("mqtt_port")) strcpy(mqtt_port, http->arg("mqtt_port").c_str());
  if (http->hasArg("mqtt_on")) USE_MQTT = (http->arg("mqtt_on").toInt() > 0) ? true : false;
  
  USE_MQTT ? writeMQTTConfig(mqtt_server, mqtt_user, mqtt_password, mqtt_port) : writeMQTTConfig("none", mqtt_user, mqtt_password, mqtt_port);

  /** в знак завершения операции отправим текущую конфигурацию */
  routeGetSettings();

}

/**
 * отправка текущей конфигурации 
 * + отправка JSON(обязательно должен завершаться запятой)
 */
void routeGetConfig() {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif

  String out;

  out += "{";
  out += "\"status\": \"ok\",";
  out += "\"value\": " + String(modes[currentMode].brightness) + ",";
  out += "\"currentMode\": " + String(currentMode) + ",";
  out += "\"brightness\": " + String(modes[currentMode].brightness) + ",";
  out += "\"speed\": " + String(modes[currentMode].speed) + ",";
  out += "\"scale\": " + String(modes[currentMode].scale) + ",";
  out += "\"on\": " + String(ONflag);
  out += "}";
  
  http->send(200, "text/json", out);
}

/**
 * изменение/применение новой конфигурации
 */
void routeSetConfig() {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif
    
  if (http->hasArg("currentMode")) {
    
    String value;

    value = http->arg("currentMode");
    currentMode =  value.toInt();

   if (currentMode == 28) {

      demo = true;
      currentMode = random(0, MODE_AMOUNT-1);      
    } else {

      demo = false;
      currentMode =  value.toInt();
      if (currentMode >= MODE_AMOUNT || currentMode < 0) currentMode = 0;
    }

    manualOff = true;
    dawnFlag = false;
    settChanged = true;
    saveEEPROM();
    loadingFlag = true;
    FastLED.clear();
    delay(1);
    sendCurrent();
    FastLED.setBrightness(modes[currentMode].brightness);
    
  }
  
  if(http->hasArg("scale")){

    byte scale = http->arg("scale").toInt();
    if (currentMode == 17 && scale > 100) scale = 100;
    
    modes[currentMode].scale = scale;
    loadingFlag = true;
    settChanged = true;
    eepromTimer = millis();

  }
  
  if(http->hasArg("brightness")){
    modes[currentMode].brightness = http->arg("brightness").toInt();
    FastLED.setBrightness(modes[currentMode].brightness);

    sendCurrent();
    settChanged = true;
    eepromTimer = millis();

  }
  
  if(http->hasArg("speed")){
    modes[currentMode].speed = http->arg("speed").toInt();
    loadingFlag = true;
    settChanged = true;
    eepromTimer = millis();

  }
  
  if(http->hasArg("r")){
    r = http->arg("r").toInt();

  }

  if(http->hasArg("g")){
    g = http->arg("g").toInt();

  }

  if(http->hasArg("b")){
    b = http->arg("b").toInt();

  }

  if (http->hasArg("on")) {
    
    ONflag = (http->arg("on").toInt() > 0) ? true : false;
    settChanged = true;
    changePower();
    sendCurrent();
    
  }

/** в знак завершения операции отправим текущую конфигурацию */
  routeGetConfig();
  MQTTUpdateState();
  
}

void routeAlarm(){
  #ifdef ENG
  String out, days[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  #else
  String out, days[] = {"пн","вт","ср","чт","пт","сб","вс"};
  #endif

  out = "<form>";
    for (byte i = 0; i < 7; i++) {
      out += "<div class='ui-field-contain'>";  
          out += "<label for='day_" + String(i) + "'>"+days[i]+"</label>";
          out += "<select name='day_" + String(i) + "' id='day_" + String(i) + "' data-role='slider' data-mini='true'>";
              out += "<option value='0'></option>";
              out += "<option value='1'></option>";
          out += "</select>";
      out += "</div>";
      #ifdef ENG
      out += "<div class='ui-field-contain'><label for='time_" + String(i) + "'>Time</label><input name='time_" + String(i) + "' id='time_" + String(i) + "' type='time' value='00:00' /></div>";
      #else
      out += "<div class='ui-field-contain'><label for='time_" + String(i) + "'>время</label><input name='time_" + String(i) + "' id='time_" + String(i) + "' type='time' value='00:00' /></div>";
      #endif
    }
    
    out += "<div class='ui-field-contain'>";
      #ifdef ENG
      out += "<label for='dawnMode'>Dawn for:</label>";
      #else
      out += "<label for='dawnMode'>Рассвета за:</label>";
      #endif
      out += "<select name='dawnMode' id='dawnMode'>";
      for(byte i = 0; i <= sizeof(dawnOffsets) - 1; i++){
       out += "<option value='" + String(i) + "'>" + String(dawnOffsets[i]) + "</option>"; 
      }
      out += "</select>";
    out += "</div>";
  out += "</form>";
  
  out += "<script type='text/javascript'>$(document).ready(()=>{syncConfig('/getalarmconfig','/setalarmconfig');});</script>";
  
  responseHtml(out);
}

void routeSetAlarmConfig(){
  
  for (byte i = 0; i < 7; i++) {
    
    if(http->hasArg("day_"+String(i))){
      alarm[i].state = (http->arg("day_" + String(i)).toInt() > 0);
      saveAlarm(i);
      settChanged = true;
    }
    if(http->hasArg("time_"+String(i))){
      
      alarm[i].time = http->arg("time_" + String(i)).substring(0,2).toInt() * 60 + http->arg("time_" + String(i)).substring(3,5).toInt();
      saveAlarm(i);
      settChanged = true;
    }
  }
  
  if(http->hasArg("dawnMode")){
    dawnMode = http->arg("dawnMode").toInt();
    saveDawnMmode();
    settChanged = true;
  }
  
  routeGetAlarmConfig();
  
}

void routeGetAlarmConfig() {

  #ifdef WEBAUTH
  if (!http->authenticate(clientId.c_str(), clientId.c_str())) {
      return http->requestAuthentication();
  }
  #endif
  
  String out = "{";
  int _time;
    
  for (byte i = 0; i < 7; i++) {
    out += (alarm[i].state == true) ? "\"day_"+String(i)+"\":\"1\"," : "\"day_"+String(i)+"\":\"0\",";
    if(alarm[i].time){
      String h,m;
      h = (alarm[i].time/60<10) ? "0" + String(alarm[i].time/60) : String(alarm[i].time/60);
      m = (alarm[i].time%60<10) ? "0" + String(alarm[i].time%60) : String(alarm[i].time%60);
      out += "\"time_"+String(i)+"\":\""+h+":"+m+"\",";
    }else{
      out += "\"time_"+String(i)+"\":\""+timeClient.getHours()+":"+timeClient.getMinutes()+"\",";
    }
  }

  out += "\"dawnMode\":\"" + String(dawnMode) + "\"}";
  
  http->send(200, "text/json", out);
}

/**
 * главная страница
 */
void routeHome(){
  
  String out;

  #ifdef ENG
  
  out = "<form>";

      out += "<div class='ui-field-contain'>";
        out += "<label for='on'>Power:</label>";
        out += "<select name='on' id='on' data-role='slider' data-mini='true'>";
          out += "<option value='0'>Off</option>";
          out += "<option value='1'>On</option>";
        out += "</select>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='currentMode'>Mode:</label>";
        out += "<select name='currentMode' id='currentMode' data-mini='true'>";
          
          out += "<option value='0'>Confetti</option>";
          out += "<option value='1'>Fire</option>";
          out += "<option value='2'>Rainbow vertical</option>";
          out += "<option value='3'>Rainbow horizontal</option>";
          out += "<option value='4'>Color change</option>";
          out += "<option value='5'>3D Madness</option>";
          out += "<option value='6'>3D clouds</option>";
          out += "<option value='7'>3D lava</option>";
          out += "<option value='8'>3D plasma</option>";
          out += "<option value='9'>3D rainbow</option>";
          out += "<option value='10'>3D peacock</option>";
          out += "<option value='11'>3D zebra</option>";
          out += "<option value='12'>3D forest</option>";
          out += "<option value='13'>3D ocean</option>";
          out += "<option value='14'>Color</option>";
          out += "<option value='15'>Snowfall</option>";
          out += "<option value='16'>Matrix</option>";
          out += "<option value='17'>Fireflies</option>";
          out += "<option value='18'>Aquarium</option>";
          out += "<option value='19'>Starfall</option>";
          out += "<option value='20'>Paintball</option>";
          out += "<option value='21'>Spiral</option>";
          out += "<option value='22'>Warm Light</option>";
          out += "<option value='23'>Pendulum</option>";
          out += "<option value='24'>Twinkles</option>";
          out += "<option value='25'>Police Strobo</option>";
          out += "<option value='26'>Incremental Drift Rose</option>";
          out += "<option value='27'>Pride</option>";
          out += "<option value='28'>Demo</option>";
          
        out += "</select>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='brightness'>Brightness:</label>";
        out += "<input type='range' name='brightness' id='brightness' value='50' min='1' max='255' data-highlight='true'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='speed'>Speed:</label>";
        out += "<input type='range' name='speed' id='speed' value='50' min='0' max='255' data-highlight='true'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='scale'>Scale:</label>";
        out += "<input type='range' name='scale' id='scale' value='50' min='0' max='100' data-highlight='true'>";
      out += "</div>";

  out += "</form>";
  out += "<script type='text/javascript'>$(()=>{syncConfig('/getconfig','/setconfig');});</script>";
  out += "<br>";

  #else
  
  out = "<form>";

      out += "<div class='ui-field-contain'>";
        out += "<label for='on'>Питание лампы:</label>";
        out += "<select name='on' id='on' data-role='slider' data-mini='true'>";
          out += "<option value='0'>Выкл</option>";
          out += "<option value='1'>Вкл</option>";
        out += "</select>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='currentMode'>Режим:</label>";
        out += "<select name='currentMode' id='currentMode' data-mini='true'>";
          
          out += "<option value='0'>Конфетти</option>";
          out += "<option value='1'>Огонь</option>";
          out += "<option value='2'>Радуга вертикальная</option>";
          out += "<option value='3'>Радуга горизонтальная</option>";
          out += "<option value='4'>Смена цвета</option>";
          out += "<option value='5'>Безумие 3D</option>";
          out += "<option value='6'>Облака 3D</option>";
          out += "<option value='7'>Лава 3D</option>";
          out += "<option value='8'>Плазма 3D</option>";
          out += "<option value='9'>Радуга 3D</option>";
          out += "<option value='10'>Павлин 3D</option>";
          out += "<option value='11'>Зебра 3D</option>";
          out += "<option value='12'>Лес 3D</option>";
          out += "<option value='13'>Океан 3D</option>";
          out += "<option value='14'>Цвет</option>";
          out += "<option value='15'>Снегопад</option>";
          out += "<option value='16'>Матрица</option>";
          out += "<option value='17'>Светлячки</option>";
          out += "<option value='18'>Аквариум</option>";
          out += "<option value='19'>Звездопад</option>";
          out += "<option value='20'>Пейнтбол</option>";
          out += "<option value='21'>Спираль</option>";
          out += "<option value='22'>Теплый свет</option>";
          out += "<option value='23'>Маятник</option>";
          out += "<option value='24'>Мерцание</option>";
          out += "<option value='25'>Полицейская сирена</option>";
          out += "<option value='26'>Дрейф</option>";
          out += "<option value='27'>Стая</option>";
          out += "<option value='28'>Демо</option>";
          
        out += "</select>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='brightness'>Яркость:</label>";
        out += "<input type='range' name='brightness' id='brightness' value='50' min='1' max='255' data-highlight='true'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='speed'>Скорость:</label>";
        out += "<input type='range' name='speed' id='speed' value='50' min='0' max='255' data-highlight='true'>";
      out += "</div>";
      
      out += "<div class='ui-field-contain'>";
        out += "<label for='scale'>Масштаб:</label>";
        out += "<input type='range' name='scale' id='scale' value='50' min='0' max='100' data-highlight='true'>";
      out += "</div>";
      
    
  out += "</form>";
  out += "<script type='text/javascript'>$(()=>{syncConfig('/getconfig','/setconfig');});</script>";
  out += "<br>";

  #endif

  responseHtml(out);
}
