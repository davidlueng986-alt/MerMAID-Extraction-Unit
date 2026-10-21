#include "sm/sm_machine.h"
#include <stdio.h>

const char* sm_state_name(SmState s) {
  static const char* n[] = {"S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "ERROR"};
  return n[(int)s];
}

void SmMachine::stopAll() {  // C-S01
  hal_.pumpPwm(1, 0);
  hal_.pumpPwm(2, 0);
  hal_.motorPwm(0);
  hal_.buzzer(false);
  hal_.led(false);
}

void SmMachine::begin() {
  stopAll();
  log_.header("SM", fw_, "skeleton=1");
  enter(SmState::S0_SelfTest, "BOOT");
}

void SmMachine::enter(SmState s, const char* ev) {
  state_ = s;
  log_.event(hal_.millis(), sm_state_name(s), ev);
}

void SmMachine::toError(uint8_t code) {
  stopAll();
  err_ = code;
  enter(SmState::Error, error_code_str(code));
}

void SmMachine::tick() {
  uint32_t now = hal_.millis();
  bool sw1 = hal_.buttonPressed(1);
  if (sw1 && !prevSw1_) pressStart_ = now;
  if (!sw1 && prevSw1_) {
    uint32_t held = now - pressStart_;
    if (state_ == SmState::Error) { err_ = ERR_NONE; enter(SmState::S0_SelfTest, "ACK"); }  // short press = confirm
    else if (held >= 2000) toError(ERR_E_ABORT);                                           // C-S06: long press = abort
    // TODO(SM-R03, SM-R08, SM-R09): short press confirms soil weighed / pulled / maintenance done
  }
  prevSw1_ = sw1;
  // TODO(SM-R01): S0 self-test (scale zero, prime P1/P2, reservoir check)
  // TODO(SM-R02): S1 cup detection 5-15 g, stable 3 s
  // TODO(SM-R03): S2 soil mass 2.0-3.0 g else E1
  // TODO(SM-R04): S3 dosing V = 10 x m_soil, P1 then P2, 90 % fast + pulses, >5 % over or timeout -> E3
  // TODO(SM-R05): S4 closed-loop 600 rpm +-5 %, t_ext timer, E2
  // TODO(SM-R06..R08): S5 settle 5 min, S6 handover/READY/E6, S7 pulled mass/E7
  // TODO(SM-R10, SM-R11): reservoir estimate E4, scale retry E5
}

void SmMachine::handle(const Command& c) {
  char b[64];
  switch (c.type) {
    case Cmd::Id: snprintf(b, sizeof b, "ID,SM,%s", fw_); log_.raw(b); break;
    case Cmd::Status: snprintf(b, sizeof b, "STATUS,%s,%u", sm_state_name(state_), err_); log_.raw(b); break;
    case Cmd::Abort: toError(ERR_E_ABORT); log_.raw("OK"); break;
    case Cmd::Log: log_.setEnabledD(c.flag); log_.raw("OK"); break;
    default: log_.raw("ERR,NOT_IMPLEMENTED"); break;  // TODO(SM-R14): CFG, TARE, CAL, PUMP, OUT
  }
}
