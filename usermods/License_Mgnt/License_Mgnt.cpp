#include "License_Mgnt.h"

void License_Mgnt::setup()
{
  LM_UM_DEBUGLN("[LM-UM] License Management Usermod setup started");  

  // Validate the device key on setup
  devKeyStatus = validateDeviceKey();
  getDevKeyStatusString(devKeyStatus, licenseStatus);
  deviceId = getDeviceId();

  if (0 == devKeyStatus)
  {
    LM_UM_DEBUGLN("[LM-UM] Device key validated successfully on setup");
    String deviceKey = getStoredDeviceKey();
    LM_UM_DEBUGF("[LM-UM] Device key retrieved from storage is %s\n", deviceKey.c_str());
  }
  else
  {
    LM_UM_DEBUGF("[LM-UM] Device key validation failed on setup (status=%d)\n", devKeyStatus);
  }

  // Initialize last time
  lastTime = millis();
  LM_UM_DEBUGLN("[LM-UM] License Management Usermod initialized\n");
}

void License_Mgnt::loop()
{
  unsigned long now = millis();
  // compute elapsed ms as unsigned long to avoid ambiguous abs() overload on unsigned types
  unsigned long elapsedMs = now - lastTime;
  if ((elapsedMs / 1000UL) > (unsigned long)TIMEOUT_60_SECONDS)
  {
    // Reset last time
    lastTime = now;
    // Implement 1-min task logic
    LM_UM_DEBUGLN("[LM-UM] 1-min task triggered\n");

    if (true == rebootRequested)
    {
      LM_UM_DEBUGLN("[LM-UM] Reboot requested, performing reboot now...\n");
      delay(1000); // Allow debug messages to flush
      WLED::instance().reset();
    }

    // Check license status in case device key was not valid before
    devKeyStatus = validateDeviceKey();
    getDevKeyStatusString(devKeyStatus, licenseStatus);
    if (0 != devKeyStatus)
    {
      LM_UM_DEBUGF("[LM-UM] Fail to validate device key (status=%d, %s)\n", devKeyStatus, licenseStatus.c_str());

      LM_UM_DEBUGF("[LM-UM] Have %d minutes to run KNX_IP user mode if no valid key is provided\n", TIMEOUT_60_MINUTES);
      LM_UM_DEBUGF("[LM-UM] Will force reboot once %d-min timeout reached\n", TIMEOUT_60_MINUTES);

      counter++;
      setTrialMinsLeft(TIMEOUT_60_MINUTES - counter);
      LM_UM_DEBUGF("[LM-UM] [%d] minutes remaining for shutting down KNX_IP user mode\n", getTrialMinsLeft());

      if (counter >= TIMEOUT_60_MINUTES)
      {
        // Implement 1-hour task logic
        LM_UM_DEBUGLN("[LM-UM] 1-hour task triggered\n");
        LM_UM_DEBUGLN("[LM-UM] Rebooting device due to invalid device key\n");

        LM_UM_DEBUGLN("[LM-UM] Set effect to \"Red Blink\" as warn before rebooting\n");
        // Set effect to "Red Blink" before rebooting to avoid FX-related issues on restart
        warningEffectBeforeReboot();

        // Should never reach here
        // counter = 0; just for logic completeness
      }
    }
    else
    {
      LM_UM_DEBUGLN("[LM-UM] Device key validated successfully\n");
    }
  }

#ifndef WLED_DISABLE_MQTT
  if (WLED_MQTT_CONNECTED)
  {
    // Todo: adjust topic as needed
  }
#endif
}

void License_Mgnt::addToJsonInfo(JsonObject &root)
{
  LM_UM_DEBUGF("[LM-UM] addToJsonInfo called - adding usermod info to JSON\n");

  // if "u" object does not exist yet we need to create it
  JsonObject user = root["u"];
  if (user.isNull())
  {
    user = root.createNestedObject("u");
  }

  // Check license status
  JsonArray licenseInfo = user.createNestedArray("License");
  String devKeyStatusStr;
  devKeyStatus = validateDeviceKey();
  getDevKeyStatusString(devKeyStatus, devKeyStatusStr);
  licenseInfo.add(devKeyStatusStr);

  // Check free trial status if device key is not valid
  if (0 != devKeyStatus)
  {
    JsonArray trialInfo = user.createNestedArray("Free Trial");
    uint8_t trialMinsLeft = getTrialMinsLeft();
    if (trialMinsLeft > 0)
    {
      if (trialMinsLeft == 1)
      {
        trialInfo.add(String(trialMinsLeft) + " min left");
        LM_UM_DEBUGF("[LM-UM] Free trial active: %d min left\n", trialMinsLeft);
      }
      else
      {
        trialInfo.add(String(trialMinsLeft) + " mins left");
        LM_UM_DEBUGF("[LM-UM] Free trial active: %d mins left\n", trialMinsLeft);
      }
      
    }
    else
    {
      trialInfo.add("Expired");
      LM_UM_DEBUGF("[LM-UM] Free trial expired\n");
    }
  }
}

void License_Mgnt::addToConfig(JsonObject &root)
{
  JsonObject top = root.createNestedObject(FPSTR(_name));
  top[FPSTR(_licenseStatus)] = licenseStatus;
  top[FPSTR(_deviceId)] = deviceId;
}

// Append useful info to the usermod settings gui
void License_Mgnt::appendConfigData()
{
  // No additional info needed currently
}

bool License_Mgnt::readFromConfig(JsonObject &root)
{
  JsonObject top = root[FPSTR(_name)];
  bool configComplete = !top.isNull();
  // configComplete &= getJsonValue(top[FPSTR(_licenseStatus)], licenseStatus); // licenseStatus is read-only
  // configComplete &= getJsonValue(top[FPSTR(_deviceId)], deviceId); // deviceId is read-only
  return configComplete;
}

void License_Mgnt::publishMqtt(const char *state, bool retain)
{
#ifndef WLED_DISABLE_MQTT
  // Check if MQTT Connected, otherwise it will crash the 8266
  if (WLED_MQTT_CONNECTED)
  {
    // Todo: adjust topic as needed
  }
#endif
}

// Quick blink red effect to indicate impending reboot
void License_Mgnt::warningEffectBeforeReboot()
{
  strip.getMainSegment().setColor(0, RGBW32(255, 0, 0, 100));
  strip.getMainSegment().setMode(FX_MODE_BLINK);
  colorUpdated(CALL_MODE_DIRECT_CHANGE);

  LM_UM_DEBUGLN("[LM-UM] Warning effect before reboot triggered\n");
  rebootRequested = true;
}

void License_Mgnt::getDevKeyStatusString(const int8_t devKeyStatus, String &statusStr)
{
  switch (devKeyStatus)
  {
    case 0:
      statusStr = "Valid and Forever";
      LM_UM_DEBUGLN("[LM-UM] Device key is valid and forever\n");
      break;
    case -1:
      statusStr = "Not Imported";
      LM_UM_DEBUGF("[LM-UM] Device key is not imported\n");
      break;
    case -2:
      statusStr = "Error";
      LM_UM_DEBUGF("[LM-UM] Device key open error\n");
      break;
    case -3:
      statusStr = "Invalid";
      LM_UM_DEBUGF("[LM-UM] Device key is invalid\n");
      break;
    default:
      statusStr = "Unknown";
      LM_UM_DEBUGF("[LM-UM] Device key status unknown\n");
      break;
  }
}

String License_Mgnt::getStoredDeviceKey()
{
#define DEVICE_KEY_FILE "/DEVICE_KEY" // NOTE - device key file name
#define DEV_KEY_DEBUG_NAME "DEV-KEY: "

  // Retrieve the stored device key from persistent storage
  String storedDeviceKey = "";
  
  if (WLED_FS.exists(DEVICE_KEY_FILE))
  {
    LM_UM_DEBUGLN(DEV_KEY_DEBUG_NAME "Reading the device key file...");
    File file = WLED_FS.open(DEVICE_KEY_FILE, "r");
    if (file)
    {
      size_t size = file.size();
      char *buf = new char[size + 1];
      file.readBytes(buf, size);
      buf[size] = '\0';
      storedDeviceKey = String(buf);
      delete[] buf;
      file.close();
      LM_UM_DEBUGLN(DEV_KEY_DEBUG_NAME "Device key file read successfully.");
    }
    else
    {
      LM_UM_DEBUGLN(DEV_KEY_DEBUG_NAME "Failed to open device key file!");
    }
  }
  else
  {
    LM_UM_DEBUGLN(DEV_KEY_DEBUG_NAME "Device key file not found.");
  }

  return storedDeviceKey;
}

const char License_Mgnt::_name[] PROGMEM = "License_Mgnt";
const char License_Mgnt::_licenseStatus[] PROGMEM = "License Status";
const char License_Mgnt::_deviceId[] PROGMEM = "Device ID";

static License_Mgnt license_mgnt;
REGISTER_USERMOD(license_mgnt);