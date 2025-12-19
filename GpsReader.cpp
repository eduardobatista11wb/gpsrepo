#include "GpsReader.h"

GpsReader::GpsReader() : serial(2), rawWrite(0), rawCount(0), lastLat(0.0), lastLng(0.0), lastSats(0), lastHdop(0), lastSpeed(0.0), lastAlt(0.0), lastRxMs(0), lastNmeaMs(0), lastUbxMs(0), prevByte(0) {
  lastTime[0] = '\0';
}

void GpsReader::begin(uint32_t baud, int rxPin, int txPin) {
  serial.begin(baud, SERIAL_8N1, rxPin, txPin);
}

void GpsReader::loop() {
  while (serial.available() > 0) {
    char c = (char)serial.read();
    gps.encode(c);
    rawBuf[rawWrite] = (uint8_t)c;
    rawWrite = (rawWrite + 1) % sizeof(rawBuf);
    if (rawCount < sizeof(rawBuf)) rawCount++;
    lastRxMs = millis();
    if (c == '$') lastNmeaMs = lastRxMs;
    if (prevByte == 0xB5 && (uint8_t)c == 0x62) lastUbxMs = lastRxMs;
    prevByte = (uint8_t)c;
  }

  if (gps.location.isValid()) {
    lastLat = gps.location.lat();
    lastLng = gps.location.lng();
  }
  if (gps.satellites.isValid()) lastSats = gps.satellites.value();
  if (gps.hdop.isValid()) lastHdop = gps.hdop.value();
  if (gps.speed.isValid()) lastSpeed = gps.speed.kmph();
  if (gps.altitude.isValid()) lastAlt = gps.altitude.meters();
  if (gps.date.isValid() && gps.time.isValid()) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d",
             gps.date.day(), gps.date.month(), gps.date.year(),
             gps.time.hour(), gps.time.minute(), gps.time.second());
    strncpy(lastTime, buf, sizeof(lastTime));
    lastTime[sizeof(lastTime)-1] = '\0';
  }
}

GpsData GpsReader::getData() {
  unsigned long now = millis();
  GpsData d;
  d.fix = gps.location.isValid();
  d.lat = d.fix ? gps.location.lat() : lastLat;
  d.lng = d.fix ? gps.location.lng() : lastLng;
  d.sats = gps.satellites.isValid() ? gps.satellites.value() : lastSats;
  d.hdop = gps.hdop.isValid() ? gps.hdop.value() : lastHdop;
  d.speedKmph = gps.speed.isValid() ? gps.speed.kmph() : lastSpeed;
  d.altMeters = gps.altitude.isValid() ? gps.altitude.meters() : lastAlt;
  d.timeStr = String(lastTime[0] ? lastTime : "--/--/---- --:--:--");
  bool rx_ok = (now - lastRxMs) < 2000;
  bool tx_ok = serial.availableForWrite() > 0;
  bool comm_ok = rx_ok && ((now - lastNmeaMs) < 3000 || (now - lastUbxMs) < 3000);
  d.rx_ok = rx_ok;
  d.tx_ok = tx_ok;
  d.comm_ok = comm_ok;
  return d;
}

String GpsReader::getRawText() {
  String out;
  size_t cap = sizeof(rawBuf);
  size_t cnt = rawCount;
  size_t start = (rawWrite + cap - cnt) % cap;
  out.reserve(cnt + 32);
  for (size_t i = 0; i < cnt; i++) {
    char ch = (char)rawBuf[(start + i) % cap];
    out += ch;
  }
  return out;
}
