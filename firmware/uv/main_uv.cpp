// UV-AOP module firmware entry point (skeleton, desktop-tested only: the module is not built).
#include <Arduino.h>
#include "common/hal_esp32.h"
#include "common/pins_uv.h"
using namespace uv_pins;
#include "uv/uv_machine.h"

struct Esp32UvHal : UvHal {
  void dim(bool high) override { digitalWrite(PIN_DIM_GPIO, high); }
  void relay(bool on) override { digitalWrite(PIN_RELAY_IN, on); }
  void pumpPwm(int p, uint8_t v) override { ledcWrite(p == 1 ? 0 : 1, v); }
  void fanPwm(uint8_t v) override { ledcWrite(2, v); }
  void stirPwm(uint8_t v) override { ledcWrite(3, v); }
  void buzzer(bool on) override { digitalWrite(PIN_BZ, on); }
  void led(bool on) override { digitalWrite(PIN_LED, on); }
  bool doorClosed() override { return analogReadMilliVolts(PIN_IL_SENSE) > 2500; }  // B-U01: closed > 2.5 V, open < 0.5 V
  bool abortPressed() override { return digitalRead(PIN_SW2) == LOW; }              // external pull-up, 3 reads (spec 6.2) TODO
  uint32_t millis() override { return ::millis(); }
};

static Esp32UvHal gHal;
static Logger gLog(hal::serialSink);
static UvMachine* gMachine;

void setup() {
  const uint8_t outs[] = {PIN_DIM_GPIO, PIN_RELAY_IN, PIN_P_H_PWM, PIN_P_E_PWM, PIN_FAN_PWM, PIN_STIR_PWM, PIN_BZ, PIN_LED};
  hal::outputsLow(outs, 8);                      // C-S01 first: DIM low and relay off before anything else
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT);                       // GPIO39: external pull-up
  Serial.begin(115200);
  hal::pwmAttach(PIN_P_H_PWM, 0, 1000);
  hal::pwmAttach(PIN_P_E_PWM, 1, 1000);
  hal::pwmAttach(PIN_FAN_PWM, 2, 1000);
  hal::pwmAttach(PIN_STIR_PWM, 3, 1000);
  hal::watchdogStart();
  static UvMachine m(gHal, gLog, FW_COMMIT);
  gMachine = &m;
  m.begin();
}

void loop() {
  static uint32_t next = 0;
  static char line[64];
  hal::watchdogFeed();
  if (hal::readLine(line, sizeof line)) gMachine->handle(parse_command(line));
  if ((int32_t)(millis() - next) >= 0) { next += 10; gMachine->tick(); }
}
