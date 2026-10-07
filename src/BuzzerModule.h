#pragma once
#include "Module.h"

class BuzzerModule : public Module {
public:
  const char* name() override { return "buzzer"; }

  void begin(JsonObject cfg, Network& net, ModuleRegistry&) override {
    _pin  = cfg["pin"]  | 23;
    _freq = cfg["freq"] | 2000;

    ledcSetup(_chan, _freq, 8);
    ledcAttachPin(_pin, _chan);
    silence();

    // Commands coming from the FastAPI backend over WebSocket
    net.onBackendMessage = [this](const char* payload, size_t len) {
      JsonDocument doc;
      if (deserializeJson(doc, payload, len)) return;
      applyState(doc);
    };

    // --- local HTTP routes (debug) ---
    net.http.on("/buzzer", HTTP_POST, [this, &net]() {
      if (net.http.hasArg("freq")) {
        int f = net.http.arg("freq").toInt();
        if (f < 20 || f > 20000) {
          net.http.send(400, "application/json", "{\"error\":\"freq must be 20..20000\"}");
          return;
        }
        _freq = f;
      }
      if (net.http.hasArg("state")) {
        String s = net.http.arg("state");
        if      (s == "on")  { stopMelody(); tone(_freq); }
        else if (s == "off") { stopMelody(); silence(); }
        else { net.http.send(400, "application/json", "{\"error\":\"state must be on or off\"}"); return; }
      } else if (!net.http.hasArg("freq")) {
        net.http.send(400, "application/json", "{\"error\":\"state or freq required\"}");
        return;
      } else if (_on) {
        tone(_freq);
      }
      net.http.send(200, "application/json", status());
    });

    net.http.on("/melody/stop", HTTP_POST, [this, &net]() {
      stopMelody();
      silence();
      net.http.send(200, "application/json", status());
    });
  }

  void loop() override {
    if (!_playing) return;
    if (millis() - _stepStart < _steps[_idx].d) return;

    _idx++;
    if (_idx >= _count) {
      _idx = 0;
      if (_repeat > 0 && ++_played >= _repeat) {   // repeat 0 = infinite
        stopMelody();
        silence();
        return;
      }
    }
    playStep();
  }

  // --- public API for other modules ---
  void setFreq(int f) { _freq = f; if (_on && !_playing) tone(_freq); }
  void on()           { stopMelody(); tone(_freq); }
  void off()          { stopMelody(); silence(); }
  bool isOn()         { return _on; }

private:
  struct Step { uint16_t f; uint16_t d; };
  static const int MAX_STEPS = 512;   // 4096 * 4 bytes = 16 KB of RAM; 512 is plenty

  // Backend state: {"buzzer":"on|off","freq":2000,"melody":{...}|null,"melody_id":N}
  void applyState(JsonDocument& doc) {
    int f = doc["freq"] | _freq;
    if (f >= 20 && f <= 20000) _freq = f;

    // 1) New melody (melody_id changed)
    int mid = doc["melody_id"] | 0;
    if (mid != _lastMelodyId) {
      _lastMelodyId = mid;
      JsonObject m = doc["melody"];
      if (!m.isNull() && loadMelody(m)) {
        startMelody();
        return;
      }
    }

    // 2) Melody cancelled by the backend (melody=null)
    bool melodyNull = doc["melody"].isNull();
    const char* b = doc["buzzer"] | "off";

    if (_playing) {
      if (melodyNull) { stopMelody(); silence(); }   // backend sent "off"
      return;                                         // otherwise let the melody finish
    }

    // 3) Plain on/off + freq
    if (strcmp(b, "on") == 0) tone(_freq);
    else                      silence();
  }

  bool loadMelody(JsonObject m) {
    JsonArray steps = m["steps"];
    if (steps.isNull() || steps.size() == 0 || steps.size() > MAX_STEPS) return false;
    _count = 0;
    for (JsonObject s : steps) {
      _steps[_count].f = s["f"] | 0;
      _steps[_count].d = s["d"] | 100;
      _count++;
    }
    _repeat = m["repeat"] | 1;
    return true;
  }

  void tone(int f)  { _on = true;  ledcWriteTone(_chan, f); }
  void silence()    { _on = false; ledcWriteTone(_chan, 0); }

  void startMelody() {
    _idx = 0; _played = 0; _playing = true;
    playStep();
  }
  void stopMelody() { _playing = false; }

  void playStep() {
    _stepStart = millis();
    if (_steps[_idx].f > 0) tone(_steps[_idx].f);
    else                    silence();
  }

  String status() {
    return String("{\"buzzer\":\"") + (_on ? "on" : "off") +
           "\",\"freq\":" + _freq +
           ",\"melody\":" + (_playing ? "true" : "false") + "}";
  }

  int  _pin = 23, _freq = 2000;
  const int _chan = 0;
  bool _on = false;

  Step _steps[MAX_STEPS];
  int  _count = 0, _idx = 0, _repeat = 1, _played = 0;
  bool _playing = false;
  unsigned long _stepStart = 0;
  int  _lastMelodyId = 0;
};