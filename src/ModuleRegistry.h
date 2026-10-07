#pragma once
#include <vector>
#include "Module.h"
#include "Config.h"

class ModuleRegistry {
public:
  void add(Module* m) { _modules.push_back(m); }

  void beginAll(Config& cfg, Network& net) {
    for (Module* m : _modules) {
      m->enabled = cfg.isEnabled(m->name());
      if (m->enabled) {
        Serial.printf("[MOD] %-12s ENABLED\n", m->name());
        m->begin(cfg.module(m->name()), net, *this);
      } else {
        Serial.printf("[MOD] %-12s disabled\n", m->name());
      }
    }
  }

  void loopAll() {
    for (Module* m : _modules) if (m->enabled) m->loop();
  }

  // Find another module by name (returns nullptr if missing or disabled)
  template<typename T>
  T* get(const char* n) {
    for (Module* m : _modules)
      if (m->enabled && strcmp(m->name(), n) == 0) return static_cast<T*>(m);
    return nullptr;
  }

private:
  std::vector<Module*> _modules;
};