#include "Clock.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "fonts.h"

#define FONT_NARROW 2

int screenMode = mTime;
int screenModeRequested = mTime;
int currentPriority = 255;
bool scrollFinished = true;
char msgBuf[256];   // one-time messages
char scrollBuf[256] = "- - - - -"; // retained

const char mn1[] PROGMEM = "января";
const char mn2[] PROGMEM = "февраля";
const char mn3[] PROGMEM = "марта";
const char mn4[] PROGMEM = "апреля";
const char mn5[] PROGMEM = "мая";
const char mn6[] PROGMEM = "июня";
const char mn7[] PROGMEM = "июля";
const char mn8[] PROGMEM = "августа";
const char mn9[] PROGMEM = "сентября";
const char mn10[] PROGMEM = "октября";
const char mn11[] PROGMEM = "ноября";
const char mn12[] PROGMEM = "декабря";

const char *const months[] PROGMEM = {
  mn1, mn2, mn3, mn4, mn5, mn6, mn7, mn8, mn9, mn10, mn11, mn12
};

const char day0[] PROGMEM = "Воскресенье";
const char day1[] PROGMEM = "Понедельник";
const char day2[] PROGMEM = "Вторник";
const char day3[] PROGMEM = "Среда";
const char day4[] PROGMEM = "Четверг";
const char day5[] PROGMEM = "Пятница";
const char day6[] PROGMEM = "Суббота";

const char* const days[] PROGMEM = {
  day0, day1, day2, day3, day4, day5, day6
};

MD_Parola* Panel = nullptr;

void utf8rus(const char* source, char* target, int maxLen) {
  int i = 0, k , idx =0;
  unsigned char n;
  k = strlen(source);
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
        case 0xC2: {
          n = source[i]; i++;
          if (n == 0xb0) { n = 0xf; break; } // degree sign, hack
          break;
        }
        default: {
          n = source[i]; i++;
        }
      }
    }
    target[idx++] = n;
    if (idx>=maxLen) break;
  }
  target[idx] = 0;
}

void setPanelBrightness() {
  if (Panel) {
    int br;
    if (isNight()) {
      br = cfg.getIntValue(F("panel_brightness_night"));
    } else {
      br = cfg.getIntValue(F("panel_brightness_day"));
    }
    Panel->setIntensity(br);
  }
}

void setupPanel() {
  if (Panel) {
    Panel->displayClear();
    delete Panel;
    Panel=nullptr;
  }
  int pin_din = cfg.getIntValue(F("pin_din"));
  int pin_clk = cfg.getIntValue(F("pin_clk"));
  int pin_cs = cfg.getIntValue(F("pin_cs"));
  if (!pin_din || !pin_clk || !pin_cs) {
    pin_din = D6;
    pin_clk = D8;
    pin_cs = D7;
  }
  int led_modules = cfg.getIntValue(F("led_modules"));
  if (!led_modules) led_modules = 4;
  Panel = new MD_Parola(MD_MAX72XX::FC16_HW,pin_din,pin_clk,pin_cs,led_modules);
  Panel->begin();
  Panel->setFont(RomanCyrillic);
  unregisterTimeHandler(F("brightness"));
  registerTimeHandler(F("brightness"),'h',setPanelBrightness);
}

int panelSpeed() {
  int panel_speed = cfg.getIntValue(F("panel_speed"));
  if (panel_speed>0 && panel_speed<=20) {
    panel_speed = 500/panel_speed;
  } else  {
    panel_speed = 50;
  }
  return panel_speed;
}

void drawScroll() {
  Panel->displayClear();
  Panel->setFont(RomanCyrillic);
  Panel->displayScroll(scrollBuf, PA_CENTER, PA_SCROLL_LEFT, panelSpeed());
  screenMode = mWeather;
}

void message(const char* str, int priority) {
  if (priority > currentPriority) return;
  currentPriority = priority;
  utf8rus(str, msgBuf, 255);
  Panel->displayClear();
  Panel->setFont(RomanCyrillic);
  Panel->displayScroll(msgBuf, PA_CENTER, PA_SCROLL_LEFT, panelSpeed());
  screenModeRequested = mMessage;
  screenMode = mMessage;
}

void message(const __FlashStringHelper*  str, int priority) {
  if (priority > currentPriority) return;
  currentPriority = priority;
  char buf[256];
  strncpy_P(buf, (PGM_P)str, 255);
  utf8rus(buf, msgBuf, 255);
  Panel->displayClear();
  Panel->setFont(RomanCyrillic);
  Panel->displayScroll(msgBuf, PA_CENTER, PA_SCROLL_LEFT, panelSpeed());
  screenModeRequested = mMessage;
  screenMode = mMessage;
}

void messageModal(const char* str) {
  message(str);
  while (!(Panel->displayAnimate())) { delay(10); }
}

void messageModal(const __FlashStringHelper*  str) {
  message(str);
  while (!(Panel->displayAnimate())) { delay(10); }
}

void scroll(const char* str, bool force) {
  utf8rus(str, scrollBuf, 255);
  if (force && currentPriority>5) {
    drawScroll();
    screenModeRequested = mWeather;
  }
}

const char* templateSZ PROGMEM = "%02d%c%02d%c%02d";
const char* templateSNZ PROGMEM = "%2d%c%02d%c%02d";
const char* templateMZ PROGMEM = "%02d%c%02d";
const char* templateMNZ PROGMEM = "%2d%c%02d";

void displayTime(int hh, int mi, int ss, int dw, int dd, int mm, int yy, bool dots) {
  char divider;
  char templateStr[32];
  Panel->setFont(fonts[
     (cfg.getBoolValue(F("panel_seconds")) && (cfg.getIntValue(F("led_modules"))<6))
     ?FONT_NARROW:cfg.getIntValue(F("panel_font")) ]);
  if (screenMode !=mTime) {
    screenModeRequested = mTime;
    screenMode = mTime;
  }
  if (dots || !cfg.getBoolValue(F("flash_dots"))) {
    divider = ':';
  } else {
    divider = ' ';
  }
  if (cfg.getBoolValue(F("panel_seconds"))) {
    if (cfg.getBoolValue(F("panel_zero"))) {
      strcpy_P(templateStr,templateSZ);
    } else {
      strcpy_P(templateStr,templateSNZ);
    }
  } else {
    if (cfg.getBoolValue(F("panel_zero"))) {
      strcpy_P(templateStr,templateMZ);
    } else {
      strcpy_P(templateStr,templateMNZ);
    }
  }
  snprintf(msgBuf, 255, templateStr, hh, divider, mi, divider, ss);
  Panel->displayText(msgBuf, PA_CENTER,100,0,PA_NO_EFFECT,PA_NO_EFFECT);
  Panel->displayAnimate();
  scrollFinished = true;
}

void displayTime() {

  static bool newsec = false;
  static bool newsec06 = false;
  static int  prevss;
  static unsigned long prevmilli;

  if (ss != prevss) {
    prevmilli=millis();
    prevss = ss;
    newsec = true;
    newsec06 = true;
  }

  if (newsec) {
    displayTime(hh,mi,ss,dw,dd,mm,yy,true);
    newsec = false;
  } else {
    int delta = millis() - prevmilli;
    if (delta>600 && newsec06) {
      displayTime(hh,mi,ss,dw,dd,mm,yy,false);
      newsec06 = false;
    }
  }

}

void displayDate(int dd, int mm, int yy) {

  String msg;

  if (screenMode !=mDate) {
    screenMode = mDate;

    PGM_P day = days[dw];
    PGM_P month = months[mm];

    sprintf(msgBuf,"%s, %02d %s %04d г",day, dd, month, yy);
    utf8rus(msgBuf, msgBuf, 255);
    Panel->displayClear();
    Panel->setFont(RomanCyrillic);

    Panel->displayScroll(msgBuf, PA_CENTER, PA_SCROLL_LEFT, panelSpeed());
  }
}

void displayDate() {
  displayDate(dd,mm,yy);
}

void tickPanel() {
  if (screenMode == mTime || screenModeRequested != screenMode) { // needs to redraw
    if (now<1668332133) { message(F("Время еще неизвестно...")); }
    switch (screenModeRequested) {
      case mDefault:
      case mBarrier:
      case mLast:
      case mTime:
        displayTime();
        break;
      case mDate:
        displayDate();
        break;
      case mWeather:
        drawScroll();
        break;
      case mMessage:
        break;
    }
  } else if (Panel->displayAnimate()) {
    screenModeRequested = mTime;
    currentPriority = 255;
  }
}


