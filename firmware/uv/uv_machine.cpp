#include "uv/uv_machine.h"
#include <stdio.h>

const char* uv_state_name(UvState s) {
  static const char* n[] = {"U0", "U1", "U2", "U3", "U4", "U5", "U6", "U7", "ERROR"};
  return n[(int)s];
}

void UvMachine::lampOff() {
  hal_.dim(false);   // order matters (UV-R12): light off first ...
  hal_.relay(false); // ... then cut the supply
}

void UvMachine::stopAll() {  // C-S01
  lampOff();
  hal_.pumpPwm(1, 0);
  hal_.pumpPwm(2, 0);
  hal_.fanPwm(0);
  hal_.stirPwm(0);
  hal_.buzzer(false);
  hal_.led(false);
}

void UvMachine::begin() {
  stopAll();
  log_.header("UV", fw_, "skeleton=1");
  enter(UvState::U0_SelfTest, "BOOT");
}

void UvMachine::enter(UvState s, const char* ev) {
  state_ = s;
  log_.event(hal_.millis(), uv_state_name(s), ev);
}

void UvMachine::toError(uint8_t code) {
  stopAll();
  err_ = code;
  enter(UvState::Error, error_code_str(code));
}

void UvMachine::tick() {
  bool ab = hal_.abortPressed();
  if (ab && !prevAbort_) toError(ERR_E_ABORT);  // UV-R13: SW2 in any state
  prevAbort_ = ab;
  // Door opened while the relay could be on: lamp off immediately (UV-R07).  Cheap and safe even in the skeleton.
  if (!hal_.doorClosed() && state_ == UvState::U5_Irradiate) toError(ERR_E_UV1);
  // TODO(UV-R01..R04): U0 self-test, U1 tube detection, U2 filtrate 8-12 g, U3 start
  // TODO(UV-R05): U4 H2O2 dosing 1/9 of filtrate
  // TODO(UV-R06..R09): U5 irradiation: door check, relay then 100 ms then DIM, timer, ring temperature, dose
  // TODO(UV-R10, UV-R11): U6 neutralisation, U7 done
}

void UvMachine::handle(const Command& c) {
  char b[64];
  switch (c.type) {
    case Cmd::Id: snprintf(b, sizeof b, "ID,UV,%s", fw_); log_.raw(b); break;
    case Cmd::Status: snprintf(b, sizeof b, "STATUS,%s,%u", uv_state_name(state_), err_); log_.raw(b); break;
    case Cmd::Abort: toError(ERR_E_ABORT); log_.raw("OK"); break;
    case Cmd::Log: log_.setEnabledD(c.flag); log_.raw("OK"); break;
    default: log_.raw("ERR,NOT_IMPLEMENTED"); break;
  }
}
