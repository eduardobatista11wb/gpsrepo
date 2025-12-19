#ifndef GPS_READER_H
#define GPS_READER_H

#include <HardwareSerial.h>
#include <TinyGPS++.h>

struct GpsData {
  bool fix;
  double lat;
  double lng;
  uint32_t sats;
  uint32_t hdop;
  double speedKmph;
  double altMeters;
  String timeStr;
  bool rx_ok;
  bool tx_ok;
  bool comm_ok;
};

class GpsReader {
 public:
  GpsReader();
  void begin(uint32_t baud, int rxPin, int txPin);
  void loop();
  GpsData getData();
  String getRawText();

 private:
  HardwareSerial serial;
  TinyGPSPlus gps;
  uint8_t rawBuf[2048];
  size_t rawWrite;
  size_t rawCount;
  double lastLat;
  double lastLng;
  uint32_t lastSats;
  uint32_t lastHdop;
  double lastSpeed;
  double lastAlt;
  char lastTime[24];
  unsigned long lastRxMs;
  unsigned long lastNmeaMs;
  unsigned long lastUbxMs;
  uint8_t prevByte;
};

#endif
