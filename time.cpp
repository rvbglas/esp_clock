#include "Clock.h"
#include <time.h>
#include <coredecls.h>

bool isTimeSet = false;
time_t now;
time_t last_sync;

int hh;
int mi;
int ss;

int dw;

int dd;
int mm;
int yy;

#define maxTimeHandlers 8
const char* tHandlerNames[maxTimeHandlers];
char tHandlerTypes[maxTimeHandlers];
std::function<void()> tTimeHandlers[maxTimeHandlers];

void timeIsSet(bool ntp) {
  if (ntp) {
    Serial.println(F("Время синхронизировано"));
    message(F("Время синхронизировано"));
    reportMessage(F("Время синхронизировано"));
    if (isRTCEnabled) {
      RTC.adjust(DateTime(now));
    }
  }
  isTimeSet = true;
  last_sync = now;
}

void setupHandlers() {
  for (int i=0; i<maxTimeHandlers; i++) {
    tHandlerNames[i] = nullptr;
    tHandlerTypes[i] = ' ';
    tTimeHandlers[i] = nullptr;
  }
}

void setupTime() {
  configTime(cfg.getCharValue(F("tz")),cfg.getCharValue(F("ntp_server")));
  settimeofday_cb(&timeIsSet);
}

void registerTimeHandler(const char* handlerName, const char handlerType, std::function<void()> timeHandler) {
  for (int i=0; i<maxTimeHandlers; i++) {
    if (!tHandlerNames[i]) {
      // empty slot found!
      tHandlerNames[i] = handlerName;
      tHandlerTypes[i] = handlerType;
      tTimeHandlers[i] = timeHandler;
      break;
    }
  }
}

void registerTimeHandler(const __FlashStringHelper* handlerName, const char handlerType, std::function<void()> timeHandler) {
  for (int i=0; i<maxTimeHandlers; i++) {
    if (!tHandlerNames[i]) {
      // empty slot found!
      tHandlerNames[i] = copystr(handlerName);
      tHandlerTypes[i] = handlerType;
      tTimeHandlers[i] = timeHandler;
      break;
    }
  }
}

void unregisterTimeHandler(const char* handlerName) {
  for (int i=0; i<maxTimeHandlers; i++) {
    if (tHandlerNames[i] && strcmp(tHandlerNames[i],handlerName) == 0) {
      tHandlerNames[i] = nullptr;
      tHandlerTypes[i] = ' ';
      tTimeHandlers[i] = nullptr;
      break;
    }
  }
}

void unregisterTimeHandler(const __FlashStringHelper* handlerName) {
  for (int i=0; i<maxTimeHandlers; i++) {
    if (tHandlerNames[i] && strcmp_P(tHandlerNames[i],(PGM_P)handlerName) == 0) {
      tHandlerNames[i] = nullptr;
      tHandlerTypes[i] = ' ';
      tTimeHandlers[i] = nullptr;
      break;
    }
  }
}

void runHandlers(char handlerType) {
  for (int i=0; i<maxTimeHandlers; i++) {
    if (tTimeHandlers[i] && tHandlerTypes[i]==handlerType) {
      tTimeHandlers[i]();
    }
  }
}

void tickTime() {
  static int prevD = 0, prevH = 0, prevM = 0, prevS = 0;
  time(&now);
  struct tm* timeinfo = localtime(&now);  
  hh = timeinfo->tm_hour;
  mi = timeinfo->tm_min;
  ss = timeinfo->tm_sec;
  dw = timeinfo->tm_wday;
  dd = timeinfo->tm_mday;
  mm = timeinfo->tm_mon+1;
  yy = timeinfo->tm_year+1900;
  if (dd != prevD) {
    prevD = dd;
    runHandlers('d');
  }
  if (hh != prevH) {
    prevH = hh;
    runHandlers('h');
  }
  if (mi != prevM) {
    prevM = mi;
    runHandlers('m');
  }
  if (ss != prevS) {
    prevS = ss;
    runHandlers('s');
  }
}

bool isNight() {
  int day_from = cfg.getIntValue(F("day_from"));
  int night_from = cfg.getIntValue(F("night_from"));
  if (day_from<night_from) { // night ... day ... night
     return hh<day_from || hh>=night_from;
  } else { /// late day ... night .... day till mindnight
     return (hh>=night_from && hh<day_from);
  }
}

