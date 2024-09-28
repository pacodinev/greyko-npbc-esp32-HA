#pragma once

#include "ha.hpp"
#include "npbc_communication.hpp"

class NPBCController {
public:

  struct InitConf {
    String mqtthost;
    String mqttuser = "";
    String mqttpass = "";
    HardwareSerial *serial;
    float kgPerSec;
    uint32_t generalInfoUpdateMillis=20000;
    uint16_t mqttport=1883;
    uint8_t rxPin, txPin;
  };
  
private:
  InitConf m_conf;

  NPBCCommunication m_comm;
  HAEntries m_ha;

  uint32_t m_feederTimeTotal=0;
  uint32_t m_lastGeneralInfoTime;

  NPBCCommunication::GenInfoResp readGeneralInfo(bool &ok);

  static NPBCCommunication::Mode onModeChange(NPBCCommunication::Mode reqMode, NPBCCommunication::Mode oldMode, HAEntries *src);
  static NPBCCommunication::State onStateChange(NPBCCommunication::State reqState, NPBCCommunication::State oldState, HAEntries *src);
  static HANumeric onTempChange(const HANumeric &reqTemp, const HANumeric &oldTemp, HAEntries *src);

public:

  NPBCController(Client &netClient) : m_ha{netClient} {}

  void setup(InitConf conf);

  void loop();
};
