#pragma once
#include "Module.h"
#include <DHT.h>

class DHTModule : public Module {
public:
  const char* name() override { return "dht11"; }

  void begin(JsonObject cfg, Network& net, ModuleRegistry&) override {
    _net      = &net;
    _pin      = cfg["pin"]         | 4;
    _interval = cfg["interval_ms"] | 2000;   // DHT11 max rate is ~1 Hz
    _dht = new DHT(_pin, DHT11);
    _dht->begin();

    net.http.on("/dht", HTTP_GET, [this, &net]() {
      net.http.send(200, "application/json", json());
    });
  }

  void loop() override {
    if (millis() - _last < _interval) return;
    _last = millis();

    float t = _dht->readTemperature();
    float h = _dht->readHumidity();
    if (isnan(t) || isnan(h)) {
      Serial.println("[DHT] read failed");
      return;                        // keep last good values
    }
    _temp = t;
    _hum  = h;

    if (_net->hasClients()) _net->broadcast("{\"dht\":" + json() + "}");
  }

  String json() {
    return String("{\"temp\":") + String(_temp, 1) +
           ",\"hum\":"          + String(_hum, 0)  + "}";
  }

private:
  Network* _net = nullptr;
  DHT*     _dht = nullptr;
  int      _pin = 4;
  unsigned long _interval = 2000, _last = 0;
  float _temp = NAN, _hum = NAN;
};