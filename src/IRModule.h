#pragma once
#include "Module.h"

class IRModule : public Module {
public:
  const char* name() override { return "ir"; }

  void begin(JsonObject cfg, Network& net, ModuleRegistry&) override {
    _net        = &net;
    _pin        = cfg["pin"]         | 27;
    _interval   = cfg["interval_ms"] | 50;
    _activeLow  = cfg["active_low"]  | true;   // FC-51: LOW = obstacle
    _debounce   = cfg["debounce_ms"] | 100;
    pinMode(_pin, INPUT);

    _state = readRaw();
    _lastChange = millis();

    net.http.on("/ir", HTTP_GET, [this, &net]() {
      net.http.send(200, "application/json", json());
    });
  }

  void loop() override {
    if (millis() - _last < _interval) return;
    _last = millis();

    bool raw = readRaw();
    if (raw == _state) { _candidateSince = 0; return; }

    // state differs: require it to be stable for debounce_ms
    if (_candidateSince == 0) _candidateSince = millis();
    if (millis() - _candidateSince < _debounce) return;

    _state = raw;
    _candidateSince = 0;
    Serial.printf("[IR] obstacle=%s\n", _state ? "yes" : "no");

    if (_net->hasClients()) _net->broadcast("{\"ir\":" + json() + "}");
  }

  String json() {
    return String("{\"obstacle\":") + (_state ? "true" : "false") + "}";
  }

private:
  bool readRaw() {
    bool level = digitalRead(_pin);
    return _activeLow ? !level : level;   // true = obstacle detected
  }

  Network* _net = nullptr;
  int  _pin = 27;
  bool _activeLow = true;
  bool _state = false;
  unsigned long _interval = 50, _debounce = 100;
  unsigned long _last = 0, _lastChange = 0, _candidateSince = 0;
};