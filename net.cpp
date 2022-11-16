#include "Clock.h"

bool isNetConnected = false;
const char* SSID;
const char* PSK;

const char* apMsg1 PROGMEM = "Активирована точка доступа ";
const char* apMsg2 PROGMEM = ", пароль ";
const char* apMsg3 PROGMEM = ", IP-адрес ";

void setupNet(bool AP) {
  SSID = cfg.getCharValue(F("sta_ssid"));
  PSK = cfg.getCharValue(F("sta_psk"));
  if (SSID && SSID[0] && !AP) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID,PSK);
    isApEnabled = false;
    Serial.println(F("STA mode active"));
  } else {
    SSID = cfg.getCharValue(F("ap_ssid"));
    PSK = cfg.getCharValue(F("ap_psk"));
    if (!SSID || !SSID[0]) {
      SSID = "WiFi Clock";
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID,PSK);
    Serial.println(F("AP mode active"));
    isApEnabled = true;
    String IP = WiFi.softAPIP().toString();
    char buf[256];
    strcpy_P(buf, apMsg1);
    strcat(buf, SSID);
    if (PSK && PSK[0]) {
      strcat_P(buf, apMsg2);
      strcat(buf, PSK);
    }
    strcat_P(buf, apMsg3);
    strcat(buf, IP.c_str());
    message(buf);
  }
}

void tickNet() {
  if (isApEnabled) {
    if (WiFi.status() == WL_CONNECTED && !isNetConnected) {
      isNetConnected = true;
      Serial.print(F("Device connected to SSID ")); Serial.println(SSID);
      message(F("Соединение установлено"));
    } else if (WiFi.status() != WL_CONNECTED && isNetConnected) {
      isNetConnected = false;
      Serial.println(F("Network connection lost"));
      message(F("Сеть потеряна"));
    }
  }
}