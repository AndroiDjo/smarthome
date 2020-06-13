byte minuteCounter = 0;
CHSV dawnColor;

void timeTick() {
  if (ESP_MODE == 1) {
    if (timeTimer.isReady()) {  // каждую секунду
      secs++;
      if (secs == 60) {
        secs = 0;
        mins++;
        minuteCounter++;
        updTime();
      }
      if (mins == 60) {
        mins = 0;
        hrs++;
        if (hrs == 24) {
          hrs = 0;
          days++;
          if (days > 6) days = 0;
        }
        updTime();
      }

      if (minuteCounter > 30 && WiFi.status() == WL_CONNECTED) {    // синхронизация каждые 30 минут
        minuteCounter = 0;
        if (timeClient.update()) {
          hrs = timeClient.getHours();
          mins = timeClient.getMinutes();
          secs = timeClient.getSeconds();
          days = timeClient.getDay();
        }
      }

      if (secs % 3 == 0) checkDawn();   // каждые 3 секунды проверяем рассвет
    }
  }
  if (dawnFlag && timeStrTimer.isReady()) {
    fill_solid(leds, NUM_LEDS, dawnColor);
    fillString(timeStr, CRGB::Black, false);
    delay(1);
    yield();
    FastLED.show();
  }
}

void updTime() {
  timeStr = String(hrs);
  timeStr += ":";
  timeStr += (mins < 10) ? "0" : "";
  timeStr += String(mins);
}

void checkDawn() {
  byte thisDay = days;
  if (thisDay == 0) thisDay = 7;  // воскресенье это 0
  thisDay--;
  thisTime = hrs * 60 + mins + (float)secs / 60;

  // проверка рассвета
  if (alarm[thisDay].state &&                                       // день будильника
      thisTime >= (alarm[thisDay].time - dawnOffsets[dawnMode]) &&  // позже начала
      thisTime < (alarm[thisDay].time + DAWN_TIMEOUT) ) {           // раньше конца + минута
    if (!manualOff) {
      // величина рассвета 0-255
      int dawnPosition = (float)255 * ((float)((float)thisTime - (alarm[thisDay].time - dawnOffsets[dawnMode])) / dawnOffsets[dawnMode]);
      dawnPosition = constrain(dawnPosition, 0, 255);
      dawnColor = CHSV(map(dawnPosition, 0, 255, 10, 35),
                       map(dawnPosition, 0, 255, 255, 170),
                       map(dawnPosition, 0, 255, 10, DAWN_BRIGHT));
      FastLED.setBrightness(255);
      dawnFlag = true;
    }
  } else {
    if (dawnFlag) {
      dawnFlag = false;
      manualOff = false;
      FastLED.setBrightness(modes[currentMode].brightness);
      FastLED.clear();
      FastLED.show();
    }
  }
}
 
String getTimeStampString() {
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   #ifdef ENG

   return "Date: "+ dayStr + "-" + monthStr + "-" + yearStr + ". " + "Time: " +
          hoursStr + ":" + minuteStr;
   #else

   return "Дата: "+ dayStr + "-" + monthStr + "-" + yearStr + ". " + "Время: " +
          hoursStr + ":" + minuteStr;

   #endif
}
