#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ota.h>

namespace OTA {
const char *POWER_CYCLE_COUNT_KEY = "powerCycleCount";
const unsigned long POWER_CYCLE_TIME_THRESHOLD = 3000;
const int NUMBER_OF_POWER_CYCLES_FOR_OTA = 3;
const unsigned long WIFI_WAIT = 10000;

AsyncWebServer server(80);
Preferences preferences;
bool _could_be_power_cycle = true;
bool _running = false;

void start(String version, String name) {
  _running = true;

  Serial.begin(9600);
  Serial.println("Starting OTA...");

  WiFi.mode(WIFI_STA);
  preferences.begin("wifi", false);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < WIFI_WAIT) {
      Serial.println("Connecting to Wi-Fi...");
      delay(500);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("KTANE_OTA_" + name + "_" +
                WiFi.macAddress().substring(12, 14) +
                WiFi.macAddress().substring(15, 17));
  } else {
    MDNS.begin("ktane-ota");
  }

  server.on("/", HTTP_GET, [version, name](AsyncWebServerRequest *request) {
    request->send(
        200, "text/html",
        "<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> "
        "<meta name=\"viewport\" content=\"width=device-width, "
        "initial-scale=1.0\"> <title>KTANE OTA</title> <style> body { "
        "background-color: #555; color: #ccc; font-family: sans-serif; "
        "text-align: center; padding: 20px; } h1 { font-size: 2em; "
        "text-transform: uppercase; letter-spacing: 3px; text-shadow: 2px 2px "
        "4px rgba(0, 0, 0, 0.7); } p { font-size: 1.2em; margin: 10px 0; } "
        "form { display: inline-block; padding: 15px; border-radius: 8px; "
        "box-shadow: 0 0 10px rgba(0, 0, 0, 0.5); border: 2px solid #b22222; "
        "margin-bottom: 20px; } .file-label, .input-label { display: block; "
        "font-weight: bold; margin: 10px 0 5px; color: #fff; } .file-label { "
        "background-color: #b22222; color: white; padding: 8px 12px; "
        "font-size: 1em; border-radius: 5px; cursor: pointer; transition: "
        "background 0.2s ease-in-out; border: 2px solid #8b1a1a; display: "
        "inline-block; } .file-label:hover { background-color: #d32f2f; } "
        "input[type=\"file\"] { display: none; } input[type=\"text\"], "
        "input[type=\"password\"] { width: 90%; padding: 8px; border: 2px "
        "solid #b22222; border-radius: 5px; font-size: 1em; background-color: "
        "#444; color: white; } #file-name { margin-top: 8px; font-size: 1em; "
        "color: #fff; } button { background-color: #b22222; color: white; "
        "font-size: 1.2em; font-weight: bold; border: 2px solid #8b1a1a; "
        "padding: 8px 16px; border-radius: 5px; cursor: pointer; margin-top: "
        "10px; transition: background 0.2s ease-in-out; } button:hover { "
        "background-color: #d32f2f; } .forms { max-width: 500px; margin: auto; "
        "display: flex; flex-direction: column; justify-content: center; gap: "
        "20px; } </style></head><body> <h1>KTANE OTA</h1> <p>" +
            name + " (" + version + ")</p> <p>" + WiFi.macAddress() +
            "</p> <div class=\"forms\"> <form "
            "method=\"POST\" action=\"/update\" "
            "enctype=\"multipart/form-data\"> "
            "<label for=\"firmware\" class=\"file-label\">Choose File</label> "
            "<input type=\"file\" id=\"firmware\" name=\"firmware\"> <p "
            "id=\"file-name\">No file chosen</p> <button>Upload</button> "
            "</form> "
            "<form method=\"POST\" action=\"/set-wifi\"> <label "
            "class=\"input-label\" for=\"ssid\">Wi-Fi SSID</label> <input "
            "type=\"text\" id=\"ssid\" name=\"ssid\"> <label "
            "class=\"input-label\" for=\"password\">Wi-Fi Password</label> "
            "<input "
            "type=\"password\" id=\"password\" name=\"password\"> "
            "<button "
            "type=\"submit\">Save Wi-Fi Settings</button> </form> </div> "
            "<script> "
            "document.getElementById(\"firmware\").addEventListener(\"change\","
            " "
            "function() { const fileName = this.files[0] ? this.files[0].name "
            ": "
            "\"No file chosen\"; "
            "document.getElementById(\"file-name\").textContent = fileName; "
            "}); "
            "</script></body></html>");
  });

  server.on("/set-wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    const AsyncWebParameter *ssid = request->getParam("ssid", true);
    const AsyncWebParameter *password = request->getParam("password", true);

    Serial.println(ssid->value());
    Serial.println(password->value());

    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid->value());
    preferences.putString("password", password->value());
    preferences.end();

    request->send(200, "text/plain", "OK");
    ESP.restart();
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

void reset() {
  preferences.begin("ota", false);
  preferences.putInt(POWER_CYCLE_COUNT_KEY, 0);
  preferences.end();
}

bool shouldStart() {
  preferences.begin("ota", false);
  int powerCycleCount = preferences.getInt(POWER_CYCLE_COUNT_KEY, 0);
  powerCycleCount++;
  if (powerCycleCount >= NUMBER_OF_POWER_CYCLES_FOR_OTA) {
    preferences.end();
    return true;
  }
  preferences.putInt(POWER_CYCLE_COUNT_KEY, powerCycleCount);
  preferences.end();
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
