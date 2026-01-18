#pragma once

#if defined(ESP8266)
#error "License_Mgnt is currently not supported on ESP8266 platform, please use ESP32"
#endif

#include "wled.h"

// Debug logging control: define LM_UM_DEBUG at build time to enable verbose logs
#ifdef LM_UM_DEBUG
#define LM_UM_DEBUGF(...) Serial.printf(__VA_ARGS__)
#define LM_UM_DEBUGLN(msg) Serial.println(msg)
#else
#define LM_UM_DEBUGF(...)
#define LM_UM_DEBUGLN(msg)
#endif

// Warning logging (always on unless explicitly suppressed)
#ifndef LM_UM_SUPPRESS_WARN
#define LM_UM_WARNF(...) Serial.printf(__VA_ARGS__)
#define LM_UM_WARNLN(msg) Serial.println(msg)
#else
#define LM_UM_WARNF(...)
#define LM_UM_WARNLN(msg)
#endif

#ifndef USERMOD_ID_LICENSE_MGNT
#define USERMOD_ID_LICENSE_MGNT 0xA901
#endif

class License_Mgnt : public Usermod
{
public:
  // --- Usermod API ---
  void setup();
  void loop();
  void addToJsonInfo(JsonObject &root);
  void addToConfig(JsonObject &root);
  void appendConfigData();
  bool readFromConfig(JsonObject &root);
  uint16_t getId() { return USERMOD_ID_LICENSE_MGNT; }
  const char *getName() { return "License_Mgnt"; }

  // Accessors

private:
  // --- Config values (editable via JSON/UI) ---
  String licenseStatus = "";
  String deviceId = "";

  int8_t devKeyStatus = -1; // 0=valid, -1=not imported, -2=invalid, -3=error
  unsigned long lastTime = 0;
  uint8_t counter = 0;
  uint8_t trialMinsLeft = TIMEOUT_60_MINUTES;
  bool rebootRequested = false;

  static const char _name[];
  static const char _licenseStatus[];
  static const char _deviceId[];

  const int TIMEOUT_60_SECONDS = 60;
  const int TIMEOUT_60_MINUTES = 60;

  // any private methods should go here (non-inline method should be defined out of class)
  void publishMqtt(const char *state, bool retain = false); // example for publishing MQTT message
  void warningEffectBeforeReboot();
  void getDevKeyStatusString(const int8_t devKeyStatus, String &statusStr);
  String getStoredDeviceKey();

  inline uint8_t getTrialMinsLeft() const { return trialMinsLeft; }
  inline void setTrialMinsLeft(uint8_t mins) { trialMinsLeft = mins; }
};
