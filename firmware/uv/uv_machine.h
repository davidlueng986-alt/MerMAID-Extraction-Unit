#pragma once
// UV-AOP module state machine SKELETON (spec 11 / 7.3).  Implemented: boot with all outputs off (C-S01), U0-U7 state list,
// the lamp-off order DIM low -> relay off (UV-R12) and abort handling (UV-R13).  NOT implemented: the behaviour inside
// the states (see TODO(UV-Rxx)).  Per README the UV module is not built: this firmware only has to compile and pass
// the desktop tests.
#include <stdint.h>
#include "common/commands.h"
#include "common/error_codes.h"
#include "common/log.h"

struct UvHal {
  virtual ~UvHal() {}
  virtual void dim(bool high) = 0;       // LDB-300L DIM (GPIO13)
  virtual void relay(bool on) = 0;       // interlock relay (GPIO27)
  virtual void pumpPwm(int pump, uint8_t v) = 0;  // 1 = P_H, 2 = P_E
  virtual void fanPwm(uint8_t v) = 0;
  virtual void stirPwm(uint8_t v) = 0;
  virtual void buzzer(bool on) = 0;
  virtual void led(bool on) = 0;
  virtual bool doorClosed() = 0;         // IL_SENSE
  virtual bool abortPressed() = 0;       // SW2
  virtual uint32_t millis() = 0;
};

enum class UvState : uint8_t { U0_SelfTest, U1_WaitTube, U2_Fill, U3_WaitStart, U4_AddH2O2, U5_Irradiate, U6_Neutralise, U7_Done, Error };
const char* uv_state_name(UvState s);

class UvMachine {
 public:
  UvMachine(UvHal& hal, Logger& log, const char* fw) : hal_(hal), log_(log), fw_(fw) {}
  void begin();
  void tick();
  void handle(const Command& c);
  UvState state() const { return state_; }
  void lampOff();                        // UV-R12: DIM low first, then relay off
 private:
  void stopAll();
  void toError(uint8_t code);
  void enter(UvState s, const char* ev);
  UvHal& hal_; Logger& log_; const char* fw_;
  UvState state_ = UvState::U0_SelfTest;
  uint8_t err_ = ERR_NONE;
  bool prevAbort_ = false;
};
