#include "npbc_controller.hpp"

#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})


static NPBCCommunication::GenInfoResp testResp = {
  .Tset=20, .Tch=30, .Tdhw=40,
  .flame=50,
  .fan=60,
  .feederTime=10,
  .mode = NPBCCommunication::Mode::AUTO,
  .state = NPBCCommunication::State::CH_PRIORITY,
  .status = NPBCCommunication::Status::BURNING,
  .power = NPBCCommunication::Power::P2,
  .ignitionFail = false, .pelletJam = false,
  .heater = false, .CHPump = true, .DHWPump = true,
};

NPBCCommunication::Mode NPBCController::onModeChange(NPBCCommunication::Mode reqMode, NPBCCommunication::Mode oldMode, HAEntries *src) {
  NPBCController *cur = container_of(src, NPBCController, m_ha);
  if((int)reqMode < 0) return oldMode;
  bool ok = false;
  NPBCCommunication::GenInfoResp resp = cur->readGeneralInfo(ok);
  if(!ok) return oldMode;
  auto curState = resp.state;
  for(int i=0; i<3; ++i) {
    cur->m_comm.setBurnerModeAndState(reqMode, curState, ok);
    if(ok) break;
  }
  if(!ok) return oldMode;
  return reqMode;
}

NPBCCommunication::State NPBCController::onStateChange(NPBCCommunication::State reqState, NPBCCommunication::State oldState, HAEntries *src) {
  NPBCController *cur = container_of(src, NPBCController, m_ha);
  if((int)reqState < 0) return oldState;
  bool ok = false;
  NPBCCommunication::GenInfoResp resp = cur->readGeneralInfo(ok);
  if(!ok) return oldState;
  auto curMode = resp.mode;
  for(int i=0; i<3; ++i) {
    cur->m_comm.setBurnerModeAndState(curMode, reqState, ok);
    if(ok) break;
  }
  if(!ok) return oldState;
  return reqState;
}

HANumeric NPBCController::onTempChange(const HANumeric &reqTemp, const HANumeric &oldTemp, HAEntries *src) {
  NPBCController *cur = container_of(src, NPBCController, m_ha);
  if(!reqTemp.isSet()) return oldTemp;
  bool ok = false;
  for(int i=0; i<3; ++i) {
    cur->m_comm.setTemperature(reqTemp.toInt8(), ok);
    if(ok) break;
  }
  if(!ok) return oldTemp;
  return reqTemp;
}


NPBCCommunication::GenInfoResp NPBCController::readGeneralInfo(bool &ok) {
  NPBCCommunication::GenInfoResp resp;
  for(int i=0; i<3; ++i) {
    resp = m_comm.generalInfo(ok);
    if(ok) break;
  }
  if(!ok) return resp;
  for(int i=0; i<3; ++i) {
    m_comm.resetFeederCounter(ok);
    if(ok) break;
  }

  m_feederTimeTotal += resp.feederTime;
  
  return resp;
}

void NPBCController::setup(InitConf conf) {
  m_conf = std::move(conf);
  m_comm.begin(m_conf.serial, m_conf.rxPin, m_conf.txPin);
  const char *mqttuserPtr = (m_conf.mqttuser != "") ? m_conf.mqttuser.c_str() : nullptr;
  const char *mqttpassPtr = (m_conf.mqttpass != "") ? m_conf.mqttpass.c_str() : nullptr;

  m_ha.onModeChange(onModeChange);
  m_ha.onStateChange(onStateChange);
  m_ha.onTempChange(onTempChange);
  m_ha.setup(m_conf.mqtthost.c_str(), m_conf.mqttport, mqttuserPtr, mqttpassPtr);

  bool ok = false;
  NPBCCommunication::GenInfoResp resp = readGeneralInfo(ok);
  if(!ok) return;

  float burnedFuel = m_feederTimeTotal * m_conf.kgPerSec;
  m_ha.publishGeneralInfo(resp, burnedFuel);
}

void NPBCController::loop() {
  m_ha.loop();
  
  if(millis() - m_lastGeneralInfoTime >= m_conf.generalInfoUpdateMillis) {
    bool ok = false;
    // NPBCCommunication::GenInfoResp resp = readGeneralInfo(ok);
    NPBCCommunication::GenInfoResp resp = testResp; ok = true;
    m_lastGeneralInfoTime = millis();
    if(!ok) {
      m_ha.makeUnavailable();
      return;
    }
    float burnedFuel = m_feederTimeTotal * m_conf.kgPerSec;
    m_ha.publishGeneralInfo(resp, burnedFuel);
  }
}
