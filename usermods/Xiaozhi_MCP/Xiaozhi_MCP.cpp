#include "Xiaozhi_MCP.h"
#include "src/dependencies/network/Network.h"

void Xiaozhi_MCP::setup()
{
  if (!isEnabled)
  {
    return;
  }

  if (((mcpEndpoint == "") || mcpEndpoint.length() == 0))
  {
    MCP_UM_WARNLN("[MCP-UM] MCP Endpoint is empty, disabling usermod.");
    return;
  }

  // Check network connectivity
  if (!Network.isConnected())
  {
    MCP_UM_DEBUGLN("[MCP-UM] Network not connected, deferring MCP.begin().");
    return;
  }

  IPAddress ip = Network.localIP();
  if (!ip || ip.toString() == String("0.0.0.0"))
  {
    MCP_UM_DEBUGLN("[MCP-UM] Network connected but no IP yet, deferring MCP.begin().");
    return;
  }

#ifdef ARDUINO_ARCH_ESP32
  if (Network.isEthernet())
  {
    // For Ethernet, we don't need to disable WiFi sleep
    MCP_UM_DEBUGLN("[MCP-UM] Using Ethernet connection");
  }
  else
  {
    WiFi.setSleep(false); // modem-sleep off helps WiFi multicast reliability
    MCP_UM_DEBUGLN("[MCP-UM] Using WiFi connection, sleep disabled");

    delay(100); // Give lwIP time to complete initialization

    // Verify WiFi is still connected after delay
    if (WiFi.status() != WL_CONNECTED)
    {
      MCP_UM_DEBUGLN("[MCP-UM] WiFi disconnected during lwIP wait, deferring MCP.begin().");
      return;
    }
  }
#endif

  // Additional safety: ensure we're not in a critical network transition
  yield();

  MCP_UM_DEBUGF("[MCP-UM] Mode Name: '%s'\n", getName());
  MCP_UM_DEBUGF("[MCP-UM] MCP Endpoint: '%s'\n", mcpEndpoint.c_str());

  // Start MCP service
  mcpClient.begin(mcpEndpoint.c_str(), onConnectionStatus);

  // Initialize last time
  lastTime = millis();

  isSetupDone = true;
}

void Xiaozhi_MCP::loop()
{
  if (!isEnabled)
  {
    return;
  }

  unsigned long now = millis();
  if (int((now - lastTime) / 1000) > TIMEOUT_60_SECONDS)
  {
    // Reset last time
    lastTime = now;
    // Implement 1-min task logic
    MCP_UM_DEBUGLN("[MCP-UM] 1-min task triggered\n");

    // Check network connectivity
    if (Network.isConnected() && !isSetupDone)
    {
      setup();
    }
  }


#ifndef WLED_DISABLE_MQTT
  if (WLED_MQTT_CONNECTED)
  {
    char array[10];
    snprintf(array, sizeof(array), "%s", mcpTerminalAlias.c_str());
    publishMqtt(array);
  }
#endif
  // Handle MCP client events
  mcpClient.loop();
}

void Xiaozhi_MCP::addToJsonInfo(JsonObject &root)
{
  if (!isEnabled)
  {
    MCP_UM_DEBUGF("[MCP-UM] addToJsonInfo called but usermod disabled\n");
    return;
  }

  MCP_UM_DEBUGF("[MCP-UM] addToJsonInfo called - adding usermod info to JSON\n");

  // if "u" object does not exist yet we need to create it
  JsonObject user = root["u"];
  if (user.isNull())
  {
    user = root.createNestedObject("u");
  }

  // Check MCP service status
  JsonArray mcpStatus = user.createNestedArray("MCP Service Status");
  if (mcpClient.isConnected())
  {
    mcpStatus.add(F("connected"));
    MCP_UM_DEBUGLN("[MCP-UM] MCP service is connected");
  }
  else
  {
    mcpStatus.add(F("disconnected"));
    MCP_UM_DEBUGLN("[MCP-UM] MCP service is not connected");
  }

  // Check MCP terminal alias status
  JsonArray mcpTerminalAlias = user.createNestedArray("MCP Terminal Alias");
  mcpTerminalAlias.add(F(mcpTerminalAlias.c_str()));
}

void Xiaozhi_MCP::addToConfig(JsonObject &root)
{
  JsonObject top = root.createNestedObject(FPSTR(_name));
  top[FPSTR(_enabled)] = isEnabled;
  top[FPSTR(_mcpTerminalAlias)] = mcpTerminalAlias;
  top[FPSTR(_mcpEndpoint)] = mcpEndpoint;
}

// Append useful info to the usermod settings gui
void Xiaozhi_MCP::appendConfigData()
{
  // No additional info needed currently
}

bool Xiaozhi_MCP::readFromConfig(JsonObject &root)
{
  JsonObject top = root[FPSTR(_name)];
  bool configComplete = !top.isNull();
  configComplete &= getJsonValue(top[FPSTR(_enabled)], isEnabled);
  configComplete &= getJsonValue(top[FPSTR(_mcpTerminalAlias)], mcpTerminalAlias);
  configComplete &= getJsonValue(top[FPSTR(_mcpEndpoint)], mcpEndpoint);
  return configComplete;
}

void Xiaozhi_MCP::publishMqtt(const char *state, bool retain)
{
#ifndef WLED_DISABLE_MQTT
  // Check if MQTT Connected, otherwise it will crash the 8266
  if (WLED_MQTT_CONNECTED)
  {
    // Todo: adjust topic as needed
  }
#endif
}

static void getCurrentRGBW(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w)
{
  uint32_t c = SEGCOLOR(0); // primary color slot
  r = R(c);
  g = G(c);
  b = B(c);
  w = W(c);
}

void onConnectionStatus(bool connected)
{
  if (connected)
  {
    Serial.println("[MCP] Connected to server");
    // Register tools after successful connection
    registerMcpTools();
  }
  else
  {
    Serial.println("[MCP] Disconnected from server");
  }
}

void registerMcpTools()
{
  // Register WLED on/off control tool
  mcpClient.registerTool(
      "led_on_off",
      "控制WLED开关",
      "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"string\",\"enum\":[\"on\",\"off\"]}},\"required\":[\"state\"]}",
      [](const String &args)
      {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, args);
        String state = doc["state"].as<String>();

        if (state == "on")
        {
          if (bri == 0)
          {
            bri = (briLast > 0) ? briLast : 128;
          }
        }
        else if (state == "off")
        {
          briLast = bri;
          bri = 0;
        }

        stateUpdated(CALL_MODE_DIRECT_CHANGE);

        return WebSocketMCP::ToolResponse("{\"success\":true,\"state\":\"" + state + "\"}");
      });

  // Register WLED brightness control tool
  mcpClient.registerTool(
      "led_brightness",
      "控制WLED亮度",
      "{\"type\":\"object\",\"properties\":{\"brightness\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":100}},\"required\":[\"brightness\"]}",
      [](const String &args)
      {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, args);
        int brightness = doc["brightness"] | -1;
        if (brightness < 0 || brightness > 100)
        {
          return WebSocketMCP::ToolResponse("{\"success\":false,\"error\":\"invalid brightness\"}");
        }
        // map 0..100 -> 0..255 with rounding
        bri = (uint8_t)(((uint32_t)brightness * 255 + 50) / 100);
        // update last-brightness if turning from zero to non-zero
        if (bri > 0)
        {
          briLast = bri;
        }
        stateUpdated(CALL_MODE_DIRECT_CHANGE);

        return WebSocketMCP::ToolResponse("{\"success\":true,\"brightness\":" + String(brightness) + "}");
      });

  // Register WLED RGB color control tool
  mcpClient.registerTool(
      "led_color",
      "控制WLED颜色",
      "{\"type\":\"object\",\"properties\":{\"r\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255},\"g\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255},\"b\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":255}},\"required\":[\"r\",\"g\",\"b\"]}",
      [](const String &args)
      {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, args);
        int tr = doc["r"] | -1;
        int tg = doc["g"] | -1;
        int tb = doc["b"] | -1;
        if (tr < 0 || tr > 255 || tg < 0 || tg > 255 || tb < 0 || tb > 255)
        {
          return WebSocketMCP::ToolResponse("{\"success\":false,\"error\":\"invalid color values\"}");
        }
        // Use uint8_t for r,g,b as requested
        uint8_t r = (uint8_t)tr;
        uint8_t g = (uint8_t)tg;
        uint8_t b = (uint8_t)tb;

        uint8_t cr, cg, cb, cw, bri;
        getCurrentRGBW(cr, cg, cb, cw);
        bri = strip.getBrightness();
        MCP_UM_DEBUGF("[MCP-UM] R=%d G=%d B=%d <- current setting R=%d G=%d B=%d W=%d\n", r, g, b, cr, cg, cb, cw);
        MCP_UM_DEBUGF("[MCP-UM] Current WLED state: bri=%d, on=%d\n", bri, (bri > 0));

        // Set the color on the segment
        uint32_t newColor = RGBW32(r, g, b, cw);
        strip.getMainSegment().setColor(0, newColor);

        // Update global color variables for GUI synchronization
        colPri[0] = r;
        colPri[1] = g;
        colPri[2] = b;
        colPri[3] = cw;
        MCP_UM_DEBUGF("[MCP-UM] segment color and global colPri updated, calling stateUpdated()\n");

        stateUpdated(CALL_MODE_DIRECT_CHANGE);

        return WebSocketMCP::ToolResponse("{\"success\":true,\"color\":{\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + "}}");
      });

  // Register WLED effect control tool
  mcpClient.registerTool(
      "led_effect",
      "控制WLED效果 (0-128)",
      "{\"type\":\"object\",\"properties\":{\"effect\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":128}},\"required\":[\"effect\"]}",
      [](const String &args)
      {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, args);
        int effectIndex = doc["effect"] | -1;
        if (effectIndex < 0 || effectIndex > 128)
        {
          return WebSocketMCP::ToolResponse("{\"success\":false,\"error\":\"invalid effect index\"}");
        }
        // Set the LED effect by integer index (0..128)
        strip.getMainSegment().setMode(effectIndex);
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
        return WebSocketMCP::ToolResponse("{\"success\":true,\"effect\":" + String(effectIndex) + "}");
      });

  Serial.println("[MCP] LED control tool registered");
}

const char Xiaozhi_MCP::_name[] PROGMEM = "Xiaozhi_MCP";
const char Xiaozhi_MCP::_enabled[] PROGMEM = "Enabled";
const char Xiaozhi_MCP::_mcpTerminalAlias[] PROGMEM = "Terminal Alias";
const char Xiaozhi_MCP::_mcpEndpoint[] PROGMEM = "MCP Endpoint";

static Xiaozhi_MCP xiaozhi_mcp;
REGISTER_USERMOD(xiaozhi_mcp);