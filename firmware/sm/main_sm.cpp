// Soil module firmware entry point (skeleton).  Logic lives in sm_machine.cpp.
#include <Arduino.h>
#include "common/hal_esp32.h"
#include "common/pins_sm.h"
using namespace sm_pins;
#include "sm/sm_machine.h"

struct Esp32SmHal : SmHal {
  void pumpPwm(int p, uint8_t v) override { ledcWrite(p == 1 ? 0 : 1, v); }
  void motorPwm(uint8_t v) override { ledcWrite(2, v); }
  void buzzer(bool on) override { digitalWrite(PIN_BZ, on); }
  void led(bool on) override { digitalWrite(PIN_LED, on); }
  bool buttonPressed(int sw) override { return digitalRead(sw == 1 ? PIN_SW1 : PIN_SW2) == LOW; }
  uint32_t millis() override { return ::millis(); }
};

static Esp32SmHal gHal;
static Logger gLog(hal::serialSink);
static SmMachine* gMachine;

void setup() {
  const uint8_t outs[] = {PIN_P1_PWM, PIN_P2_PWM, PIN_MOT_PWM, PIN_AIN1, PIN_AIN2, PIN_BZ, PIN_LED};
  hal::outputsLow(outs, 7);                      // C-S01 first
  digitalWrite(PIN_AIN1, HIGH);                  // fixed direction (spec 4.x: forward = encoder counts up)
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  Serial.begin(115200);
  hal::pwmAttach(PIN_P1_PWM, 0, 1000);
  hal::pwmAttach(PIN_P2_PWM, 1, 1000);
  hal::pwmAttach(PIN_MOT_PWM, 2, 20000);         // TB6612 20 kHz
  hal::watchdogStart();
  static SmMachine m(gHal, gLog, FW_COMMIT);
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
