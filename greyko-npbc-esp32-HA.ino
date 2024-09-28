#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "npbc_controller.hpp"

#include "web_settings.hpp"

/**
 * Preferences configuration keys:
 * wifissid
 * wifipass
 * mqtthost
 * mqttport
 * mqttuser
 * mqttpass
 * updateInterval
 * fuelKgPerSec
 * greykoTXPin
 * greykoRXPin
 */

namespace {
  constexpr unsigned long STA_TIMEOUT = 30000; // 30 seconds
  constexpr int WEB_CONFIG_PORT = 1337; // settings are under http://$DEVICE_IP:$WEB_CONFIG_PORT/config
  // example: http://192.168.8.1:1337/config
  
  constexpr char * const prefList[] = { 
    "wifissid", "wifipass", "mqtthost", "mqttport", "mqttuser",
    "mqttpass", "updateInterval", "fuelKgPerSec",
    "greykoTXPin", "greykoRXPin", "AOTAPassHash"
  };
  constexpr size_t prefListSize = 11;

  Preferences preferences;
  
  WiFiClient mqttWifiClient;
  NPBCController haController{mqttWifiClient};
  
  WebSettings webSettngs{1337, prefList, prefListSize, preferences};

  bool isConfigured = false;

  uint32_t staDisconnStart;
  
  
}

namespace {

  void setupWifi() {
    bool staConnected;
    if(preferences.getType("wifissid") == PT_STR && preferences.getType("wifipass") == PT_STR) {
      WiFi.mode(WIFI_STA);
      String wifissid = preferences.getString("wifissid");
      String wifipass = preferences.getString("wifipass");
      WiFi.begin(wifissid, wifipass);
      staDisconnStart = millis();
      Serial.println("SSID: " + wifissid);
      Serial.print("Connecting to WiFi ..");
      int wifiStatus;
      while ((wifiStatus = WiFi.status()) != WL_CONNECTED && (millis() - staDisconnStart) <= STA_TIMEOUT) {
        Serial.print(".");
        delay(200);
      }
      if(wifiStatus == WL_CONNECTED) {
        staConnected = true;
        Serial.println();
        Serial.print(F("Current address: "));
        Serial.println(WiFi.localIP());
      }
    }
    if(!staConnected) {
      Serial.println("Failed to connect to WiFi, switching to AP mode.");

      // Switch to AP mode
      WiFi.mode(WIFI_AP);
      
      IPAddress localIP(192, 168, 8, 1);
      IPAddress gateway(192, 168, 8, 1);
      IPAddress subnet(255, 255, 255, 0);
      WiFi.softAPConfig(localIP, gateway, subnet);
  
      // Start the Access Point
      const char *apSSID = "GreykoESPHA";
      WiFi.softAP(apSSID);
      Serial.println("Access Point started.");
      Serial.print("AP IP Address: ");
      Serial.println(WiFi.softAPIP());
    }
  }
  
}

void setup() {
  delay(100);

  Serial.begin(115200);

  bool ok;

  // Setup NVS
  ok = preferences.begin("config", false);
  if(!ok) {
    Serial.println(F("Failed to init preferences!"));
    ESP.restart();
  }

  // Setup WiFi
  setupWifi();

  String otaHash = preferences.getString("AOTAPassHash");
  if(otaHash != "") {
    ArduinoOTA.setPasswordHash(otaHash.c_str());
  }
  ArduinoOTA.begin();

  NPBCController::InitConf initConf;

  initConf.mqtthost = preferences.getString("mqtthost");
  initConf.mqttport = preferences.getUInt("mqttport");
  initConf.mqttuser = preferences.getString("mqttuser");
  initConf.mqttpass = preferences.getString("mqttpass");
  initConf.serial = &Serial2;
  initConf.kgPerSec = preferences.getFloat("fuelKgPerSec");
  initConf.generalInfoUpdateMillis = preferences.getUInt("updateInterval");
  initConf.rxPin = preferences.getChar("greykoRXPin");
  initConf.txPin = preferences.getChar("greykoTXPin");

  if(preferences.isKey("mqtthost") && preferences.isKey("mqttport") && 
     preferences.isKey("updateInterval") && preferences.isKey("fuelKgPerSec") && 
     preferences.isKey("greykoTXPin") && preferences.isKey("greykoRXPin")) {
    isConfigured = true;
  }

  webSettngs.setup();

  if(isConfigured) {
    haController.setup(std::move(initConf));
  }
}

void loop() {
  ArduinoOTA.handle();

  if(isConfigured) {
    haController.loop();
  }

  webSettngs.loop();  
}
