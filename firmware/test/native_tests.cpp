// Desktop tests (no ESP32): g++ -I firmware ... ; run with tools/run_native_tests.sh
// Build twice: normal, and with -DENABLE_FAULT_INJECTION=1 (FI tests are compiled only then).
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include "common/commands.h"
#include "common/error_codes.h"
#include "common/pins_wm.h"
#include "common/pins_uv.h"
#include "sm/sm_machine.h"
#include "uv/uv_machine.h"
#include "wm/wm_machine.h"

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static std::vector<std::string> out;
static void sink(const char* l) { out.push_back(l); }
static bool has(const char* prefix) { for (auto& s : out) if (s.rfind(prefix, 0) == 0) return true; return false; }
static std::string find(const char* prefix) { for (auto& s : out) if (s.rfind(prefix, 0) == 0) return s; return ""; }

struct FakeWm : WmHal {
  uint32_t t = 0; uint8_t pump = 99; bool led_ = true, bz = true, btn = false;
  int pumpWrites = 0;
  void pumpPwm(uint8_t v) override { pump = v; pumpWrites++; }
  void led(bool o) override { led_ = o; }
  void buzzer(bool o) override { bz = o; }
  bool buttonPressed() override { return btn; }
  uint32_t millis() override { return t; }
};

struct Rig {
  FakeWm hal; Logger log{sink}; FaultInjector fi; WmConfig cfg; WmMachine m;
  explicit Rig(uint16_t t_run = 5) : m(hal, log, fi, [&] { cfg.t_run_s = t_run; return cfg; }(), "test123") { out.clear(); m.begin(); }
  void run_ms(uint32_t ms) { for (uint32_t i = 0; i < ms; i += 10) { hal.t += 10; m.tick(); } }
  void press(uint32_t held) { hal.btn = true; run_ms(held); hal.btn = false; run_ms(20); }
};

static void test_wm() {
  { Rig r; CHECK(r.hal.pump == 0 && !r.hal.led_ && !r.hal.bz);  // C-S01
    CHECK(has("H,WM,fw=test123,t_run_s=5")); CHECK(r.m.state() == WmState::Idle); }
  { Rig r(5); r.press(100);  // short press starts
    CHECK(r.m.state() == WmState::Run); CHECK(r.hal.pump == 128);
    r.run_ms(4900); CHECK(r.m.state() == WmState::Run);
    r.run_ms(300); CHECK(r.m.state() == WmState::Idle); CHECK(r.hal.pump == 0);
    std::string s = find("S,");  // run,t_run_s,pwm,start_ms,end_ms,result
    unsigned run, pwm; float tr; unsigned long st, en; char res[8];
    CHECK(sscanf(s.c_str(), "S,%u,%f,%u,%lu,%lu,%7s", &run, &tr, &pwm, &st, &en, res) == 6);
    CHECK(run == 1 && pwm == 128 && std::string(res) == "OK");
    CHECK(en - st >= 5000 && en - st <= 5100);  // WM-R02: within 0.1 s
    CHECK(has("D,"));  // per-second D lines
    }
  { Rig r(5); r.press(100); r.run_ms(1000); r.press(100);  // short press during run = abort
    CHECK(r.m.state() == WmState::Error); CHECK(r.hal.pump == 0); CHECK(r.m.error() == ERR_E_ABORT);
    CHECK(find("S,").find("ABORT") != std::string::npos);
    r.press(100); CHECK(r.m.state() == WmState::Idle);  // ack with a key press only (C-S05
  }
  { Rig r; r.hal.btn = true; r.run_ms(1200);  // long press = manual pump while held
    CHECK(r.m.state() == WmState::Manual); CHECK(r.hal.pump == 128);
    r.hal.btn = false; r.run_ms(20); CHECK(r.m.state() == WmState::Idle); CHECK(r.hal.pump == 0); }
  { Rig r(3); r.press(100); r.run_ms(3050);  // beeps twice after done
    int on_edges = 0; bool prev = r.hal.bz;
    for (int i = 0; i < 100; i++) { r.run_ms(10); if (r.hal.bz && !prev) on_edges++; prev = r.hal.bz; }
    CHECK(on_edges + (r.hal.bz ? 0 : 0) >= 1); }
  { Rig r(120); out.clear();  // cfg: target / flow -> t_run (WM-R04)
    r.m.handle(parse_command("CFG flow_ml_s=0.1")); r.m.handle(parse_command("CFG target_ml=10"));
    CHECK(r.m.config().t_run_s == 100); CHECK(has("OK"));
    r.m.handle(parse_command("CFG pwm=300")); CHECK(has("ERR,BAD_CFG"));
    r.m.handle(parse_command("PUMP 1 200 2")); CHECK(r.hal.pump == 200); r.run_ms(2100); CHECK(r.hal.pump == 0); }
  { Rig r; out.clear(); r.m.handle(parse_command("FI PUMP_TIMEOUT 1"));
#if ENABLE_FAULT_INJECTION
    CHECK(has("OK"));
    r.press(100); r.run_ms(6000); CHECK(r.m.state() == WmState::Run);  // done check suppressed ...
    r.run_ms(31000); CHECK(r.m.state() == WmState::Error); CHECK(r.m.error() == ERR_E_WM1); CHECK(r.hal.pump == 0);  // ... C-S03 stops the pump
#else
    CHECK(has("ERR,FI_DISABLED"));
    r.press(100); r.run_ms(5300); CHECK(r.m.state() == WmState::Idle);  // injection has no effect in normal builds
#endif
  }
}

static void test_parser() {
  CHECK(parse_command("ID?").type == Cmd::Id);
  CHECK(parse_command("CFG t_run_s=125").type == Cmd::CfgSet);
  CHECK(std::string(parse_command("CFG t_run_s=125").key) == "t_run_s");
  CHECK(parse_command("CFG =1").type == Cmd::Unknown);
  Command c = parse_command("PUMP 1 128 10"); CHECK(c.type == Cmd::Pump && c.pump == 1 && c.pwm == 128 && c.seconds == 10);
  CHECK(parse_command("PUMP 1 300 10").type == Cmd::Unknown);
  CHECK(parse_command("CAL 20.000").type == Cmd::Cal && parse_command("CAL 20.000").num == 20.0f);
  CHECK(parse_command("CAL -3").type == Cmd::Unknown);
  CHECK(parse_command("LOG 1").flag && !parse_command("LOG 0").flag);
  CHECK(parse_command("").type == Cmd::None);
  CHECK(parse_command("RM -RF").type == Cmd::Unknown);
}

static void test_errors_and_pins() {
  CHECK(std::string(error_code_str(ERR_E_UV1)) == "E-UV1");
  CHECK(std::string(error_code_str(ERR_E5)) == "E5");
  for (unsigned i = 0; i < ERROR_TABLE_LEN; i++) for (unsigned j = i + 1; j < ERROR_TABLE_LEN; j++) CHECK(ERROR_TABLE[i].id != ERROR_TABLE[j].id);
  CHECK(wm_pins::PIN_PUMP_PWM == 25 && wm_pins::PIN_SW1 == 32);  // main-board connection table, WM
  CHECK(uv_pins::PIN_DIM_GPIO == 13 && uv_pins::PIN_RELAY_IN == 27);  // main-board connection table, UV
}

struct FakeSm : SmHal {
  uint32_t t = 0; int p1 = 9, p2 = 9, mot = 9; bool bz = true, led_ = true; bool sw1 = false;
  void pumpPwm(int p, uint8_t v) override { (p == 1 ? p1 : p2) = v; }
  void motorPwm(uint8_t v) override { mot = v; }
  void buzzer(bool o) override { bz = o; }
  void led(bool o) override { led_ = o; }
  bool buttonPressed(int sw) override { return sw == 1 && sw1; }
  uint32_t millis() override { return t; }
};
struct FakeUv : UvHal {
  uint32_t t = 0; std::vector<std::string> calls; bool door = true, ab = false;
  void dim(bool h) override { calls.push_back(h ? "dim=1" : "dim=0"); }
  void relay(bool o) override { calls.push_back(o ? "relay=1" : "relay=0"); }
  void pumpPwm(int, uint8_t) override {} void fanPwm(uint8_t) override {} void stirPwm(uint8_t) override {}
  void buzzer(bool) override {} void led(bool) override {}
  bool doorClosed() override { return door; } bool abortPressed() override { return ab; }
  uint32_t millis() override { return t; }
};

static void test_sm_uv() {
  { FakeSm h; Logger l{sink}; SmMachine m(h, l, "t"); out.clear(); m.begin();
    CHECK(h.p1 == 0 && h.p2 == 0 && h.mot == 0 && !h.bz && !h.led_); CHECK(m.state() == SmState::S0_SelfTest);
    h.sw1 = true; for (int i = 0; i < 250; i++) { h.t += 10; m.tick(); } h.sw1 = false; h.t += 10; m.tick();  // 2 s hold = abort
    CHECK(m.state() == SmState::Error);
    h.sw1 = true; h.t += 10; m.tick(); h.sw1 = false; h.t += 100; m.tick(); CHECK(m.state() == SmState::S0_SelfTest); }
  { FakeUv h; Logger l{sink}; UvMachine m(h, l, "t"); out.clear(); m.begin();
    CHECK(h.calls.size() >= 2 && h.calls[0] == "dim=0" && h.calls[1] == "relay=0");  // UV-R12 order, also at boot
    h.calls.clear(); h.ab = true; h.t += 10; m.tick(); CHECK(m.state() == UvState::Error);
    CHECK(h.calls.size() >= 2 && h.calls[0] == "dim=0" && h.calls[1] == "relay=0"); }
}

int main() {
  test_wm(); test_parser(); test_errors_and_pins(); test_sm_uv();
  printf("%d checks, %d failed (fault injection %s)\n", checks, fails, ENABLE_FAULT_INJECTION ? "ON" : "off");
  return fails ? 1 : 0;
}
