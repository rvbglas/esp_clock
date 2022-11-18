#include "Clock.h"
#include <Print.h>

#define MAX_PARAMS 64

Config cfg;

char* copystr(const char* s) {
  int len = strlen(s);
  char* res = new char[len+1];
  strcpy(res, s);
  return res;
}

char* copystr(const __FlashStringHelper* s) {
  int len = strlen_P((PGM_P)s);
  char* res = new char[len+1];
  strcpy_P(res, (PGM_P)s);
  return res;
}

ConfigParameter::ConfigParameter(const char *id, const char* value) {
  init(id, 'S');
  setValue(value);
};

ConfigParameter::ConfigParameter(const char *id, int value) {
  init(id, 'I');
  setValue(value);
};

ConfigParameter::ConfigParameter(const char *id, double value) {
  init(id, 'F');
  setValue(value);
};

ConfigParameter::ConfigParameter(const char *id, bool value) {
  init(id, 'B');
  setValue(value);
};

ConfigParameter::ConfigParameter(const __FlashStringHelper *id, const char* value) {
  init(id, 'S');
  setValue(value);
};

ConfigParameter::ConfigParameter(const __FlashStringHelper *id, int value) {
  init(id, 'I');
  setValue(value);
};

ConfigParameter::ConfigParameter(const __FlashStringHelper *id, double value) {
  init(id, 'F');
  setValue(value);
};

ConfigParameter::ConfigParameter(const __FlashStringHelper *id, bool value) {
  init(id, 'B');
  setValue(value);
};

ConfigParameter::~ConfigParameter() {
  delete[] _id;
  if (_type=='S' && _value.charValue) {
    delete[] _value.charValue;
  }
};

const char *ConfigParameter::getID() const {
  return _id;
};

char  ConfigParameter::getType() const {
  return _type;
};

const bool ConfigParameter::getBoolValue() const {
  return (_type=='B')?_value.boolValue:false;
};

const int ConfigParameter::getIntValue() const {
  return (_type=='I')?_value.intValue:0;
};

const double ConfigParameter::getFloatValue() const {
  return (_type=='F')?_value.floatValue:0.0;
};

const char *ConfigParameter::getCharValue() const {
  return (_type=='S')?_value.charValue:"";
};

void  ConfigParameter::setValue(const bool value) {
  if (_type=='B') {
    _value.boolValue = value;
  }
};

void  ConfigParameter::setValue(const int value) {
  if (_type=='I') {
    _value.intValue = value;
  }
};

void  ConfigParameter::setValue(const double value) {
  if (_type=='F') {
    _value.floatValue = value;
  }
};

void  ConfigParameter::setValue(const char *value) {
  if (_type=='S') {
    int length = strlen(value);
    if (!_value.charValue) {
      _value.charValue  = new char[length + 1];
    } else if (strlen(_value.charValue) != length) {
      delete[] _value.charValue;
      _value.charValue  = new char[length + 1];
    }
    strcpy(_value.charValue,value);
  }
};

void ConfigParameter::init(const char *id, char type) {
  _id = copystr(id);
  _type = type;
  _value.charValue = nullptr;
};

void ConfigParameter::init(const __FlashStringHelper *id, char type) {
  _id = copystr(id);
  _type = type;
  _value.charValue = nullptr;
};

size_t ConfigParameter::printTo(Print& p) const {
  size_t n = 0;
  n += p.print(_id); n += p.print(":"); n += p.print(_type); n += p.print(":");
  switch (_type) {
    case 'B':
      if (_value.boolValue) { n += p.print("true"); }
      else { n += p.print("false"); }
      break;
    case 'I':
      n += p.print(_value.intValue);
      break;
    case 'F':
      n += p.print(_value.floatValue);
      break;
    case 'S':
      n += p.print(_value.charValue);
      break;
  }
  return n;
}

Config::Config() {
  init();
}

void Config::init() {
  _paramsCount = 0;
  _max_params = MAX_PARAMS;
  _params = NULL;
  _timestamp = 0;
}

Config::~Config() {
  if (_params != NULL) {
    for (int i = 0; i < _paramsCount; i++) {
      delete _params[i];
      _params[i] = nullptr;
    }
    free(_params);
    _params = NULL;
  }
}

ConfigParameter* Config::getParameter(int i) {
  return _params[i];
}

int Config::getParametersCount() const {
  return _paramsCount;
}

ConfigParameter* Config::getParam(const char *id) const {
  for (int j = 0; j < _paramsCount; j++) {
    if (strcmp(id, _params[j]->getID()) == 0) {
      return _params[j];
    }
  }
  return nullptr;
}

ConfigParameter* Config::getParam(const __FlashStringHelper *id) const {
  for (int j = 0; j < _paramsCount; j++) {
    if (strcmp_P(_params[j]->getID(),(PGM_P)id) == 0) {
      return _params[j];
    }
  }
  return nullptr;
}

void Config::addParameter(ConfigParameter* p) {
  if (_params == NULL) {
    _params = (ConfigParameter**)malloc(_max_params * sizeof(ConfigParameter*));
  }
  if (_paramsCount == _max_params) {
    _max_params += MAX_PARAMS;
    ConfigParameter** new_params = (ConfigParameter**)realloc(_params, _max_params * sizeof(ConfigParameter*));
    if (new_params != NULL) {
      _params = new_params;
    } else {
      Serial.println(F("Не удалось расширить массив параметров"));
      return;
    }
  }
  _params[_paramsCount] = p;
  _paramsCount++;
}


void Config::setValue(const char *id, const char *value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(id,value));
  }
  _timestamp = now;
}

void Config::setValue(const __FlashStringHelper *id, const char *value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(copystr(id),value));
  }
  _timestamp = now;
}

void Config::setValue(const char *id, int value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(id,value));
  }
  _timestamp = now;
}

void Config::setValue(const __FlashStringHelper *id, int value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(copystr(id),value));
  }
  _timestamp = now;
}

void Config::setValue(const char *id, const double value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(id,value));
  }
  _timestamp = now;
}

void Config::setValue(const __FlashStringHelper *id, const double value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(copystr(id),value));
  }
  _timestamp = now;
}

void Config::setValue(const char *id, const bool value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(id,value));
  }
  _timestamp = now;
}

void Config::setValue(const __FlashStringHelper *id, const bool value) {
  ConfigParameter* p = getParam(id);
  if (p) {
    p->setValue(value);
  } else {
    addParameter(new ConfigParameter(copystr(id),value));
  }
  _timestamp = now;
}

size_t Config::printTo(Print& p) const {
  size_t n = 0;
  for (int i=0; i<_paramsCount; i++) {
    ConfigParameter* param = _params[i];
    if (param) {
      n += p.print(*param); n+= p.println();
    }
  }
  return n;
}

int Config::getIntValue(const char *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getIntValue();
  } else {
    return 0;
  }
}

int Config::getIntValue(const __FlashStringHelper *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getIntValue();
  } else {
    return 0;
  }
}

double Config::getFloatValue(const char *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getFloatValue();
  } else {
    return 0.0;
  }
};

double Config::getFloatValue(const __FlashStringHelper *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getFloatValue();
  } else {
    return 0.0;
  }
};

bool Config::getBoolValue(const char *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getBoolValue();
  } else {
    return false;
  }
};

bool Config::getBoolValue(const __FlashStringHelper *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getBoolValue();
  } else {
    return false;
  }
};

const char* Config::getCharValue(const char *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getCharValue();
  } else {
    return "";
  }
};

const char* Config::getCharValue(const __FlashStringHelper *id) const {
  ConfigParameter* p = getParam(id);
  if (p) {
    return p->getCharValue();
  } else {
    return "";
  }
};

void Config::readFrom(Stream& s) {
  char buf[256]; // name + type + value + separators
  while (s.available()) {
    int x = s.readBytesUntil('\n',buf,255);
    buf[x] = 0;
    if (x>=3) {
      char* origName = strtok(buf,":\r\n");
      char* origType = strtok(NULL,":\r\n");
      char* origValue = strtok(NULL,"\r\n");
      if (origType && origName && origName[0]) {
        if (origType[0]=='B') {
          if (origValue) {
            setValue(copystr(origName),(strcmp(origValue,"true")==0));
          } else {
            setValue(copystr(origName),false);
          }
        } else if (origType[0]=='I') {
          if (origValue) {
            setValue(copystr(origName),atoi(origValue));
          } else {
            setValue(copystr(origName),0);
          }
        } else if (origType[0]=='F') {
          if (origValue) {
            setValue(copystr(origName),atof(origValue));
          } else {
            setValue(copystr(origName),0.0);
          }
        } else if (origType[0]=='S') {
          if (origValue) {
            setValue(copystr(origName),origValue);
          } else {
            setValue(copystr(origName),"");
          }
        }
      }
    }
  }
}

void Config::clear() {
  if (_params != NULL) {
    for (int i = 0; i < _paramsCount; i++) {
      delete _params[i];
      _params[i] = nullptr;
    }
    free(_params);
  }
  init();
}

void Config::dumpJson(Stream& s) const {
  s.print("{");
  for (int i=0; i<_paramsCount; i++) {
    ConfigParameter* param = _params[i];
    if (i) s.print(",");
    s.print("\"");
    s.print(param->getID());
    s.print("\":");
    switch (param->getType()) {
      case 'B':
        s.print(param->getBoolValue()?"true":"false");
        break;
      case 'I':
        s.print(param->getIntValue());
        break;
      case 'F':
        s.print(param->getFloatValue());
        break;
      case 'S':
        s.print("\"");
        s.print(param->getCharValue());
        s.print("\"");
        break;
    }
  }
  s.print("}");
}

unsigned long Config::getTimestamp() {
  return _timestamp;
}

void Config::resetTimestamp() {
  _timestamp = 0;
}

void setupConfig() {
  if (LittleFS.begin()) {
    if (File f = LittleFS.open(F("/config.txt"),"r")) {
      Serial.println('Открываю файл конфигурации');
      cfg.readFrom(f);
      f.close();
      cfg.resetTimestamp();
    }
  } else {
    Serial.println(F("Конфигурация не найдена"));
  }
}

void saveConfig(bool force) {
  if (cfg.getTimestamp() || force) {
    if (File f = LittleFS.open(F("/config.txt"),"w")) {
      f.print(cfg);
      f.close();
      Serial.println(F("Конфигурация сохранена"));
      cfg.resetTimestamp();
    }
  }
}

void reboot() {
  saveConfig();
  LittleFS.end();
  messageModal(F("Перезагружаюсь"));
  ESP.restart();
}

char* distConfig PROGMEM =
"sta_ssid:S:\n"
"sta_psk:S:\n"
"sta_wait:I:120\n"
"ap_ssid:S:WiFi_Clock\n"
"ap_psk:S:1234-5678\n"
"ntp_server:S:ru.pool.ntp.org\n"
"tz:S:MSK-3\n"
"pin_sda:I:4\n"
"pin_scl:I:5\n"
"i2c_speed:I:400000\n"
"enable_rtc:B:true\n"
"flash_dots:B:true\n"
"pin_din:I:12\n"
"pin_clk:I:15\n"
"pin_cs:I:13\n"
"led_modules:I:4\n"
"panel_font:I:1\n"
"panel_speed:I:15\n"
"panel_brightness_day:I:1\n"
"panel_brightness_night:I:0\n"
"panel_zero:B:true\n"
"panel_seconds:B:false\n"
"enable_button:B:true\n"
"button_pin:I:0\n"
"enable_buzzer:B:true\n"
"buzzer_pin:I:14\n"
"buzzer_passive:B:true\n"
"day_from:I:8\n"
"night_from:I:22\n"
"enable_hourly:B:true\n"
"hourly_night:B:false\n"
"hourly_count:I:2\n"
"hourly_tone:I:800\n"
"hourly_beep_ms:I:100\n"
"hourly_silent_ms:I:100\n"
"enable_alarm:B:false\n"
"alarm_hour:I:13\n"
"alarm_minute:I:12\n"
"alarm_days:S:1111111\n"
"alarm_tone:I:1500\n"
"alarm_length:I:60\n"
"alarm_beep_ms:I:300\n"
"alarm_silent_ms:I:200\n"
"enable_weather:B:false\n"
"weather_url:S:http://api.openweathermap.org/data/2.5/weather?lat=5{LATITUDE}&lon={LONGITUDE}&appid={API_KEY}&lang=ru&units=metric\n"
"weather_template:S:Сегодня %weather.[0].description%, температура %main.temp% (%main.feels_like%)°C, влажность %main.humidity%%%, ветер %wind.speed% м/с\n"
"weather_min:I:15\n"
"auth_user:S:\n"
"auth_pwd:S:\n";


void reset() {
  messageModal(F("Сбрасываю настройки"));
  delay(2000);
  if (File f = LittleFS.open(F("/config.txt"),"w")) {
    f.print(distConfig);
    f.close();
  }
  LittleFS.end();
  ESP.restart();
}
