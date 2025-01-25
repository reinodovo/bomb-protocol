#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ota/ota_server.h>

namespace OTAServer {
AsyncWebServer server(80);

void start(String version) {
  Serial.begin(9600);
  Serial.println("Starting OTA server");
  esp_now_deinit();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("KTANE_OTA_" + WiFi.macAddress());
  IPAddress IP = WiFi.softAPIP();
  Serial.println("AP started. IP: " + WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [version](AsyncWebServerRequest *request) {
    request->send(
        200, "text/html",
        "<h1>KTANE OTA Server</h1><p>Bomb Protocol Version: " + version +
            "</p>"
            "<form method='POST' action='/update' "
            "enctype='multipart/form-data'>"
            "<input type='file' name='firmware'><br>"
            "<button>Upload</button></form>");
  });

  server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        if (!index) {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        }
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
        if (final) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %uB\n", index + len);
          } else {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
}
}; // namespace OTAServer
