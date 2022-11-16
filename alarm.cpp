#include "Clock.h"

void hourlyBeep() {
  bool enable_hourly = cfg.getBoolValue(F("enable_hourly"));
  int hourly_count = cfg.getIntValue(F("hourly_count"));
  bool hourly_night = cfg.getBoolValue(F("hourly_night"));
  int hourly_beep_ms = cfg.getIntValue(F("hourly_beep_ms"));
  int hourly_silent_ms = cfg.getIntValue(F("hourly_silent_ms"));
  int hourly_tone = cfg.getIntValue(F("hourly_tone"));  
  if ( enable_hourly && hourly_count && (!isNight() || hourly_night)) {
    int length = hourly_count * hourly_beep_ms + (hourly_count - 1) * hourly_silent_ms;
    beep(hourly_tone, length, hourly_beep_ms, hourly_silent_ms);
  }
}

void checkAlarm() {
  bool enable_alarm = cfg.getBoolValue(F("enable_alarm"));
  int alarm_hour = cfg.getIntValue(F("alarm_hour"));
  int alarm_minute = cfg.getIntValue(F("alarm_minute"));
  const char* alarm_days = cfg.getCharValue(F("alarm_days"));
  int alarm_tone = cfg.getIntValue(F("alarm_tone"));
  int alarm_length = cfg.getIntValue(F("alarm_length"));
  int alarm_beep_ms = cfg.getIntValue(F("alarm_beep_ms"));
  int alarm_silent_ms = cfg.getIntValue(F("alarm_silent_ms"));
  if ( enable_alarm && hh == alarm_hour && mi == alarm_minute && alarm_days[dd?dd-1:6]!='0') {
    beep(alarm_tone, alarm_length * 1000, alarm_beep_ms, alarm_silent_ms);
  }
}

void setupAlarm() {
  unregisterTimeHandler(F("hourly-beep"));
  unregisterTimeHandler(F("check-alarm"));
  registerTimeHandler(F("hourly-beep"),'h',hourlyBeep);
  registerTimeHandler(F("check-alarm"),'m',checkAlarm);
}

