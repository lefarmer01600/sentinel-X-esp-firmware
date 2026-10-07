#include "Config.h"
#include "Network.h"
#include "ModuleRegistry.h"
#include "BuzzerModule.h"
#include "UltrasonicModule.h"
#include "WebUIModule.h"
#include "DHTModule.h"
#include "IRModule.h"

Config         config;
Network        net;
ModuleRegistry registry;

BuzzerModule     buzzerModule;
UltrasonicModule ultrasonicModule;
WebUIModule      webUIModule;
DHTModule        dhtModule;
IRModule         irModule;

void setupConfigRoutes() {
  // GET /config  -> returns the running JSON
  net.http.on("/config", HTTP_GET, []() {
    String out; serializeJsonPretty(config.doc, out);
    net.http.send(200, "application/json", out);
  });
  // POST /config (raw JSON body) -> saves to flash and reboots
  net.http.on("/config", HTTP_POST, []() {
    JsonDocument incoming;
    if (deserializeJson(incoming, net.http.arg("plain"))) {
      net.http.send(400, "application/json", "{\"error\":\"invalid json\"}");
      return;
    }
    if (!incoming["wifi"]["ssid"].is<const char*>() || incoming["modules"].isNull()) {
        net.http.send(400, "application/json", "{\"error\":\"missing wifi.ssid or modules\"}");
        return;
    }
    config.doc = incoming;
    if (!config.save()) { net.http.send(500, "application/json", "{\"error\":\"save failed\"}"); return; }
    net.http.send(200, "application/json", "{\"status\":\"saved, rebooting\"}");
    delay(300);
    ESP.restart();
  });
}

void setup() {
  Serial.begin(115200);
  delay(200);

  config.load();                       // from LittleFS, falls back to defaults
  net.begin(config.wifi());

  registry.add(&buzzerModule);         // registration order = init/loop order
  registry.add(&ultrasonicModule);
  registry.add(&webUIModule);
  registry.add(&dhtModule);
  registry.add(&irModule);
  
  registry.beginAll(config, net);      // only enabled ones are started

  setupConfigRoutes();
  Serial.println("[SYS] ready");
}

void loop() {
  net.loop();
  registry.loopAll();
}