#include "config.h"
#include "GpsReader.h"
#include "HttpServer.h"

GpsReader gpsReader;
HttpServer httpServer(gpsReader);

void setup(){
  Serial.begin(9600);
  gpsReader.begin(GPS_BAUD, GPS_RX_PIN, GPS_TX_PIN);
  httpServer.begin(AP_SSID, AP_PASS);
}

void loop(){
  gpsReader.loop();
  httpServer.loop();
}
