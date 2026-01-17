#pragma once

#if defined(ESP8266)
#error "Xiaozhi MCP is currently not supported on ESP8266 platform, please use ESP32"
#endif

#include "wled.h"
#include <WebSocketMCP.h>

// Debug logging control: define MCP_UM_DEBUG at build time to enable verbose logs
#ifdef MCP_UM_DEBUG
#define MCP_UM_DEBUGF(...) Serial.printf(__VA_ARGS__)
#define MCP_UM_DEBUGLN(msg) Serial.println(msg)
#else
#define MCP_UM_DEBUGF(...)
#define MCP_UM_DEBUGLN(msg)
#endif

// Warning logging (always on unless explicitly suppressed)
#ifndef MCP_UM_SUPPRESS_WARN
#define MCP_UM_WARNF(...) Serial.printf(__VA_ARGS__)
#define MCP_UM_WARNLN(msg) Serial.println(msg)
#else
#define MCP_UM_WARNF(...)
#define MCP_UM_WARNLN(msg)
#endif

#ifndef USERMOD_ID_XIAOZHI_MCP
#define USERMOD_ID_XIAOZHI_MCP 0xA900
#endif

class Xiaozhi_MCP : public Usermod
{
public:
  // --- Usermod API ---
  void setup();
  void loop();
  void addToJsonInfo(JsonObject &root);
  void addToConfig(JsonObject &root);
  void appendConfigData();
  bool readFromConfig(JsonObject &root);
  uint16_t getId() { return USERMOD_ID_XIAOZHI_MCP; }
  const char *getName() { return "Xiaozhi_MCP"; }

private:
  // --- Config values (editable via JSON/UI) ---
  bool isEnabled = false;
  unsigned long connRetryInterval = 5000;
  String mcpEndpoint = "wss://api.xiaozhi.me/mcp/?token=eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjExODk4NCwiYWdlbnRJZCI6MTA5MDAsImVuZHBvaW50SWQiOiJhZ2VudF8xMDkwMCIsInB1cnBvc2UiOiJtY3AtZW5kcG9pbnQiLCJpYXQiOjE3NjgzMTM2NDMsImV4cCI6MTc5OTg3MTI0M30.Rf1Wg030527sX6DGzDzVdP52cOjYOMN67QN1iKU5fAaT3DTbAv3idzxDhn1IV5ecN45HxaXM1JdMDxZc4elkKA"; // Adjust as needed

  bool isSetupDone = false;
  static constexpr unsigned long minLoopInterval = 1000; // minimum allowable interval (ms)
  unsigned long lastTime = 0;
  float temperature = 0.0f;

  static const char _name[];
  static const char _enabled[];
  static const char _connectionRetryInterval[];
  static const char _mcpEndpoint[];

  const int TIMEOUT_60_SECONDS = 60;
  const int TIMEOUT_60_MINUTES = 60;

  // any private methods should go here (non-inline method should be defined out of class)
  void publishMqtt(const char *state, bool retain = false); // example for publishing MQTT message
};

static void getCurrentRGBW(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w);

WebSocketMCP mcpClient;
void onConnectionStatus(bool connected);
void registerMcpTools();