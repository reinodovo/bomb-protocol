#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ota.h>

namespace OTA {
const char *POWER_CYCLE_COUNT_KEY = "powerCycleCount";
const unsigned long POWER_CYCLE_TIME_THRESHOLD = 3000;
const int NUMBER_OF_POWER_CYCLES_FOR_OTA = 3;

AsyncWebServer server(80);
Preferences preferences;
bool _could_be_power_cycle = true;
bool _running = false;

void start(String version, String name) {
  _running = true;
  Serial.begin(9600);
  Serial.println("Starting OTA server");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("KTANE_OTA_" + name + "_" + WiFi.macAddress().substring(12, 14) +
              WiFi.macAddress().substring(15, 17));
  IPAddress IP = WiFi.softAPIP();

  Serial.println("AP started. IP: " + WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [version, name](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
                  "<h1>KTANE OTA Server</h1>"
                  "<p>Bomb Protocol Version: " +
                      version + "</p><p>Module Name: " + name +
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

void reset() { preferences.putInt(POWER_CYCLE_COUNT_KEY, 0); }

bool shouldStart() {
  preferences.begin("ota", false);

  int powerCycleCount = preferences.getInt(POWER_CYCLE_COUNT_KEY, 0);
  powerCycleCount++;
  if (powerCycleCount >= NUMBER_OF_POWER_CYCLES_FOR_OTA) {
    return true;
  }
  preferences.putInt(POWER_CYCLE_COUNT_KEY, powerCycleCount);
  return false;
}

void update() {
  if (!_could_be_power_cycle)
    return;
  if (millis() > POWER_CYCLE_TIME_THRESHOLD) {
    reset();
    _could_be_power_cycle = false;
  }
}

bool running() { return _running; }
}; // namespace OTA
