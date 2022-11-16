#include "Clock.h"
#include "AsyncHTTPRequest_Generic.h"
#include <Ticker.h>
#include <ArduinoJson.h>

Ticker* weatherTicker = nullptr;
AsyncHTTPRequest request;
Config weather;
char weatherData[256];

void executeWeatherRequest();

void IterateJson(JsonVariant json, const String& rootName, Config& lCfg);

void IterateJsonObject(JsonObject json, String rootName, Config& lCfg) {
  for (JsonPair kv : json) {
    IterateJson(kv.value(), rootName == "" ? String(kv.key().c_str()) : rootName + "." + String(kv.key().c_str()),lCfg);
  }
}

void IterateJsonArray(JsonArray json, const String& rootName,  Config& lCfg) {
  for (int i = 0; i < json.size(); i++) {
    IterateJson(json[i], rootName == "" ? "[" + String(i) + "]" : rootName + "." + "[" + String(i) + "]", lCfg);
  }
}

void IterateJson(JsonVariant json, const String& rootName, Config& lCfg) {
  if (json.is<JsonObject>()) {
    IterateJsonObject(json.as<JsonObject>(), rootName, lCfg);
  } else if (json.is<JsonArray>()) {
    IterateJsonArray(json.as<JsonArray>(), rootName, lCfg);
  } else {
    lCfg.setValue(rootName.c_str(), json.as<String>().c_str());
  }
}

void processTemplates(char* buf, const char* strTemplate, const Config& cfg, int maxLen) {
  buf[0] = 0;
  char varName[34];
  const char* ptr = strTemplate;
  while (true) {
    char* pos = strchr(ptr, '%');
    if (!pos) {
      strncat(buf, ptr, maxLen);
      break;
    }
    int len = pos - ptr;
    if (len + strlen(buf) >= maxLen) {
      len = maxLen - strlen(buf);
    }
    int bufLen = strlen(buf);
    strncpy(buf + bufLen, ptr, len);
    buf[bufLen+len] = 0;
    char* endpos = strchr(pos+1, '%');
    if (!endpos) {
      Serial.println(F("Незавершенное имя переменной"));
      break;
    }
    int varLen = (endpos-1) - (pos);
    if (varLen>64) {
      Serial.println(F("Имя переменной слишком длинное"));
      break;
    }
    if (varLen) {
      strncpy(varName,pos+1,varLen);
      varName[varLen] = 0;
      const char* value = cfg.getCharValue(varName);
      if (value) {
        strncat(buf, value, maxLen);
      } else {
        strncat(buf, "??", maxLen);
      }
    } else {
      strncat(buf, "%", maxLen);
    }  
    ptr = endpos+1;
  }
}

void requestCB(void* optParm, AsyncHTTPRequest* request, int readyState) {
  const char* weather_template = cfg.getCharValue(F("weather_template"));

  if (readyState == readyStateDone) {
    if (request->responseHTTPcode() == 200) {
      String weather_json = request->responseText();
      DynamicJsonDocument* current_weather = new DynamicJsonDocument(2048);
      DeserializationError error = deserializeJson(*current_weather, weather_json);
      if (error) {
        Serial.print(F("Ошибка разбора ответа: "));
        Serial.println(error.c_str());
        Serial.println(weather_json);
        return;
      }
      weather.clear();
      IterateJson(current_weather->as<JsonObject>(), "", weather);
      weather_json = String();
      delete current_weather;
      processTemplates(weatherData,weather_template,weather,255);
      sendWeather();
      scroll(weatherData, !isNight());
    }
  }
}

void weatherRequest() {
  if (!WiFi.isConnected()) { return; }
  if (WiFi.isConnected() && weatherTicker) {
    weatherTicker->detach();
    int weather_min = cfg.getIntValue(F("weather_min"));
    weather_min = weather_min ? weather_min : 60;
    weatherTicker->attach(weather_min * 60, executeWeatherRequest);  // reschedule to requested interval
  }
  static bool requestOpenResult;
  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone) {
    const char* weather_url = cfg.getCharValue(F("weather_url"));
    requestOpenResult = request.open("GET", weather_url);

    if (requestOpenResult) {
      request.send();
    } else {
      Serial.println(F("Не удалось отправить запрос"));
    }
  } else {
    Serial.println(F("Не удалось отправить запрос"));
  }
}

void executeWeatherRequest() {
  if (cfg.getBoolValue(F("enable_weather"))) {
    request.onReadyStateChange(requestCB);
    weatherRequest();
  }
}

void setupWeatherRequest() {
  if (weatherTicker) {
    weatherTicker->detach();
    delete weatherTicker;
    weatherTicker = nullptr;
  }
  if (cfg.getBoolValue(F("enable_weather"))) {
    weatherTicker = new Ticker;
    weatherTicker->attach(30, executeWeatherRequest);  // before connection - every 30s
  }
}