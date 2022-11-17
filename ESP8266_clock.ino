#include "Clock.h"
#include <LittleFS.h>

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Starting..."));

  setupConfig();
  Serial.println(cfg);

  setupHandlers();

  setupHardware();
  setupPanel();

  setupNet();
  setupTime();

  setupAlarm();
  setupWeatherRequest();

  setupWeb();
}

void mem() {
  Serial.println(F("-------------------------------------------------------------"));
  Serial.print("Heap:"); Serial.print(ESP.getFreeHeap()); 
  Serial.print(" Largest chunk:"); Serial.print(ESP.getMaxFreeBlockSize());
  Serial.print(" Fragmentation:"); Serial.print(ESP.getHeapFragmentation());
  Serial.print(" Stack:"); Serial.println(ESP.getFreeContStack());
  Serial.println(F("-------------------------------------------------------------"));
}

void loop() {
  static unsigned long lastMillis = 0;
  int interval = 15000;
  // put your main code here, to run repeatedly:
  if (millis() - lastMillis > interval) {
    lastMillis = millis();
    mem();
  }
  tickNet();
  tickTime();
  tickHardware();
  tickPanel();
  tickWeb();
}
