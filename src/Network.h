#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>     // NEW
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <functional>             // NEW

class Network {
public:
  WebServer        http{80};
  WebSocketsServer ws{81};
  WebSocketsClient backend;                                   // NEW: client to FastAPI

  // NEW: modules register here to receive backend messages
  std::function<void(const char*, size_t)> onBackendMessage;

  void begin(JsonObject cfg) {
    _ssid     = cfg["ssid"]     | "";
    _pass     = cfg["password"] | "";
    _host     = cfg["hostname"] | "";
    _timeout  = cfg["timeout_ms"]        | 15000;
    _checkInt = cfg["check_interval_ms"] | 10000;

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_host.c_str());
    WiFi.setSleep(false);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
    connect();
    startMdns();

    http.begin();
    ws.begin();
    ws.onEvent([](uint8_t num, WStype_t type, uint8_t*, size_t) {
      if (type == WStype_CONNECTED)    Serial.printf("[WS] client %u connected\n", num);
      if (type == WStype_DISCONNECTED) Serial.printf("[WS] client %u disconnected\n", num);
    });

    beginBackend(cfg);                                        // NEW
    Serial.println("[NET] servers started");
  }

  void loop() {
    http.handleClient();
    ws.loop();
    if (_backendOn) backend.loop();                           // NEW
    if (millis() - _lastCheck > _checkInt) {
      _lastCheck = millis();
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NET] Wi-Fi lost, reconnecting...");
        WiFi.disconnect();
        connect();
        startMdns();
      }
    }
  }

  // CHANGED: fan-out to local page (port 81) and to the backend
  void broadcast(const String& msg) {
    if (ws.connectedClients() > 0) ws.broadcastTXT(msg.c_str());
    if (_backendOn && backend.isConnected()) {
      String m = msg;                      // sendTXT needs a non-const String&
      backend.sendTXT(m);
    }
  }

  // CHANGED: true if the test page OR the backend is connected
  bool hasClients() {
    return ws.connectedClients() > 0 || (_backendOn && backend.isConnected());
  }

  bool backendConnected() { return _backendOn && backend.isConnected(); }   // NEW

private:
  String _ssid, _pass;
  unsigned long _timeout = 15000, _checkInt = 10000, _lastCheck = 0;

  // NEW
  bool _backendOn = false;

  // NEW: reads cfg["backend"] = { "host": "192.168.1.50", "port": 8000, "path": "/api/ws/esp32" }
  // This is called with the same cfg object as begin(), so put the keys in the same config section.
  void beginBackend(JsonObject cfg) {
    const char* host = cfg["backend_host"] | "";
    if (strlen(host) == 0) {
      Serial.println("[BACKEND] no backend_host in config, skipped");
      return;
    }
    uint16_t    port = cfg["backend_port"] | 8000;
    const char* path = cfg["backend_path"] | "/api/ws/esp32";

    backend.begin(host, port, path);
    backend.setReconnectInterval(2000);
    backend.enableHeartbeat(15000, 3000, 2);   // ping every 15 s, 3 s timeout, 2 failures
    backend.onEvent([this](WStype_t type, uint8_t* payload, size_t len) {
      switch (type) {
        case WStype_CONNECTED:
          Serial.println("[BACKEND] connected");
          break;
        case WStype_DISCONNECTED:
          Serial.println("[BACKEND] disconnected");
          break;
        case WStype_TEXT:
          if (onBackendMessage) onBackendMessage((const char*)payload, len);
          break;
        default: break;
      }
    });
    _backendOn = true;
    Serial.printf("[BACKEND] ws://%s:%u%s\n", host, port, path);
  }

  void connect() {
    WiFi.begin(_ssid.c_str(), _pass.c_str());
    Serial.print("[NET] connecting");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < _timeout) {
      delay(500); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\n[NET] connected, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("\n[NET] connection failed");
    }
  }

private:
  String _host;
  bool _mdnsOn = false;

  void startMdns() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (_mdnsOn) MDNS.end();
    if (MDNS.begin(_host.c_str())) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("ws",   "tcp", 81);
      _mdnsOn = true;
      Serial.printf("[NET] mDNS: %s.local\n", _host.c_str());
    }
  }
};