#pragma once
#include <ArduinoJson.h>
#include <LittleFS.h>

class Config
{
public:
  JsonDocument doc;

  bool load(const char *path = "/config.json")
  {
    if (!LittleFS.begin(true))
    { // true = format if mount fails
      Serial.println("[CFG] LittleFS mount failed");
      return false;
    }
    File f = LittleFS.open(path, "r");
    if (!f)
    {
      Serial.println("[CFG] config.json not found, using defaults");
      setDefaults();
      return false;
    }
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err)
    {
      Serial.printf("[CFG] JSON error: %s, using defaults\n", err.c_str());
      setDefaults();
      return false;
    }
    Serial.println("[CFG] config.json loaded");
    return true;
  }

  bool save(const char *path = "/config.json")
  {
    File f = LittleFS.open(path, "w");
    if (!f)
      return false;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
  }

  JsonObject wifi() { return doc["wifi"].as<JsonObject>(); }
  JsonObject module(const char *name) { return doc["modules"][name].as<JsonObject>(); }
  bool isEnabled(const char *name) { return doc["modules"][name]["enabled"] | false; }

  void setDefaults()
  {
    doc.clear();
    doc["wifi"]["ssid"] = "sentinelx-wifi";
    doc["wifi"]["password"] = "hotspotpassword";
    doc["wifi"]["timeout_ms"] = 15000;
    doc["wifi"]["check_interval_ms"] = 10000;
    doc["modules"]["buzzer"]["enabled"] = true;
    doc["modules"]["buzzer"]["pin"] = 23;
    doc["modules"]["buzzer"]["freq"] = 2000;
    doc["modules"]["ultrasonic"]["enabled"] = true;
    doc["modules"]["ultrasonic"]["trig_pin"] = 5;
    doc["modules"]["ultrasonic"]["echo_pin"] = 18;
    doc["modules"]["ultrasonic"]["interval_ms"] = 200;
    doc["modules"]["ultrasonic"]["timeout_us"] = 30000;
    doc["modules"]["webui"]["enabled"] = true;
    doc["modules"]["dht11"]["enabled"] = true;
    doc["modules"]["dht11"]["pin"] = 4;
    doc["modules"]["dht11"]["interval_ms"] = 2000;
    doc["modules"]["ir"]["enabled"] = true;
    doc["modules"]["ir"]["pin"] = 27;
    doc["modules"]["ir"]["interval_ms"] = 50;
    doc["modules"]["ir"]["debounce_ms"] = 100;
    doc["modules"]["ir"]["active_low"] = true;
  }
};