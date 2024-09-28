#pragma once

#include <stdint.h>

#include <Arduino.h>

class NPBCCommunication {
private:
  HardwareSerial *m_serial;
  
  void sendCommand(byte commandId, const byte payload[], size_t payloadSize);

  bool ignoreBytes(size_t bytesCnt);
  bool readResponsePayload(byte payload[], size_t payloadSize, byte &curChkSum);
  size_t readResponse(byte payload[], size_t payloadSize);
  bool ignoreResponse();

public:

  void begin(HardwareSerial *serial, uint8_t rxPin, uint8_t txPin);

  enum class Mode : byte {
    STANDBY=0, AUTO, TIMER, MODE_MAX
  };
  static constexpr char ModeOptions[] = "standby;auto;timer";
  
  enum class State : byte {
    CH_PRIORITY=0, DHW_PRIORITY, PARALLEL_PUMPS, SUMMER_MODE, STATE_MAX
  };
  static constexpr char StateOptions[] = "ch priority;dhw priority;parallel pumps;summer mode";
  
  enum class Status : byte {
    IDLE = 0, FAN_CLEANING, CLEANER, WAIT, LOADING, HEATING, IGNITION1, IGNITION2, UNFOLDING, BURNING, EXTINCTION
  };
  static constexpr char StatusOptions[] = "idle;fan cleaning;cleaner;wait;loading;heating;ignition 1; ignition 2;unfolding;burning;extinction";
  
  enum class Power : byte {
    IDLE = 0, SUSPEND, P1, P2, P3
  };
  static constexpr char PowerOptions[] = "idle;suspend;P1;P2;P3";

  static const char* modeToStr(Mode mode);
  static const char* stateToStr(State state);
  static const char* statusToStr(Status status);
  static const char* powerToStr(Power power);

  struct GenInfoResp {
    int16_t Tset, Tch, Tdhw;
    int16_t flame;
    int16_t fan;
    int16_t feederTime;
    Mode mode;
    State state;
    Status status;
    Power power;
    bool ignitionFail, pelletJam;
    bool heater, CHPump, DHWPump;
  };

  // commands
  GenInfoResp generalInfo(bool &ok);
  void setBurnerModeAndState(Mode mode, State state, bool &ok);
  void setTemperature(byte Tset, bool &ok);
  void resetFeederCounter(bool &ok);
  
 
};
