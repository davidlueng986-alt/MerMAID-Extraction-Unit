#pragma once
// Fault injection for error-rate testing (README D, F).  Compiled in ONLY when ENABLE_FAULT_INJECTION=1
// (the *_fi build environments).  In normal builds every call below is a constant, so a stray FI command
// can never change behaviour.
#include <stdint.h>
#include <string.h>

enum class Fault : uint8_t { PumpTimeout, ScaleDisconnect, ScaleUnstable, EncoderStall, DoorOpen, TempHigh, Count };

#ifndef ENABLE_FAULT_INJECTION
#define ENABLE_FAULT_INJECTION 0
#endif

#if ENABLE_FAULT_INJECTION
class FaultInjector {
 public:
  // Arm `fault` for the next `count` checks (count<=0 clears).
  void arm(Fault f, int count) { left_[(int)f] = count > 0 ? count : 0; }
  // True if the fault is armed; consumes one count.
  bool active(Fault f) { int& n = left_[(int)f]; if (n > 0) { n--; return true; } return false; }
  static bool byName(const char* name, Fault* out) {
    static const char* names[] = {"PUMP_TIMEOUT", "SCALE_DISCONNECT", "SCALE_UNSTABLE", "ENCODER_STALL", "DOOR_OPEN", "TEMP_HIGH"};
    for (int i = 0; i < (int)Fault::Count; i++) if (!strcmp(name, names[i])) { *out = (Fault)i; return true; }
    return false;
  }
  static constexpr bool enabled() { return true; }
 private:
  int left_[(int)Fault::Count] = {};
};
#else
class FaultInjector {
 public:
  void arm(Fault, int) {}
  bool active(Fault) { return false; }
  static bool byName(const char*, Fault*) { return false; }
  static constexpr bool enabled() { return false; }
};
#endif
