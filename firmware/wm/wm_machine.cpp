#include "wm/wm_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* wm_state_name(WmState s) {
  switch (s) {
    case WmState::Idle: return "IDLE";
    case WmState::Run: return "RUN";
    case WmState::Manual: return "MANUAL";
    default: return "ERROR";
  }
}

void WmMachine::stopAll() {
  hal_.pumpPwm(0);
  hal_.led(false);
  hal_.buzzer(false);
  beeping_ = false;
}

void WmMachine::begin() {
  stopAll();  // C-S01 comes before anything else
  char kv[96];
  snprintf(kv, sizeof kv, "t_run_s=%u,pwm=%u,manual_pwm=%u,flow_ml_s=%.4f,target_ml=%.2f", cfg_.t_run_s, cfg_.pwm,
           cfg_.manual_pwm, cfg_.flow_ml_s, cfg_.target_ml);
  log_.header("WM", fw_, kv);
  prevBtn_ = hal_.buttonPressed();
  enter(WmState::Idle, "BOOT");
}

void WmMachine::enter(WmState s, const char* ev, const char* detail) {
  state_ = s;
  log_.event(hal_.millis(), wm_state_name(s), ev, detail);
}

void WmMachine::toError(uint8_t code) {
  stopAll();
  err_ = code;
  enter(WmState::Error, error_code_str(code));
}

void WmMachine::startRun(const char* why) {
  if (state_ != WmState::Idle) return;
  run_++;
  runStart_ = hal_.millis();
  lastD_ = runStart_;
  ignoreDone_ = fi_.active(Fault::PumpTimeout);  // fault injection: completion check "breaks", C-S03 must catch it
  err_ = ERR_NONE;
  hal_.pumpPwm(cfg_.pwm);
  enter(WmState::Run, "START", why);
}

void WmMachine::finishRun(const char* result) {
  uint32_t now = hal_.millis();
  hal_.pumpPwm(0);
  hal_.led(false);
  char b[96];
  snprintf(b, sizeof b, "%lu,%u.0,%u,%lu,%lu,%s", (unsigned long)run_, cfg_.t_run_s, cfg_.pwm, (unsigned long)runStart_,
           (unsigned long)now, result);
  log_.summary(b);
}

void WmMachine::shortPress() {
  switch (state_) {
    case WmState::Idle: startRun("BUTTON"); break;
    case WmState::Run: finishRun("ABORT"); toError(ERR_E_ABORT); break;  // WM-R01 / C-S06
    case WmState::Error: err_ = ERR_NONE; enter(WmState::Idle, "ACK"); break;  // C-S05
    default: break;
  }
}

void WmMachine::updateBeeps() {
  if (!beeping_) return;
  uint32_t phase = (hal_.millis() - beepStart_) / 200;  // on, off, on, off
  if (phase >= 4) { hal_.buzzer(false); beeping_ = false; return; }
  hal_.buzzer((phase & 1) == 0);
}

void WmMachine::tick() {
  uint32_t now = hal_.millis();
  bool btn = hal_.buttonPressed();
  if (btn && !prevBtn_) { pressStart_ = now; longFired_ = false; }
  // long press while idle: manual pump while held (WM-R03)
  if (btn && state_ == WmState::Idle && !longFired_ && now - pressStart_ >= kLongPressMs) {
    longFired_ = true;
    manualTimed_ = false;
    manualEnd_ = now + kManualMaxMs;
    hal_.pumpPwm(cfg_.manual_pwm);
    enter(WmState::Manual, "MANUAL_ON");
  }
  if (!btn && prevBtn_) {
    if (state_ == WmState::Manual && !manualTimed_) {
      hal_.pumpPwm(0);
      enter(WmState::Idle, "MANUAL_OFF");
    } else if (!longFired_ && now - pressStart_ < kLongPressMs) {
      shortPress();
    }
  }
  prevBtn_ = btn;

  if (state_ == WmState::Run) {
    uint32_t el = now - runStart_;
    hal_.led((el % 1000) < 500);  // WM-R02: LED once per second
    if (now - lastD_ >= 1000) {
      lastD_ += 1000;
      char b[64];
      snprintf(b, sizeof b, "%lu,RUN,%u,%lu", (unsigned long)now, cfg_.pwm, (unsigned long)(el / 1000));
      log_.data(b);
    }
    if (!ignoreDone_ && el >= (uint32_t)cfg_.t_run_s * 1000UL) {
      finishRun("OK");
      enter(WmState::Idle, "DONE");
      startBeeps();
    } else if (el > (uint32_t)cfg_.t_run_s * 1000UL + kOverrunMarginMs) {  // C-S03 hard limit
      finishRun("ERR");
      toError(ERR_E_WM1);
    }
  } else if (state_ == WmState::Manual && (now >= manualEnd_)) {
    hal_.pumpPwm(0);
    enter(WmState::Idle, manualTimed_ ? "PUMP_DONE" : "MANUAL_LIMIT");
  }
  updateBeeps();
}

bool WmMachine::setCfg(const char* k, const char* v) {
  char* end;
  double x = strtod(v, &end);
  if (end == v || *end) return false;
  if (!strcmp(k, "t_run_s") && x >= 1 && x <= 3600) cfg_.t_run_s = (uint16_t)x;
  else if (!strcmp(k, "pwm") && x >= 0 && x <= 255) cfg_.pwm = (uint8_t)x;
  else if (!strcmp(k, "manual_pwm") && x >= 0 && x <= 255) cfg_.manual_pwm = (uint8_t)x;
  else if (!strcmp(k, "flow_ml_s") && x >= 0) cfg_.flow_ml_s = (float)x;
  else if (!strcmp(k, "target_ml") && x >= 0) cfg_.target_ml = (float)x;
  else return false;
  if ((!strcmp(k, "flow_ml_s") || !strcmp(k, "target_ml")) && cfg_.flow_ml_s > 0 && cfg_.target_ml > 0) {
    double t = cfg_.target_ml / cfg_.flow_ml_s;  // WM-R04
    if (t >= 1 && t <= 3600) cfg_.t_run_s = (uint16_t)(t + 0.5);
  }
  if (save_) save_(cfg_);
  return true;
}

void WmMachine::cfgList() {
  char b[140];
  snprintf(b, sizeof b, "CFG,t_run_s=%u,pwm=%u,manual_pwm=%u,flow_ml_s=%.4f,target_ml=%.2f", cfg_.t_run_s, cfg_.pwm,
           cfg_.manual_pwm, cfg_.flow_ml_s, cfg_.target_ml);
  reply(b);
}

void WmMachine::handle(const Command& c) {
  char b[96];
  switch (c.type) {
    case Cmd::Id: snprintf(b, sizeof b, "ID,WM,%s", fw_); reply(b); break;
    case Cmd::Status: snprintf(b, sizeof b, "STATUS,%s,%u", wm_state_name(state_), err_); reply(b); break;
    case Cmd::CfgList: cfgList(); break;
    case Cmd::CfgSet: reply(setCfg(c.key, c.value) ? "OK" : "ERR,BAD_CFG"); break;
    case Cmd::Start:
      if (state_ == WmState::Idle) { startRun("CMD"); reply("OK"); } else reply("ERR,BUSY");
      break;
    case Cmd::Abort:
      if (state_ == WmState::Run) { finishRun("ABORT"); toError(ERR_E_ABORT); reply("OK"); }
      else if (state_ == WmState::Manual) { hal_.pumpPwm(0); enter(WmState::Idle, "ABORT"); reply("OK"); }
      else reply("ERR,NOT_RUNNING");
      break;
    case Cmd::Pump:  // idle only (spec 6.5); one pump on the WM
      if (state_ != WmState::Idle || c.pump != 1) { reply("ERR,BUSY_OR_BAD_PUMP"); break; }
      manualTimed_ = true;
      manualEnd_ = hal_.millis() + (uint32_t)(c.seconds * 1000);
      if (manualEnd_ - hal_.millis() > kManualMaxMs) manualEnd_ = hal_.millis() + kManualMaxMs;
      hal_.pumpPwm((uint8_t)c.pwm);
      enter(WmState::Manual, "PUMP", "");
      reply("OK");
      break;
    case Cmd::Log: log_.setEnabledD(c.flag); reply("OK"); break;
    case Cmd::Fi: {
      Fault f;
      if (!FaultInjector::enabled()) { reply("ERR,FI_DISABLED"); break; }
      if (FaultInjector::byName(c.key, &f)) { fi_.arm(f, (int)c.num); reply("OK"); } else reply("ERR,UNKNOWN_FAULT");
      break;
    }
    case Cmd::Tare: case Cmd::Cal: case Cmd::Out: reply("ERR,NOT_SUPPORTED_ON_WM"); break;
    default: reply("ERR,UNKNOWN"); break;
  }
}
