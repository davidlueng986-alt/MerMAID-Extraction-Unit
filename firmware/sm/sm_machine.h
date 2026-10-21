#pragma once
// Soil module state machine SKELETON (spec 11 / 7.2).  Implemented: boot with all outputs off (C-S01), the S0-S8 state
// list with names, abort -> error handling (C-S06), command replies.  NOT implemented yet: every SM-R0x behaviour
// inside the states (weighing, dosing, closed-loop stirring ...) - see TODO(SM-Rxx) in sm_machine.cpp.
#include <stdint.h>
#include "common/commands.h"
#include "common/error_codes.h"
#include "common/log.h"

struct SmHal {
  virtual ~SmHal() {}
  virtual void pumpPwm(int pump, uint8_t v) = 0;  // 1 = P1 NaCl, 2 = P2 water
  virtual void motorPwm(uint8_t v) = 0;           // TB6612 PWMA (AIN1/AIN2 fixed forward by the HAL)
  virtual void buzzer(bool on) = 0;
  virtual void led(bool on) = 0;
  virtual bool buttonPressed(int sw) = 0;         // 1 = SW1, 2 = SW2
  virtual uint32_t millis() = 0;
};

enum class SmState : uint8_t { S0_SelfTest, S1_PlaceCup, S2_WeighSoil, S3_Dose, S4_Stir, S5_Settle, S6_Handover, S7_Done, S8_Maintenance, Error };
const char* sm_state_name(SmState s);

class SmMachine {
 public:
  SmMachine(SmHal& hal, Logger& log, const char* fw) : hal_(hal), log_(log), fw_(fw) {}
  void begin();
  void tick();
  void handle(const Command& c);
  SmState state() const { return state_; }
 private:
  void stopAll();
  void toError(uint8_t code);
  void enter(SmState s, const char* ev);
  SmHal& hal_; Logger& log_; const char* fw_;
  SmState state_ = SmState::S0_SelfTest;
  uint8_t err_ = ERR_NONE;
  uint32_t pressStart_ = 0;
  bool prevSw1_ = false;
};
