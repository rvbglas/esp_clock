#include "Clock.h"
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <StreamString.h>
#include <Ticker.h>

bool isApEnabled = false;
bool isWebStarted = false;

bool pendingWiFi = false;
bool pendingAuth = false;

AsyncWebServer server(80);
AsyncEventSource events("/events");

Ticker tKeepalive;

char auth_user[32];
char auth_pwd[32];

void reportChange(const __FlashStringHelper* name) {
  char buf[256];
  ConfigParameter* param = cfg.getParam(name);
  if (param) {
    switch (param->getType()) {
      case 'B':
        sprintf(buf,"{\"%s\":%s}", name, param->getBoolValue()?"true":"false");
        break;
      case 'I':
        sprintf(buf,"{\"%s\":%d}", name, param->getIntValue());
        break;
      case 'F':
        sprintf(buf,"{\"%s\":%f}", name, param->getFloatValue());
        break;
      case 'S':
        sprintf(buf,"{\"%s\":\"%s\"}", name, param->getCharValue());
        break;
    }
    events.send(buf,"update",millis());
  }
}

void sendInitial(AsyncEventSourceClient *client) {
  String mac = WiFi.macAddress();
  char buf[256];
  sprintf(buf,"{\"_mac\":\"%s\",\"_weather\":\"%s\"}", mac.c_str(), weatherData);
  client->send(buf,"keepalive",millis());
  mac = String();
}

void sendWeather() {
  char buf[256];
  sprintf(buf,"{\"_weather\":\"%s\"}",weatherData);
  events.send(buf,"keepalive",millis());
}

void sendKeepalive() {
  static unsigned long lastMillis = 0;
  static unsigned long uptimeCorrection = 0;
  unsigned long currentMillis = millis();
  unsigned long uptime = millis()/1000;
  if (currentMillis < lastMillis) {
    uptimeCorrection += lastMillis/1000;
  }
  lastMillis = millis();
  uptime = uptime + uptimeCorrection;
  int days = uptime / 86400;
  uptime = uptime % 86400;
  int hrs = uptime / 3600;
  uptime = uptime % 3600;
  int mins = uptime / 60;
  uptime = uptime % 60;
  int heap = ESP.getFreeHeap();
  int rssi = WiFi.RSSI();
  struct tm* timeinfo = localtime(&last_sync);
  bool changed = cfg.getTimestamp() != 0;
  char sync[16] = "--:--:--";
  if (last_sync) {
    sprintf(sync, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  }
  char buf[256];
  if (days) {
    sprintf(buf,"{\"_uptime\":\"%d д %d ч %d % м %d с\", \"_date\":\"%02d.%2d.%04d\", \"_time\":\"%02d:%02d\",\"_heap\":\"%d б\", \"_rssi\":\"%d\",  \"_last_sync\":\"%s\", \"_changed\":%s}", days, hrs, mins, uptime, dd, mm, yy, hh, mi, heap, rssi, sync, changed?"true":"false");
  } else if (hrs) {
    sprintf(buf,"{\"_uptime\":\"%d ч %d % м %d с\", \"_date\":\"%02d.%2d.%04d\", \"_time\":\"%02d:%02d\",\"_heap\":\"%d б\", \"_rssi\":\"%d\",  \"_last_sync\":\"%s\", \"_changed\":%s}", hrs, mins, uptime, dd, mm, yy, hh, mi, heap, rssi, sync, changed?"true":"false");
  } else {
    sprintf(buf,"{\"_uptime\":\"%d м %d с\", \"_date\":\"%02d.%2d.%04d\", \"_time\":\"%02d:%02d\",\"_heap\":\"%d б\", \"_rssi\":\"%d\",  \"_last_sync\":\"%s\", \"_changed\":%s}", mins, uptime, dd, mm, yy, hh, mi, heap, rssi, sync, changed?"true":"false");
  }
  events.send(buf,"keepalive",millis());
}

void apply(const char* name) {
  if (strcmp(name,"sta_ssid") == 0 || strcmp(name,"sta_psk") == 0) {
    if (!isApEnabled) {
      pendingWiFi = true;
    }
  } else if (strcmp(name,"ap_ssid") == 0 || strcmp(name,"ap_psk") == 0) {
    if (isApEnabled) {
      pendingWiFi = true;
    }
  } else if (strcmp(name,"auth_user") == 0 || strcmp(name,"auth_pwd") == 0) {
    pendingAuth = true;
  } else if (strcmp(name,"ntp_server") == 0 || strcmp(name,"tz") == 0) {
    setupTime();
  } else if (strcmp(name,"pin_sda") == 0 || strcmp(name,"pin_scl") == 0 ||
    strcmp(name,"i2c_speed") == 0 || strcmp(name,"enable_rtc") == 0 ||
    strcmp(name,"enable_button") == 0 || strcmp(name,"button_pin") == 0 || strcmp(name,"button_inversed") == 0 ||
    strcmp(name,"enable_buzzer") == 0 || strcmp(name,"buzzer_pin") == 0) {
    setupHardware();
  } else if (strcmp(name,"pin_din") == 0 || strcmp(name,"pin_clk") == 0 ||
    strcmp_P(name,"pin_cs") == 0 || strcmp(name,"led_modules") == 0){
    setupPanel();
  } else if (strcmp(name,"panel_brightness_day") == 0 || strcmp_P(name,"panel_brightness_night") == 0 ||
    strcmp_P(name,"day_from") == 0 || strcmp(name,"night_from") == 0){
    setPanelBrightness();
  }
}

char* actionScheduled = nullptr;
unsigned long millisScheduled = 0;

void setupWeb() {
  char buf[256];

  if (isWebStarted) {
    tKeepalive.detach();
    server.end();
  }

  isWebStarted = true;

  Serial.println("Setting authentication...");
  strncpy(auth_user,cfg.getCharValue(F("auth_user")),31);
  strncpy(auth_pwd,cfg.getCharValue(F("auth_pwd")),31);

  server.on("/action", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (auth_user && auth_pwd && auth_user[0] && auth_pwd[0] && !request->authenticate(auth_user, auth_pwd)) {
      return request-> requestAuthentication();
    }
    if(request->hasParam("name")) {
      const char* action = request->getParam("name")->value().c_str();
      if (strcmp(action,"restart") == 0) {
        if (pendingWiFi) {
          request->send(200,"application/json", "{\"result\":\"FAILED\",\"message\":\"Не применены настройки WiFi\", \"page\":\"wifi\"}");
        } else if (pendingAuth) {
          request->send(200,"application/json", "{\"result\":\"FAILED\",\"message\":\"Не применены настройки авторизации\", \"page\":\"system\"}");
        } else {
          request->send(200,"application/json", "{\"result\":\"OK\",\"message\":\"Перезагружаюсь\"}");
          millisScheduled = millis();
          actionScheduled = "restart";
        }
      } else if (strcmp(action,"wifi") == 0) {
        if (pendingAuth) {
          request->send(200,"application/json", "{\"result\":\"FAILED\",\"message\":\"Не применены настройки авторизации\", \"page\":\"system\"}");
        } else {
          request->send(200,"application/json", "{\"result\":\"OK\",\"message\":\"Применяю настройки\"}");
          millisScheduled = millis();
          actionScheduled = "wifi";
        }
      } else if (strcmp(action,"auth") == 0) {
        request->send(200,"application/json", "{\"result\":\"OK\",\"message\":\"Применяю настройки\"}");
        millisScheduled = millis();
        actionScheduled = "auth";
      } else if (strcmp(action,"save") == 0) {
        if (pendingWiFi) {
          request->send(200,"application/json", "{\"result\":\"FAILED\",\"message\":\"Не применены настройки WiFi\", \"page\":\"wifi\"}");
        } else if (pendingAuth) {
          request->send(200,"application/json", "{\"result\":\"FAILED\",\"message\":\"Не применены настройки авторизации\", \"page\":\"system\"}");
        } else {
          request->send(200,"application/json", "{\"result\":\"OK\",\"message\":\"Сохраняю настройки\"}");
          millisScheduled = millis();
          actionScheduled = "save";
        }
      }
    }
  });

  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (auth_user && auth_pwd && auth_user[0] && auth_pwd[0] && !request->authenticate(auth_user, auth_pwd)) {
      return request-> requestAuthentication();
    }
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
      for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += "{";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += ",\"secure\":" + String(WiFi.encryptionType(i));
        json += ",\"hidden\":" + String(WiFi.isHidden(i) ? "true" : "false");
        json += "}";
      }
      WiFi.scanDelete();
      if (WiFi.scanComplete() == -2) {
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });

  server.on("/config/get", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (auth_user && auth_pwd && auth_user[0] && auth_pwd[0] && !request->authenticate(auth_user, auth_pwd)) {
      return request-> requestAuthentication();
    }
    AsyncResponseStream* s = request->beginResponseStream("application/json");
    s->print("{");
    for (int i = 0; i < cfg.getParametersCount(); i++) {
      ConfigParameter* param = cfg.getParameter(i);
      if (i) s->print(",");
      s->print("\"");
      s->print(param->getID());
      s->print("\":");
      switch (param->getType()) {
        case 'B':
          s->print(param->getBoolValue() ? "true" : "false");
          break;
        case 'I':
          s->print(param->getIntValue());
          break;
        case 'F':
          s->print(param->getFloatValue());
          break;
        case 'S':
          s->print("\"");
          s->print(param->getCharValue());
          s->print("\"");
          break;
      }
    }
    s->print("}");
    request->send(s);
  });

  server.on("/config/set", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (auth_user && auth_pwd && auth_user[0] && auth_pwd[0] && !request->authenticate(auth_user, auth_pwd)) {
      return request-> requestAuthentication();
    }
    if(request->hasParam("name") && (request->hasParam("value"))) {
      const char* name = request->getParam("name")->value().c_str();
      const char* value = request->getParam("value")->value().c_str();
      Serial.print(name); Serial.print(" = "); Serial.println(value);
      char buf[256];
      ConfigParameter* param = cfg.getParam(name);
      if (param) {
        switch (param->getType()) {
          case 'B':
            cfg.setValue(name, strcmp(value, "true")==0);
            sprintf(buf,"{\"%s\":%s}", name, param->getBoolValue()?"true":"false");
            break;
          case 'I':
            cfg.setValue(name, atoi(value));
            sprintf(buf,"{\"%s\":%d}", name, param->getIntValue());
            break;
          case 'F':
            cfg.setValue(name, atof(value));
            sprintf(buf,"{\"%s\":%f}", name, param->getFloatValue());
            break;
          case 'S':
            cfg.setValue(name, value);
            sprintf(buf,"{\"%s\":\"%s\"}", name, param->getCharValue());
            break;
        }
        apply(name);
      } else {
          request->send(500, "text/plain", "Unknown parameter name");
          return;
      }
      request->send(200,"application/json",buf);
     } else {
      request->send(500, "text/plain", "Not all parameters set");
    }
  });

  server.serveStatic("ui", LittleFS, "/ui.json").setAuthentication(auth_user,auth_pwd);

  server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html").setAuthentication(auth_user,auth_pwd);

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404,"text/plain","Not found");
  });

  events.onConnect([](AsyncEventSourceClient *client){
    sendInitial(client);
  });

  events.setAuthentication(auth_user,auth_pwd);

  server.addHandler(&events);

  server.begin();

  tKeepalive.attach(2, sendKeepalive);

}

#define CFG_AUTOSAVE 15

void tickWeb() {
  static char storedSSID[64];
  static char storedPSK[64];
  static bool connectInProgress = false;
  static unsigned long connectMillis = 0;
  if (actionScheduled && millis()>millisScheduled+300) {
    Serial.print(F("Scheduled action ")); Serial.println(actionScheduled);
    //
    if (strcmp(actionScheduled,"restart") == 0) {
      server.end();
      reboot();
    } else if (strcmp(actionScheduled,"auth") == 0) {
      Serial.println("New authentication credentials");
      strncpy(auth_user,cfg.getCharValue(F("auth_user")),31);
      strncpy(auth_pwd,cfg.getCharValue(F("auth_pwd")),31);
      pendingAuth = false;
    } else if (strcmp(actionScheduled,"wifi") == 0) {
      Serial.println("New wifi credentials");
      strcpy(storedSSID,WiFi.SSID().c_str());
      strcpy(storedPSK,WiFi.psk().c_str());
      WiFi.mode(WIFI_STA);
      WiFi.begin(cfg.getCharValue("sta_ssid"),cfg.getCharValue("sta_psk"));
      connectInProgress = true;
      connectMillis = millis();
    } else if (strcmp(actionScheduled,"save") == 0 && !pendingWiFi && !pendingAuth) {
      saveConfig();
    }
    actionScheduled = nullptr;
  }
  if (connectInProgress && (millis() > connectMillis + 1000)) {
    if (WiFi.status() == WL_CONNECTED) {
      char buf[64];
      sprintf(buf,"Подключен к %s, IP=%s", WiFi.SSID(), WiFi.localIP().toString().c_str());
      Serial.println(buf);
      message(buf,1);
      pendingWiFi = false;
      connectInProgress = false;
    } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL || ((WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) && (millis()>connectMillis+1000*cfg.getIntValue("sta_wait")))) {
      Serial.println(F("Подключение не удалось, возвращаю прежние настройки"));
      message(F("Подключение не удалось, возвращаю прежние настройки"),1);
      WiFi.begin(storedSSID, storedPSK);
      connectInProgress = false;
    }
  }

  if (!pendingWiFi && !pendingAuth && cfg.getTimestamp() && cfg.getTimestamp() < now - CFG_AUTOSAVE) {
    saveConfig();
    Serial.println(F("Настройки сохранены"));
  }
}
