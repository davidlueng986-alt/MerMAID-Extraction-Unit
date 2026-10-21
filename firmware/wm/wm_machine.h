#pragma once
// Water module state machine (spec 11 / 7.1: WM-R01..R07).  No Arduino dependency: all hardware goes through WmHal,
// so the same code runs on the ESP32 and in the desktop tests.
#include <stdint.h>
#include "common/commands.h"
#include "common/error_codes.h"
#include "common/fault_inject.h"
#include "common/log.h"

struct WmHal {
  virtual ~WmHal() {}
  virtual void pumpPwm(uint8_t v) = 0;   // 0-255
  virtual void led(bool on) = 0;
  virtual void buzzer(bool on) = 0;
  virtual bool buttonPressed() = 0;      // SW1, true while held
  virtual uint32_t millis() = 0;
};

struct WmConfig {                        // NVS keys (WM-R06)
  uint16_t t_run_s = 120;
  uint8_t pwm = 128;                     // 50 %
  uint8_t manual_pwm = 128;
  float flow_ml_s = 0;                   // 0 = not calibrated
  float target_ml = 0;
};

enum class WmState : uint8_t { Idle, Run, Manual, Error };
const char* wm_state_name(WmState s);

class WmMachine {
 public:
  typedef void (*SaveFn)(const WmConfig&);
  static constexpr uint32_t kLongPressMs = 1000;
  static constexpr uint32_t kOverrunMarginMs = 30000;   // C-S03: t_run + 30 s
  static constexpr uint32_t kManualMaxMs = 300000;      // PUMP command / long press never longer than 5 min

  WmMachine(WmHal& hal, Logger& log, FaultInjector& fi, const WmConfig& cfg, const char* fw, SaveFn save = nullptr)
      : hal_(hal), log_(log), fi_(fi), cfg_(cfg), fw_(fw), save_(save) {}

  void begin();                          // C-S01: every output off first, then header line
  void tick();                           // call every 10 ms
  void handle(const Command& c);         // serial command
  WmState state() const { return state_; }
  uint8_t error() const { return err_; }
  const WmConfig& config() const { return cfg_; }
  uint32_t runCount() const { return run_; }

 private:
  void startRun(const char* why);
  void finishRun(const char* result);
  void stopAll();
  void toError(uint8_t code);
  void enter(WmState s, const char* ev, const char* detail = "");
  void shortPress();
  void startBeeps() { beepStart_ = hal_.millis(); beeping_ = true; }
  void updateBeeps();
  bool setCfg(const char* key, const char* val);
  void cfgList();
  void reply(const char* line) { log_.raw(line); }

  WmHal& hal_; Logger& log_; FaultInjector& fi_; WmConfig cfg_; const char* fw_; SaveFn save_;
  WmState state_ = WmState::Idle;
  uint8_t err_ = ERR_NONE;
  uint32_t run_ = 0, runStart_ = 0, lastD_ = 0, manualEnd_ = 0, pressStart_ = 0, beepStart_ = 0;
  bool prevBtn_ = false, longFired_ = false, ignoreDone_ = false, beeping_ = false, manualTimed_ = false;
};
