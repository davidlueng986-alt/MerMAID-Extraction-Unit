#pragma once
// ESP32 (Arduino-ESP32 2.x) helpers shared by the three modules.  Only included from main_*.cpp.
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "common/commands.h"

#ifndef FW_COMMIT
#define FW_COMMIT "unknown"   // set by tools/fw_version.py at build time
#endif

namespace hal {

// C-S01: the very first thing in setup().  Drive every output pin low before anything else runs.
inline void outputsLow(const uint8_t* pins, int n) {
  for (int i = 0; i < n; i++) { pinMode(pins[i], OUTPUT); digitalWrite(pins[i], LOW); }
}

inline void pwmAttach(uint8_t pin, uint8_t channel, uint32_t hz) {
  ledcSetup(channel, hz, 8);
  ledcAttachPin(pin, channel);
  ledcWrite(channel, 0);
}

// C-S02: 5 s task watchdog on the loop task.  A hang resets the chip; outputs are low again in setup().
inline void watchdogStart() {
  esp_task_wdt_init(5, true);
  esp_task_wdt_add(NULL);
}
inline void watchdogFeed() { esp_task_wdt_reset(); }
inline bool resetWasWatchdog() {
  esp_reset_reason_t r = esp_reset_reason();
  return r == ESP_RST_TASK_WDT || r == ESP_RST_INT_WDT || r == ESP_RST_WDT;
}

// Collect a serial line without blocking; returns true when a full line is ready in `buf`.
inline bool readLine(char* buf, size_t cap) {
  static size_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { buf[n] = 0; n = 0; return true; }
    if (n + 1 < cap) buf[n++] = c;
  }
  return false;
}

inline void serialSink(const char* line) { Serial.println(line); }

}  // namespace hal
