#pragma once

#include <WebServer.h>
#include <Preferences.h>

class WebSettings {
private:
  WebServer m_srv;
  const char * const *m_prefList;
  size_t m_prefListSize;
  Preferences *m_pref;

  String generatePrefsPage(const char msg[] = "") const;

  void handlePrefChange();

  void handlePrefsView();

public:

  WebSettings(uint16_t port, const char * const *prefList, size_t prefListSize, Preferences &prefs)
    : m_srv{port}, m_prefList{prefList}, m_prefListSize{prefListSize}, m_pref{&prefs} {}

  void setup();

  void loop();
};
