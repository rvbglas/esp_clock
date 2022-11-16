#pragma once
#include <ESP8266WiFi.h>
#include <Printable.h>
#include <LittleFS.h>
#include <RTClib.h>

// net.cpp

extern bool isNetConnected;

void setupNet(bool AP = false);
void tickNet();

// time.cpp

extern bool isTimeSet;
extern time_t now;
extern time_t last_sync;

extern int hh;
extern int mi;
extern int ss;

extern int dw;

extern int dd;
extern int mm;
extern int yy;

void registerTimeHandler(const char* handlerName, const char handlerType, std::function<void()> timeHandler);
void unregisterTimeHandler(const char* handlerName);
void registerTimeHandler(const __FlashStringHelper* handlerName, const char handlerType, std::function<void()> timeHandler);
void unregisterTimeHandler(const __FlashStringHelper* handlerName);
void setupHandlers();

void setupTime();
void tickTime();

bool isNight();

// config.cpp

char* copystr(const char* s);
char* copystr(const __FlashStringHelper* s);

union ValueType {
  bool boolValue;
  int intValue;
  double floatValue;
  char *charValue;
};

class ConfigParameter: public Printable {
  public:
    
    ConfigParameter(const char *id, const char* value);
    ConfigParameter(const char *id, int value);
    ConfigParameter(const char *id, double value);
    ConfigParameter(const char *id, bool value);
    ConfigParameter(const __FlashStringHelper* id, const char* value);
    ConfigParameter(const __FlashStringHelper* id, int value);
    ConfigParameter(const __FlashStringHelper* id, double value);
    ConfigParameter(const __FlashStringHelper* id, bool value);
    ~ConfigParameter();
    
    const char *getID() const;
    char  getType() const;
    const bool getBoolValue() const;
    const int getIntValue() const ;
    const double getFloatValue() const;
    const char *getCharValue() const;
    
    void  setValue(const bool value);
    void  setValue(const int value);
    void  setValue(const double value);
    void  setValue(const char *value);

    virtual size_t printTo(Print& p) const;
  
  protected:
    void init(const char *id, char type);
    void init(const __FlashStringHelper *id, char type);
    
  private:
    const char *_id;
    char  _type; // _B_oolean, _I_nt, double _F_loat, _S_tring
    ValueType _value;
};

class Config: public Printable {
  public:

    Config();
    ~Config();

    ConfigParameter* getParameter(int i);
    int   getParametersCount() const;

    void   setValue(const char *id, const char *value);
    void   setValue(const char *id, int value);
    void   setValue(const char *id, double value);
    void   setValue(const char *id, bool value);
    int    getIntValue(const char *id) const;
    double getFloatValue(const char *id) const;
    bool   getBoolValue(const char *id) const;
    const char* getCharValue(const char *id) const;

    void   setValue(const __FlashStringHelper* id, const char *value);
    void   setValue(const __FlashStringHelper* id, int value);
    void   setValue(const __FlashStringHelper* id, double value);
    void   setValue(const __FlashStringHelper* id, bool value);
    int    getIntValue(const __FlashStringHelper* id) const;
    double getFloatValue(const __FlashStringHelper* id) const ;
    bool   getBoolValue(const __FlashStringHelper* id) const;
    const char* getCharValue(const __FlashStringHelper* id) const;

    ConfigParameter* getParam(const char *id) const;
    ConfigParameter* getParam(const __FlashStringHelper* id) const;

    virtual size_t printTo(Print& p) const;

    void readFrom(Stream& s);
    void dumpJson(Stream& s) const;

    void clear();

    unsigned long getTimestamp();
    void resetTimestamp();

  protected:
    void init();
    void addParameter(ConfigParameter* p);

  private:
    int         _paramsCount;
    int         _max_params;
    unsigned long _timestamp;
    ConfigParameter** _params;
  
};

extern Config cfg;
void setupConfig();
void saveConfig(bool force = false);
void reboot();
void reset();

// hardware.cpp

extern bool isRTCEnabled;
extern RTC_DS3231 RTC;

void setupHardware();
void tickHardware();

void beep(int tone, int length, int beep_ms = 60000, int silent_ms = 0);

// panel.cpp

#define mDefault 0
#define mTime 1
#define mDate 2
#define mWeather 3
#define mBarrier 4
#define mMessage 5
#define mLast 6

extern int screenMode;
extern int screenModeRequested;

void setupPanel();
void tickPanel();

void utf8rus(const char* source, char* target, int maxLen = 255);

void message(const char* str, int priority=10);
void message(const __FlashStringHelper*  str, int priority=10);
void messageModal(const char* str);
void messageModal(const __FlashStringHelper*  str);
void scroll(const char* str, bool force = false);
void setPanelBrightness();

// alarm.cpp

void setupAlarm();

// weather.cpp

extern char weatherData[256];

void setupWeatherRequest();

// web.cpp

extern bool isApEnabled;

void setupWeb();
void tickWeb();
void reportChange(const __FlashStringHelper* name);
void sendWeather();
