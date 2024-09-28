#pragma once

#include <ArduinoHA.h>

#include "npbc_communication.hpp"

class HAEntries {
private:
  HADevice m_hadev;
  HAMqtt m_hamqtt;

  // entities:
  HASensorNumber m_TchEnt{"tch"}, m_TdhwEnt{"tdhw"};
  HASensorNumber m_flameEnt{"flame"}, m_fanEnt{"fan"};
  HASensorNumber m_burnedFuel{"fuel", HANumber::PrecisionP2};

  HABinarySensor m_ignitionFailEnt{"ignition-fail"}, m_pelletJam{"pellet-jam"};
  HABinarySensor m_heaterEnt{"heater"}, m_CHPumpEnt{"chpump"}, m_DHWPumpEnt{"dhwpump"};

  HANumber m_TsetEnt{"tset"};

  HASelect m_modeEnt{"mode"}, m_stateEnt{"state"}, m_statusEnt{"status"}, m_powerEnt{"power"};

  using ModeChangeCbkType = NPBCCommunication::Mode (*)(NPBCCommunication::Mode reqMode, NPBCCommunication::Mode oldMode, HAEntries *src);
  using StateChangeCbkType = NPBCCommunication::State (*)(NPBCCommunication::State reqState, NPBCCommunication::State oldState, HAEntries *src);
  using TempChangeCbkType = HANumeric (*)(const HANumeric &reqTemp, const HANumeric &oldTemp, HAEntries *src);
  
  ModeChangeCbkType m_modeChgCbk = nullptr;
  StateChangeCbkType m_stateChgCbk = nullptr;
  TempChangeCbkType m_tempChgCbk = nullptr;

  static void modeCallback(int8_t index, HASelect *sender);
  static void stateCallback(int8_t index, HASelect *sender);
  static void tempCallback(HANumeric number, HANumber *sender);
  
public:
  explicit HAEntries(Client &netClient) : m_hamqtt{netClient, m_hadev, 18} {}

  void setup(const char mqttserver[], uint16_t mqttport=1883, const char mqttuser[]=nullptr, const char mqttpass[]=nullptr);

  void publishGeneralInfo(NPBCCommunication::GenInfoResp info, float burnedFuel);

  void makeUnavailable();

  void onModeChange(ModeChangeCbkType clbk) { m_modeChgCbk = clbk; }
  void onStateChange(StateChangeCbkType clbk) { m_stateChgCbk = clbk; }
  void onTempChange(TempChangeCbkType clbk) { m_tempChgCbk = clbk; }

  void loop() {
    m_hamqtt.loop();
  }

  
  
};
