#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

class Network {
public:
  WebServer        http{80};
  WebSocketsServer ws{81};

  void begin(JsonObject cfg) {
    _ssid     = cfg["ssid"]     | "";
    _pass     = cfg["password"] | "";
    _timeout  = cfg["timeout_ms"]        | 15000;
    _checkInt = cfg["check_interval_ms"] | 10000;

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
    connect();

    http.begin();
    ws.begin();
    ws.onEvent([](uint8_t num, WStype_t type, uint8_t*, size_t) {
      if (type == WStype_CONNECTED)    Serial.printf("[WS] client %u connected\n", num);
      if (type == WStype_DISCONNECTED) Serial.printf("[WS] client %u disconnected\n", num);
    });
    Serial.println("[NET] servers started");
  }

  void loop() {
    http.handleClient();
    ws.loop();
    if (millis() - _lastCheck > _checkInt) {
      _lastCheck = millis();
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NET] Wi-Fi lost, reconnecting...");
        WiFi.disconnect();
        connect();
      }
    }
  }

  void broadcast(const String& msg) {
    if (ws.connectedClients() > 0) ws.broadcastTXT(msg.c_str());
  }
  bool hasClients() { return ws.connectedClients() > 0; }

private:
  String _ssid, _pass;
  unsigned long _timeout = 15000, _checkInt = 10000, _lastCheck = 0;

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
};