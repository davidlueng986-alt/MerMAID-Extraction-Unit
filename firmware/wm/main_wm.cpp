// Water module firmware entry point (ESP32-DevKitC).  Logic lives in wm_machine.cpp.
#include <Arduino.h>
#include <Preferences.h>
#include "common/hal_esp32.h"
#include "common/pins_wm.h"
using namespace wm_pins;
#include "wm/wm_machine.h"

static const uint8_t kPumpCh = 0;

struct Esp32WmHal : WmHal {
  void pumpPwm(uint8_t v) override { ledcWrite(kPumpCh, v); }
  void led(bool on) override { digitalWrite(PIN_LED, on); }
  void buzzer(bool on) override { digitalWrite(PIN_BZ, on); }
  bool buttonPressed() override { return digitalRead(PIN_SW1) == LOW; }  // internal pull-up
  uint32_t millis() override { return ::millis(); }
};

static Esp32WmHal gHal;
static Logger gLog(hal::serialSink);
static FaultInjector gFi;
static Preferences gPrefs;
static WmMachine* gMachine;
static WmConfig gCfg;

static void saveCfg(const WmConfig& c) {
  gPrefs.putUShort("t_run_s", c.t_run_s);
  gPrefs.putUChar("pwm", c.pwm);
  gPrefs.putUChar("manual_pwm", c.manual_pwm);
  gPrefs.putFloat("flow_ml_s", c.flow_ml_s);
  gPrefs.putFloat("target_ml", c.target_ml);
}

void setup() {
  const uint8_t outs[] = {PIN_PUMP_PWM, PIN_BZ, PIN_LED};
  hal::outputsLow(outs, 3);                      // C-S01 first
  pinMode(PIN_SW1, INPUT_PULLUP);
  Serial.begin(115200);
  hal::pwmAttach(PIN_PUMP_PWM, kPumpCh, 1000);   // pump 1 kHz
  gPrefs.begin("wm", false);
  gCfg.t_run_s = gPrefs.getUShort("t_run_s", gCfg.t_run_s);
  gCfg.pwm = gPrefs.getUChar("pwm", gCfg.pwm);
  gCfg.manual_pwm = gPrefs.getUChar("manual_pwm", gCfg.manual_pwm);
  gCfg.flow_ml_s = gPrefs.getFloat("flow_ml_s", gCfg.flow_ml_s);
  gCfg.target_ml = gPrefs.getFloat("target_ml", gCfg.target_ml);
  hal::watchdogStart();
  static WmMachine m(gHal, gLog, gFi, gCfg, FW_COMMIT, saveCfg);
  gMachine = &m;
  m.begin();
  if (hal::resetWasWatchdog()) gLog.event(millis(), "IDLE", "E-WDT", "restart after watchdog");  // C-S02: no auto-resume
}

void loop() {
  static uint32_t next = 0;
  static char line[64];
  hal::watchdogFeed();
  if (hal::readLine(line, sizeof line)) gMachine->handle(parse_command(line));
  if ((int32_t)(millis() - next) >= 0) {         // 10 ms tick, no delay()
    next += 10;
    gMachine->tick();
  }
}
