#include "npbc_communication.hpp"

namespace {
  void printHexArr(uint8_t arr[], size_t arrSize) {
    if(arrSize > 0) {
      for(size_t i=0; i<arrSize-1; ++i) {
        Serial.print(arr[i], HEX);
        Serial.write(' ');
      }
      Serial.print(arr[arrSize-1], HEX);
    }
  }
}

void NPBCCommunication::sendCommand(byte commandId, const byte payload[], size_t payloadSize) {
  while(m_serial->available() > 0) m_serial->read();
  m_serial->write(0x5A);
  m_serial->write(0x5A);
  m_serial->write((byte)(2+payloadSize));
  m_serial->write(commandId);
  {
    // NOTE: DEBUG
    Serial.println("Send to Greyko:");
    Serial.print(0x5A, HEX); Serial.write(' ');
    Serial.print(0x5A, HEX); Serial.write(' ');
    Serial.print((byte)(2+payloadSize), HEX); Serial.write(' ');
    Serial.print(commandId, HEX); Serial.write(' ');
  }
  byte curChkSum = (2+payloadSize)+commandId;
  for(size_t i=0; i<payloadSize; ++i) {
    curChkSum += payload[i];
    m_serial->write((byte)(payload[i] + i + 1));
    Serial.print((byte)(payload[i] + i + 1), HEX); Serial.write(' '); // NOTE: DEBUG
  }
  byte chkSum = curChkSum ^ 0xFF;
  m_serial->write((byte)(chkSum + payloadSize + 1));
  Serial.print((byte)(chkSum + payloadSize + 1), HEX); Serial.println(); // NOTE: DEBUG
}

bool NPBCCommunication::ignoreBytes(size_t bytesCnt) {
  byte smth[1];
  size_t bytesRead;

  for(size_t i=0; i<bytesCnt; ++i) {
    bytesRead = m_serial->readBytes(smth, 1); // has timeout
    if(bytesRead < 1) return false;
    Serial.print(smth[0], HEX); Serial.write(' '); // NOTE: DEBUG
    // smth[0] -= i;
    // curChkSum += smth[0];
  }

  return true;
}

bool NPBCCommunication::readResponsePayload(byte payload[], size_t payloadSize, byte &curChkSum) {
  size_t bytesRead = m_serial->readBytes(payload, payloadSize);
  printHexArr(payload, bytesRead); Serial.write(' '); // NOTE: DEBUG
  if(bytesRead != payloadSize) return false;

  for(size_t i=0; i<bytesRead; ++i) {
    payload[i] -= i;
    curChkSum += payload[i];
  }
  return true;
}

size_t NPBCCommunication::readResponse(byte payload[], size_t payloadSize) {
  byte header[3];
  size_t bytesRead;
  
  bytesRead = m_serial->readBytes(header, sizeof(header));
  {
    // NOTE: DEBUG
    Serial.println("Received from Greyko:");
    printHexArr(header, bytesRead); Serial.write(' ');
  }
  
  if(bytesRead != sizeof(header)) return (size_t)-1;
  if(header[0] != 0x5A || header[1] != 0x5A) return (size_t)-1;
  
  size_t actualSize = header[2];
  
  if(actualSize == 0 || actualSize - 1 > payloadSize) {
    ignoreBytes(actualSize);
    return (size_t)-1;
  }
  
  byte curChkSum = header[2];

  bool ok;
  ok = readResponsePayload(payload, actualSize-1, curChkSum);

  if(!ok) return (size_t)-1;
  
  uint8_t chkSum = curChkSum ^ 0xFF;

  byte recvChkSum;
  bytesRead = m_serial->readBytes(&recvChkSum, 1);
  if(bytesRead != 1) return (size_t)-1;
  Serial.print(recvChkSum, HEX); Serial.println(); // NOTE: DEBUG
  Serial.print("Calculated checksum: "); Serial.println(chkSum, HEX); // NOTE: DEBUG

  recvChkSum -= actualSize-1;
  if(recvChkSum != chkSum) return (size_t)-1;
  return actualSize-1;
}

bool NPBCCommunication::ignoreResponse() {
  byte header[3];
  size_t bytesRead;
  
  bytesRead = m_serial->readBytes(header, sizeof(header));
  {
    // NOTE: DEBUG
    Serial.println("Ignoring this message: ");
    Serial.println("Received from Greyko:");
    printHexArr(header, bytesRead); Serial.write(' ');
  }
  
  if(bytesRead < sizeof(header)) return false;
  if(header[0] != 0x5A || header[1] != 0x5A) return false;
  
  size_t actualSize = header[2];

  ignoreBytes(actualSize);
  
  return true;
}

void NPBCCommunication::begin(HardwareSerial *serial, uint8_t rxPin, uint8_t txPin) {
  m_serial = serial;
  m_serial->begin(9600, SERIAL_8N1, rxPin, txPin);
  m_serial->setTimeout(100); // timeout of 100 ms
}

NPBCCommunication::GenInfoResp NPBCCommunication::generalInfo(bool &ok) {
  GenInfoResp res;
  ok = false;
  
  sendCommand(1, nullptr, 0);

  byte resp[32];
  size_t respSize = readResponse(resp, sizeof(resp));
  if(respSize == (size_t)-1) return res;

  if(respSize < 27) return res;

  res.mode = (NPBCCommunication::Mode)resp[7];
  res.state = (NPBCCommunication::State)resp[8];
  res.status = (NPBCCommunication::Status)resp[9];
  res.ignitionFail = ((resp[12] & (1U << 0)) != 0);
  res.pelletJam = ((resp[12] & (1U << 5)) != 0);
  res.Tset = resp[15];
  res.Tch = resp[16];
  res.Tdhw = resp[17];
  res.flame = resp[19];
  res.heater = ((resp[20] & (1U << 1)) != 0);
  res.CHPump = ((resp[20] & (1U << 7)) != 0);
  res.DHWPump = ((resp[20] & (1U << 3)) != 0);
  res.fan = resp[22];
  res.power = (NPBCCommunication::Power)resp[23];
  res.feederTime = resp[26];
  ok = true;
  return res;
}

void NPBCCommunication::setBurnerModeAndState(Mode mode, State state, bool &ok) {
  ok = false;

  int modeInt = (int)mode;
  int stateInt = (int)state;
  if(modeInt < 0 || modeInt >= (int)Mode::MODE_MAX) return;
  if(stateInt < 0 || stateInt >= (int)State::STATE_MAX) return;

  byte command[] = {(byte) mode, (byte) state};
  sendCommand(3, command, sizeof(command));
  
  ok = ignoreResponse();
}

void NPBCCommunication::setTemperature(byte Tset, bool &ok) {
  ok = false;

  if(Tset < 30 || Tset > 94) return;
  
  byte command[] = {Tset};
  sendCommand(7, command, sizeof(command));
  
  ok = ignoreResponse();
}


void NPBCCommunication::resetFeederCounter(bool &ok) {
  sendCommand(9, nullptr, 0);
  
  ok = ignoreResponse();
}

// *****************
const char* NPBCCommunication::modeToStr(NPBCCommunication::Mode mode) {
  switch(mode) {
    case Mode::STANDBY: return "standby";
    case Mode::AUTO: return "auto";
    case Mode::TIMER: return "timer";
  }
  return "";
}

const char* NPBCCommunication::stateToStr(NPBCCommunication::State state) {
  switch(state) {
    case State::CH_PRIORITY: return "CH priority";
    case State::DHW_PRIORITY: return "DHW priority";
    case State::PARALLEL_PUMPS: return "parallel pumps";
    case State::SUMMER_MODE: return "summer mode";
  }
  return "";
}

const char* NPBCCommunication::statusToStr(NPBCCommunication::Status status) {
  switch(status) {
    case Status::IDLE: return "idle";
    case Status::FAN_CLEANING: return "fan cleaning";
    case Status::CLEANER: return "cleaner";
    case Status::WAIT: return "wait";
    case Status::LOADING: return "loading";
    case Status::HEATING: return "heating";
    case Status::IGNITION1: return "ignition 1";
    case Status::IGNITION2: return "ignition 2";
    case Status::UNFOLDING: return "unfolding";
    case Status::BURNING: return "burning";
    case Status::EXTINCTION: return "extinction";
  }
  return "";
}

const char* NPBCCommunication::powerToStr(NPBCCommunication::Power power) {
  switch(power) {
    case Power::IDLE: return "idle";
    case Power::SUSPEND: return "suspend";
    case Power::P1: return "power 1";
    case Power::P2: return "power 2";
    case Power::P3: return "power 3";
  }
  return "";
}
