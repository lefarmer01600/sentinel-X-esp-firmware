#pragma once
#include "Module.h"

static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SentinelX</title>
</head>
<body style="font-family:sans-serif;text-align:center;margin-top:40px">
  <h1>Distance</h1>
  <p style="font-size:64px;margin:10px"><span id="dist">--</span> cm</p>

  <h1>Environnement</h1>
  <p style="font-size:40px;margin:10px">
    <span id="temp">--</span> °C &nbsp;|&nbsp; <span id="hum">--</span> %
  </p>

  <h1>Obstacle IR</h1>
  <p style="font-size:40px;margin:10px"><span id="ir">--</span></p>

  <p>Connexion : <b id="statut">...</b></p>
  <button onclick="buzzer('on')"  style="font-size:20px;padding:12px 24px">Buzzer on</button>
  <button onclick="buzzer('off')" style="font-size:20px;padding:12px 24px">Buzzer off</button>

  <script>
    const $ = (id) => document.getElementById(id);
    let ws;

    function connecter() {
      ws = new WebSocket('ws://' + location.hostname + ':81/');
      ws.onopen = () => $('statut').textContent = 'connecté';
      ws.onmessage = (e) => {
        let m;
        try { m = JSON.parse(e.data); } catch { return; }

        if ('dist' in m) {
          $('dist').textContent = (m.dist === null) ? 'hors portée' : m.dist;
        }
        if (m.dht) {
          $('temp').textContent = m.dht.temp;
          $('hum').textContent  = m.dht.hum;
        }
        if (m.ir) {
          $('ir').textContent = m.ir.obstacle ? '🚧 Obstacle' : '✅ Libre';
        }
      };
      ws.onclose = () => {
        $('statut').textContent = 'déconnecté, nouvelle tentative...';
        setTimeout(connecter, 2000);
      };
    }

    function buzzer(s) {
      fetch('/buzzer', { method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'state=' + s });
    }
    connecter();
  </script>
</body>
</html>
)rawliteral";

class WebUIModule : public Module
{
public:
  const char *name() override { return "webui"; }

  void begin(JsonObject, Network &net, ModuleRegistry &) override
  {
    net.http.on("/", HTTP_GET, [&net]()
                { net.http.send_P(200, "text/html", PAGE); });
  }
};