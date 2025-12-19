#include "HttpServer.h"
#include <WiFi.h>
#include "config.h"

HttpServer::HttpServer(GpsReader& gps) : server(80), reader(gps) {}

void HttpServer::begin(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  server.on("/", [this]() { server.send(200, "text/html", indexHtml()); });
  server.on("/data", [this]() {
    GpsData d = reader.getData();
    String s = String("{")
      + "\"rx_ok\":" + String(d.rx_ok ? "true" : "false") + ","
      + "\"tx_ok\":" + String(d.tx_ok ? "true" : "false") + ","
      + "\"comm_ok\":" + String(d.comm_ok ? "true" : "false") + ","
      + "\"fix\":" + String(d.fix ? "true" : "false") + ","
      + "\"lat\":" + String(d.lat, 6) + ","
      + "\"lng\":" + String(d.lng, 6) + ","
      + "\"sats\":" + String(d.sats) + ","
      + "\"hdop\":" + String(d.hdop) + ","
      + "\"speed\":" + String(d.speedKmph, 2) + ","
      + "\"alt\":" + String(d.altMeters, 2) + ","
      + "\"time\":\"" + d.timeStr + "\"" 
      + "}";
    server.send(200, "application/json", s);
  });
  server.on("/raw", [this]() {
    server.send(200, "text/plain", reader.getRawText());
  });
  server.begin();
}

void HttpServer::loop() {
  server.handleClient();
}

String HttpServer::indexHtml() {
  return String(
    "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP32 AP</title><style>body{font-family:sans-serif;margin:20px}#v{white-space:pre;margin-bottom:12px}pre{white-space:pre;background:#111;color:#0f0;padding:10px;border-radius:6px;max-height:60vh;overflow:auto}</style></head><body><h1>ESP32 AP</h1><div id='v'>Carregando...</div><pre id='console'></pre><script>async function tick(){try{let d=await (await fetch('/data')).json();document.getElementById('v').textContent=`RX OK: ${d.rx_ok}  TX OK: ${d.tx_ok}  COMM: ${d.comm_ok}\nFix: ${d.fix?'OK':'Sem fixo'}\nLat: ${d.lat}\nLng: ${d.lng}\nSats: ${d.sats}\nHDOP: ${d.hdop}\nSpeed km/h: ${d.speed}\nAlt m: ${d.alt}\nTime: ${d.time}`;let raw=await (await fetch('/raw')).text();document.getElementById('console').textContent=raw;}catch(e){}}setInterval(tick,1000);tick();</script></body></html>"
  );
}
