#pragma once
#include <ArduinoJson.h>
#include "Network.h"

class ModuleRegistry;   // forward declaration

class Module {
public:
  virtual ~Module() {}
  virtual const char* name() = 0;
  // Called once if enabled. cfg = the module's own JSON object.
  virtual void begin(JsonObject cfg, Network& net, ModuleRegistry& reg) = 0;
  // Called every loop() if enabled.
  virtual void loop() {}
  bool enabled = false;
};