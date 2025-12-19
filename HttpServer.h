#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WebServer.h>
#include "GpsReader.h"

class HttpServer {
 public:
  HttpServer(GpsReader& gps);
  void begin(const char* ssid, const char* pass);
  void loop();

 private:
  WebServer server;
  GpsReader& reader;
  static String indexHtml();
};

#endif
