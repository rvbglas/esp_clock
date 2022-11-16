#include "Clock.h"
#include <Wire.h>
#include "time.h"
#include <Button2.h>
#include <Arduino.h>
#include <coredecls.h>

RTC_DS3231 RTC;
bool isRTCEnabled = false;

Button2 btn;
bool isButtonEnabled = false;

bool isBuzzerEnabled = false;
int buzzer_pin;
bool buzzer_passive;

bool beepState = false;
bool beepStateRequested = false;

int beepToneRequested;
int beepLengthRequested;

int beepMs = 400;
int silentMs = 200;
int beepTone = 1000;

void beep(int tone, int length, int beep, int silent) {
  beepStateRequested = true;
  beepToneRequested = tone;
  beepLengthRequested = length;
  if (!silent && beep>length) {
    beepMs = length; 
  } else { 
    beepMs = beep; 
  }
  silentMs = silent;
}

unsigned long first_click_millis = 0;

void buttonHandler(Button2& btn) {
  if (!isButtonEnabled) return;
  if (beepStateRequested) {
    beepStateRequested = false;
    return;
  }
  clickType click = btn.read();
  Serial.println(btn.clickToString(click));
  switch (click) {
    case single_click:
      screenModeRequested++;
      if (screenModeRequested == mBarrier) { screenModeRequested = mDefault; }
      if (screenModeRequested == mLast) { screenModeRequested = mDefault; }
      break;
    case double_click:
      screenModeRequested--;
      if (screenModeRequested <= mDefault) { screenModeRequested = mBarrier - 1; }
      break;
    case triple_click:
      static int click_counter = 0;
      if (millis() - first_click_millis > 6000) {
        first_click_millis = millis();
        click_counter = 1;
        beep(400,800,200,100);
        message(F("Длинное - точка доступа, тройное - сброс"));
      } else {
        message(F("Еще раз для сброса"));
        beep(800,800,200,100);
        click_counter++;
        if (click_counter>2) {
          tone(1200,1000);
          Serial.println(F("Три тройных нажатия - сбрасываю конфигурацию"));
          reset();
        }
      }
      break;
  }
}

void buttonLongClickHandler(Button2& btn) {
  if (!isButtonEnabled) return;
  if (beepStateRequested) {
    beepStateRequested = false;
    return;
  }
  if (millis() - first_click_millis < 6000) {
    if (isApEnabled) {    
      setupNet(false);
    } else {
      setupNet(true);
    }
    return;
  }
  bool enable_alarm = cfg.getBoolValue(F("enable_alarm"));
  enable_alarm = !enable_alarm;
  cfg.setValue(F("enable_alarm"), enable_alarm);
  Serial.println(F("Alarm = ")); Serial.println(enable_alarm);
  if (enable_alarm) {
    message(F("Будильник включен"));
    reportChange(F("enable_alarm"));    
  } else {
    message(F("Будильник выключен"));
    reportChange(F("enable_alarm"));
  }

}

void setupHardware() {
  int pin_sda = cfg.getIntValue(F("pin_sda"));
  int pin_scl = cfg.getIntValue(F("pin_scl"));
  if (pin_sda && pin_scl && cfg.getBoolValue(F("enable_rtc"))) {
    Serial.println(F("Нестандартные пины i2c"));
    Wire.begin(pin_sda,pin_scl);
  }
  int i2c_speed = cfg.getIntValue(F("i2c_speed"));
  if (i2c_speed) Wire.setClock(i2c_speed);
  if (cfg.getBoolValue(F("enable_rtc"))) {
    RTC.begin();
    time_t rtc = RTC.now().unixtime();
    timeval tv = { rtc, 0 };
    settimeofday(&tv, nullptr);  
    isRTCEnabled = true;
    Serial.println(F("Время установлено по встроенным часам"));
  }
  if (cfg.getBoolValue(F("enable_button"))) {
    int button_pin = cfg.getIntValue(F("button_pin"));
    btn.begin(button_pin, INPUT_PULLUP, !cfg.getBoolValue("button_inversed"));
    btn.setLongClickTime(700);
    btn.setDoubleClickTime(500);
    btn.setClickHandler(&buttonHandler);
    btn.setDoubleClickHandler(&buttonHandler);
    btn.setTripleClickHandler(&buttonHandler);
    btn.setLongClickDetectedHandler(&buttonLongClickHandler);
    isButtonEnabled = true;
  } else {
    isButtonEnabled = false;
  }
  if (cfg.getBoolValue(F("enable_buzzer"))) {
    isBuzzerEnabled = true;
    buzzer_pin = cfg.getIntValue(F("buzzer_pin"));
    pinMode(buzzer_pin,OUTPUT);
    buzzer_passive = cfg.getBoolValue(F("buzzer_passive"));    
  } else {
    isBuzzerEnabled = true;
  }
}

void doBeep(int note, int duration) {
  tone(buzzer_pin, note, duration);
}

void tickHardware() {
  if (isButtonEnabled) {
    btn.loop();    
  }
  if (isBuzzerEnabled) {
    static unsigned long stopBeepMillis = 0;
    if (stopBeepMillis && millis() >= stopBeepMillis) {
      beepStateRequested = false;
      stopBeepMillis = 0;      
    }
    if (beepStateRequested != beepState && beepStateRequested) {
      stopBeepMillis = millis() + beepLengthRequested;
    }
    if (buzzer_passive) {
      static unsigned long nextmillis;
      if (beepStateRequested != beepState) {
        beepState = beepStateRequested;
        nextmillis = 0;
      }
      if (beepState) {
        if (millis() > nextmillis) {
          doBeep(beepToneRequested, beepMs);
          nextmillis = millis() + beepMs + silentMs;
        }
      }
    } else {
      static unsigned long nextonmillis;
      static unsigned long nextoffmillis;
      if (beepStateRequested != beepState) {
        beepState = beepStateRequested;
        nextonmillis = 0;
        nextoffmillis = 0;
      }
      if (beepState) {
        static unsigned pinState = 0;
        if (millis() > nextonmillis) {
          digitalWrite(buzzer_pin, HIGH);
          nextoffmillis = nextonmillis + beepMs;
          nextonmillis = nextonmillis + beepMs + silentMs;
        } else if (millis() > nextoffmillis) {
          digitalWrite(buzzer_pin, LOW);
          nextoffmillis = nextonmillis;
        }
      } else {
        digitalWrite(buzzer_pin, LOW);
      }
    }
  }
}
