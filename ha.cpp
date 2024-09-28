#include "ha.hpp"

#include <string.h>

#include <WiFi.h>

#include <ArduinoHA.h>

#include "npbc_communication.hpp"


#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})

namespace {
  void statusCallback(int8_t index, HASelect *sender) {
    return;
  }

  void powerCallback(int8_t index, HASelect *sender) {
    return;
  }
}

void HAEntries::modeCallback(int8_t index, HASelect *sender) {
  // TODO: index can be -1
  HAEntries *cur = container_of(sender, HAEntries, m_modeEnt);
  if(cur->m_modeChgCbk != nullptr && index >= 0) {
    NPBCCommunication::Mode reqMode = static_cast<NPBCCommunication::Mode>(index);
    NPBCCommunication::Mode oldMode = static_cast<NPBCCommunication::Mode>(sender->getCurrentState());
    NPBCCommunication::Mode res = cur->m_modeChgCbk(reqMode, oldMode, cur);
    sender->setState((int)res);
  }
}
  
void HAEntries::stateCallback(int8_t index, HASelect *sender) {
  // TODO: index can be -1
  HAEntries *cur = container_of(sender, HAEntries, m_stateEnt);
  if(cur->m_stateChgCbk != nullptr && index >= 0) {
    NPBCCommunication::State reqState = static_cast<NPBCCommunication::State>(index);
    NPBCCommunication::State oldState = static_cast<NPBCCommunication::State>(sender->getCurrentState());
    NPBCCommunication::State res = cur->m_stateChgCbk(reqState, oldState, cur);
    sender->setState((int)res);
  }
}

void HAEntries::tempCallback(HANumeric number, HANumber *sender) {
  // TODO: number can be NO VALUE
  HAEntries *cur = container_of(sender, HAEntries, m_TsetEnt);
  if(cur->m_tempChgCbk != nullptr) {
    HANumeric oldTemp = sender->getCurrentState();
    HANumeric res = cur->m_tempChgCbk(number, oldTemp, cur);
    sender->setState(res);
  }
}

void HAEntries::setup(const char mqttserver[], uint16_t mqttport, const char mqttuser[], const char mqttpass[]) {
  byte mac[6] = {0};
  Network.macAddress(mac);
  m_hadev.setUniqueId(mac, sizeof(mac));
  m_hadev.setName("Greyko HA controller");
  m_hadev.setSoftwareVersion("1.2.0-rc1");
  m_hadev.setManufacturer("Paco Dinev");
  // m_hadev.setModel("ABC-123");
  // m_hadev.setConfigurationUrl("http://192.168.1.55:1234"); // TODO: this

  m_hadev.enableSharedAvailability();
  m_hadev.enableLastWill();
  m_hadev.enableExtendedUniqueIds();

  m_hamqtt.begin(mqttserver, mqttport, mqttuser, mqttpass);

  m_hadev.setAvailability(false);

  m_TsetEnt.setName("temperature-set");
  m_TsetEnt.setIcon("mdi:thermostat");
  m_TsetEnt.setUnitOfMeasurement("°C");
  m_TsetEnt.setDeviceClass("temperature");
  m_TsetEnt.setRetain(false);
  m_TsetEnt.setMode(HANumber::ModeBox);
  m_TsetEnt.setMin(40);
  m_TsetEnt.setMax(90);
  m_TsetEnt.setStep(1.0f);
  m_TsetEnt.onCommand(tempCallback);
  
  m_TchEnt.setName("CH");
  m_TchEnt.setIcon("mdi:home-thermometer");
  m_TchEnt.setUnitOfMeasurement("°C");
  m_TchEnt.setStateClass("measurement");
  m_TchEnt.setDeviceClass("temperature");
  m_TdhwEnt.setName("DHW");
  m_TdhwEnt.setIcon("mdi:thermometer-water");
  m_TdhwEnt.setUnitOfMeasurement("°C");
  m_TdhwEnt.setStateClass("measurement");
  m_TdhwEnt.setDeviceClass("temperature");
  
  // TODO: fix flame & fan
  m_flameEnt.setName("flame");
  m_flameEnt.setStateClass("measurement");
  m_flameEnt.setIcon("mdi:fire");
  m_fanEnt.setName("fan");
  m_fanEnt.setStateClass("measurement");
  m_fanEnt.setIcon("mdi:fan");
  
  m_burnedFuel.setName("burned-fuel");
  m_burnedFuel.setIcon("mdi:fuel");
  m_burnedFuel.setStateClass("total_increasing");
  m_burnedFuel.setDeviceClass("weight");
  m_burnedFuel.setUnitOfMeasurement("kg");

  m_ignitionFailEnt.setName("ignition fail");
  m_ignitionFailEnt.setDeviceClass("problem");
  m_ignitionFailEnt.setIcon("mdi:message-alert");
  m_pelletJam.setName("pellet jam");
  m_pelletJam.setDeviceClass("problem");
  m_pelletJam.setIcon("mdi:message-alert");
  
  m_heaterEnt.setName("heater");
  m_heaterEnt.setIcon("mdi:heat-wave");
  m_heaterEnt.setDeviceClass("running");
  m_CHPumpEnt.setName("CH pump");
  m_CHPumpEnt.setIcon("mdi:pump");
  m_CHPumpEnt.setDeviceClass("running");
  m_DHWPumpEnt.setName("DHW pump");
  m_DHWPumpEnt.setIcon("mdi:pump");
  m_DHWPumpEnt.setDeviceClass("running");

  m_modeEnt.setName("mode");
  m_modeEnt.setIcon("mdi:power-cycle");
  m_modeEnt.setOptions(NPBCCommunication::ModeOptions);
  m_modeEnt.setRetain(false);
  m_modeEnt.onCommand(modeCallback);

  m_stateEnt.setName("state");
  m_stateEnt.setIcon("mdi:state-machine");
  m_stateEnt.setOptions(NPBCCommunication::StateOptions);
  m_stateEnt.setRetain(false);
  m_stateEnt.onCommand(stateCallback);

  m_statusEnt.setName("status");
  m_statusEnt.setIcon("mdi:state-machine");
  m_statusEnt.setOptions(NPBCCommunication::StatusOptions);
  m_statusEnt.setRetain(false);
  m_statusEnt.onCommand(statusCallback);

  m_powerEnt.setName("power");
  m_powerEnt.setIcon("mdi:lightning-bolt");
  m_powerEnt.setOptions(NPBCCommunication::PowerOptions);
  m_powerEnt.setRetain(false);
  m_powerEnt.onCommand(powerCallback);
}

void HAEntries::publishGeneralInfo(NPBCCommunication::GenInfoResp info, float burnedFuel) {
  m_burnedFuel.setValue(burnedFuel);

  m_TsetEnt.setState(info.Tset);
  
  m_TchEnt.setValue(info.Tch);
  m_TdhwEnt.setValue(info.Tdhw);
  m_flameEnt.setValue(info.flame);
  m_fanEnt.setValue(info.fan);

  m_ignitionFailEnt.setState(info.ignitionFail);
  m_pelletJam.setState(info.pelletJam);
  m_heaterEnt.setState(info.heater);
  m_CHPumpEnt.setState(info.CHPump);
  m_DHWPumpEnt.setState(info.DHWPump);

  m_modeEnt.setState((int8_t)info.mode);
  m_stateEnt.setState((int8_t)info.state);
  m_statusEnt.setState((int8_t)info.status);
  m_powerEnt.setState((int8_t)info.power);

  m_hadev.setAvailability(true);
}

void HAEntries::makeUnavailable() {
  m_hadev.setAvailability(false);
}
