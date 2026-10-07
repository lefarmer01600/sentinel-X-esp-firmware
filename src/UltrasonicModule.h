#pragma once
#include "Module.h"

class UltrasonicModule : public Module
{
public:
  const char *name() override { return "ultrasonic"; }

  void begin(JsonObject cfg, Network &net, ModuleRegistry &) override
  {
    _net = &net;
    _trig = cfg["trig_pin"] | 5;
    _echo = cfg["echo_pin"] | 18;
    _interval = cfg["interval_ms"] | 200;
    _timeout = cfg["timeout_us"] | 30000;
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);
  }

  void loop() override
  {
    if (millis() - _last < _interval)
      return;
    _last = millis();
    if (!_net->hasClients())
      return;
    float d = measure();
    _net->broadcast(d < 0 ? String("{\"dist\":null}")
                          : String("{\"dist\":") + String(d, 1) + "}");
  }

  float measure()
  {
    digitalWrite(_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trig, LOW);
    long dur = pulseIn(_echo, HIGH, _timeout);
    return dur == 0 ? -1 : dur * 0.0343f / 2;
  }

private:
  Network *_net = nullptr;
  int _trig = 5, _echo = 18;
  unsigned long _interval = 200, _timeout = 30000, _last = 0;
};