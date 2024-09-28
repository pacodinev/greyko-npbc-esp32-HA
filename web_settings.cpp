#include "web_settings.hpp"

#include <WebServer.h>
#include <Preferences.h>

#include <algorithm>

String WebSettings::generatePrefsPage(const char msg[]) const {
  String res;
  res += "<!DOCTYPE html>"
         "<html> <head> </head> <body>";
  for(size_t i=0; i<m_prefListSize; ++i) {
    String keyStr = m_prefList[i];
    String defVal;
    if(keyStr == "fuelKgPerSec") {
      defVal = String(m_pref->getFloat(keyStr.c_str()), 4);
    } else if(keyStr == "updateInterval" || keyStr == "mqttport") {
      defVal = String(m_pref->getUInt(keyStr.c_str()));
    } else if(keyStr == "greykoRXPin" || keyStr == "greykoTXPin") {
      defVal = String(((int)m_pref->getChar(keyStr.c_str())));
    } else if(keyStr == "AOTAPassHash" || keyStr == "wifipass" || keyStr == "mqttpass") {
      defVal = ""; // hide the digest
    } else {
      defVal = m_pref->getString(keyStr.c_str(), "");
    }
    res += "<form method=\"POST\">"
           "<input type=\"hidden\" name=\"prefkey\" value=\"" + keyStr + "\">"
           + keyStr + ": <input type=\"text\" name=\"prefvalue\" value=\"" + defVal + "\">"
           "<input type=\"submit\" value=\"Change\">"
           "</form> <br>";
  }
  res += msg;
  res += "</body> </html>";

  return res;
}

void WebSettings::handlePrefChange() {
  String resp;
  
  if(!m_srv.hasArg("prefkey") || !m_srv.hasArg("prefvalue")) {
    resp = generatePrefsPage("Key not provided <br>");
    m_srv.send(400, "text/html", resp);
    return;
  }

  const String prefKey = m_srv.arg("prefkey");
  const String prefVal = m_srv.arg("prefvalue");

  auto arrPos = std::find(m_prefList, m_prefList+m_prefListSize, prefKey);
  if(arrPos == m_prefList+m_prefListSize) {
    resp = generatePrefsPage("Key invalid <br>");
    m_srv.send(400, "text/html", resp);
    return;
  }

  if(prefVal.indexOf('\"') != -1 || prefVal.indexOf('<') != -1 || prefVal.indexOf('>') != -1) {
    resp = generatePrefsPage("Value characters not allowed <br>");
    m_srv.send(400, "text/plain", resp);
    return;
  }

  if(prefVal == "") {
    m_pref->remove(prefKey.c_str());
  } else if(prefKey == "fuelKgPerSec") {
    m_pref->putFloat(prefKey.c_str(), prefVal.toFloat());
  } else if(prefKey == "updateInterval" || prefKey == "mqttport") {
    m_pref->putUInt(prefKey.c_str(), prefVal.toInt());
  } else if(prefKey == "greykoRXPin" || prefKey == "greykoTXPin") {
    m_pref->putChar(prefKey.c_str(), prefVal.toInt());
  } else {
    m_pref->putString(prefKey.c_str(), prefVal.c_str());
  }
  resp = generatePrefsPage((prefVal == "") ? "Preference removed<br>" : "Preference updated<br>");
  m_srv.send(200, "text/html", resp);
}

void WebSettings::handlePrefsView() {
  String resp = generatePrefsPage();
  m_srv.send(200, "text/html", resp);
}

void WebSettings::setup() {
  m_srv.on("/config", HTTP_GET, [&]() { this->handlePrefsView(); });
  m_srv.on("/config", HTTP_POST, [&]() { this->handlePrefChange(); });
  m_srv.begin();
}

void WebSettings::loop() {
  m_srv.handleClient(); // Handle client requests
}
