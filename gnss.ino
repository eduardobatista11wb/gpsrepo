#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>

const char* AP_SSID = "ESP32-AP";
const char* AP_PASS = "esp32pass";
WebServer server(80);
int GPS_RX_PIN = 16;
int GPS_TX_PIN = 17;
static const uint32_t GPS_BAUD = 9600;
HardwareSerial gpsSerial(2);
uint8_t rawBuf[2048];
size_t rawWrite = 0;
size_t rawCount = 0;
TinyGPSPlus gps;
double lastLat = 0.0;
double lastLng = 0.0;
uint32_t lastSats = 0;
uint32_t lastHdop = 0;
double lastSpeed = 0.0;
double lastAlt = 0.0;
char lastTime[24] = "--/--/---- --:--:--";
uint32_t gpsTotal = 0;
uint32_t lastTotal = 0;
unsigned long lastPrintMs = 0;
unsigned long lastRxMs = 0;
unsigned long lastNmeaMs = 0;
unsigned long lastUbxMs = 0;
uint8_t prevByte = 0;

const char INDEX[] = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP32 AP</title><style>body{font-family:sans-serif;margin:20px}#v{white-space:pre;margin-bottom:12px}pre{white-space:pre;background:#111;color:#0f0;padding:10px;border-radius:6px;max-height:60vh;overflow:auto}label{margin-right:8px}button{margin-right:8px}</style></head><body><h1>ESP32 AP</h1><div id='v'>Carregando...</div><div id='diag'></div><div><label>RX <input id='rx' type='number' value='16'></label><label>TX <input id='tx' type='number' value='17'></label><button id='setPins'>Aplicar</button></div><pre id='console'></pre><script>async function tick(){try{let d=await (await fetch('/data')).json();document.getElementById('v').textContent=`RX OK: ${d.rx_ok}  TX OK: ${d.tx_ok}  COMM: ${d.comm_ok}\nFix: ${d.fix?'OK':'Sem fixo'}\nLat: ${d.lat}\nLng: ${d.lng}\nSats: ${d.sats}\nHDOP: ${d.hdop}\nSpeed km/h: ${d.speed}\nAlt m: ${d.alt}\nTime: ${d.time}`;let g=await (await fetch('/diag')).json();document.getElementById('diag').textContent=`RX Pin: ${g.rx}  TX Pin: ${g.tx}  Baud: ${g.baud}`;let raw=await (await fetch('/raw')).text();document.getElementById('console').textContent=raw;}catch(e){}}setInterval(tick,1000);tick();document.getElementById('setPins').onclick=async()=>{let rx=parseInt(document.getElementById('rx').value)||16;let tx=parseInt(document.getElementById('tx').value)||17;await fetch(`/setpins?rx=${rx}&tx=${tx}`);setTimeout(tick,500);};</script></body></html>";

void setup(){
  Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  server.on("/", [](){ server.send(200, "text/html", INDEX); });
  server.on("/data", [](){
    bool fix = gps.location.isValid();
    double lat = fix ? gps.location.lat() : lastLat;
    double lng = fix ? gps.location.lng() : lastLng;
    uint32_t sats = gps.satellites.isValid() ? gps.satellites.value() : lastSats;
    uint32_t hdop = gps.hdop.isValid() ? gps.hdop.value() : lastHdop;
    double speed = gps.speed.isValid() ? gps.speed.kmph() : lastSpeed;
    double alt = gps.altitude.isValid() ? gps.altitude.meters() : lastAlt;
    unsigned long now = millis();
    bool rx_ok = (now - lastRxMs) < 2000;
    bool tx_ok = gpsSerial.availableForWrite() > 0;
    bool comm_ok = rx_ok && ( (now - lastNmeaMs) < 3000 || (now - lastUbxMs) < 3000 );
    char timeBuf[24];
    if(gps.date.isValid() && gps.time.isValid()){
      snprintf(timeBuf, sizeof(timeBuf), "%02d/%02d/%04d %02d:%02d:%02d",
               gps.date.day(), gps.date.month(), gps.date.year(),
               gps.time.hour(), gps.time.minute(), gps.time.second());
      memcpy(lastTime, timeBuf, sizeof(lastTime));
    } else {
      memcpy(timeBuf, lastTime, sizeof(timeBuf));
    }
    String s = String("{")
      + "\"rx_ok\":" + String(rx_ok ? "true" : "false") + ","
      + "\"tx_ok\":" + String(tx_ok ? "true" : "false") + ","
      + "\"comm_ok\":" + String(comm_ok ? "true" : "false") + ","
      + "\"fix\":" + String(fix ? "true" : "false") + ","
      + "\"lat\":" + String(lat, 6) + ","
      + "\"lng\":" + String(lng, 6) + ","
      + "\"sats\":" + String(sats) + ","
      + "\"hdop\":" + String(hdop) + ","
      + "\"speed\":" + String(speed, 2) + ","
      + "\"alt\":" + String(alt, 2) + ","
      + "\"time\":\"" + String(timeBuf) + "\"" 
      + "}";
    server.send(200, "application/json", s);
  });
  server.on("/diag", [](){
    String s = String("{")
      + "\"rx\":" + String(GPS_RX_PIN) + ","
      + "\"tx\":" + String(GPS_TX_PIN) + ","
      + "\"baud\":" + String(GPS_BAUD)
      + "}";
    server.send(200, "application/json", s);
  });
  server.on("/raw", [](){
    String out;
    size_t cap = sizeof(rawBuf);
    size_t cnt = rawCount;
    size_t start = (rawWrite + cap - cnt) % cap;
    out.reserve(cnt + 32);
    for(size_t i=0;i<cnt;i++){
      char ch = (char)rawBuf[(start + i) % cap];
      out += ch;
    }
    server.send(200, "text/plain", out);
  });
  server.on("/setpins", [](){
    int rx = GPS_RX_PIN;
    int tx = GPS_TX_PIN;
    if(server.hasArg("rx")) rx = server.arg("rx").toInt();
    if(server.hasArg("tx")) tx = server.arg("tx").toInt();
    gpsSerial.end();
    GPS_RX_PIN = rx;
    GPS_TX_PIN = tx;
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    server.send(200, "text/plain", "OK");
  });
  server.begin();
  Serial.println(WiFi.softAPIP());
}

void loop(){
  while (gpsSerial.available() > 0) {
    char c = (char)gpsSerial.read();
    Serial.write(c);
    gps.encode(c);
    rawBuf[rawWrite] = (uint8_t)c;
    rawWrite = (rawWrite + 1) % sizeof(rawBuf);
    if(rawCount < sizeof(rawBuf)) rawCount++;
    gpsTotal++;
    lastRxMs = millis();
    if(c == '$') lastNmeaMs = lastRxMs;
    if(prevByte == 0xB5 && (uint8_t)c == 0x62) lastUbxMs = lastRxMs;
    prevByte = (uint8_t)c;
  }
  if(gps.location.isValid()){
    lastLat = gps.location.lat();
    lastLng = gps.location.lng();
  }
  if(gps.satellites.isValid()) lastSats = gps.satellites.value();
  if(gps.hdop.isValid()) lastHdop = gps.hdop.value();
  if(gps.speed.isValid()) lastSpeed = gps.speed.kmph();
  if(gps.altitude.isValid()) lastAlt = gps.altitude.meters();
  if(millis() - lastPrintMs >= 1000){
    if(gpsTotal == lastTotal){
      Serial.println("No GPS data at 9600 on RX=16");
    } else {
      Serial.print("GPS bytes/s: ");
      Serial.println(gpsTotal - lastTotal);
    }
    lastTotal = gpsTotal;
    lastPrintMs = millis();
  }
  server.handleClient();
}
