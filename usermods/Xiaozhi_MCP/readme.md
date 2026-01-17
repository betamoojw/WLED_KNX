# Xiaozhi_MCP Usermod

# Note

This user mode leverages the xiaozhi-esp32-mcp as backend service to establish the connection/bidirectional communication to xiaozhi.me with the following adjustments:

* Use the xiaozhi_mcp-1.0.0.zip fetched from the https://www.arduinolibraries.info/libraries/xiaozhi-mcp and only keep the WebSocketMCP.h and WebSocketMCP.cpp files at minimal for Xiaozhi_MCP user mode.
* Use the wled.h header instead of the ones in the original header list (the wled project has built-in ArduinoJson.h).
* Translate the Chinese into English in the Serial.println function in this WebSocketMCP.cpp file and removed the blank line.

![Screenshot of WLED info page](assets/screenshot_info.png)

![Screenshot of WLED usermod settings page](assets/screenshot_settings.png)


## Features
 - ✨ Adds the Xiaozhi AI platform connection status to the `Info` tab


## Use Examples
- Populate the Xiaozhi AI Agent MCP Endpoint in usermod settings page
- This user mode will not run if MCP Endpoint is not populated


## Compatibility
- ESP32 supports this user mode while ESP8266 does not support
- TBD


## Installation
- Add `Xiaozhi_MCP` to `custom_usermods` in your `platformio.ini` (or `platformio_override.ini`).

## 📝 Change Log

2025-12-31

- Documentation updated

2025-12-26

* "Xiaozhi_MCP" usermod created


## Authors
- M-Tech [@betamoojw](https://github.com/betamoojw)
