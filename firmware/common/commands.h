#pragma once
// Serial command parser (spec 11 / 6.5).  Pure parsing, no hardware.
#include <stdint.h>

enum class Cmd : uint8_t { None, Id, Status, CfgList, CfgSet, Start, Abort, Tare, Cal, Pump, Out, Log, Fi, Unknown };

struct Command {
  Cmd type = Cmd::None;
  char key[24] = "";      // CFG key, OUT name, FI name
  char value[24] = "";    // CFG value
  float num = 0;          // CAL grams, OUT value, FI count
  int pump = 0;           // PUMP n
  int pwm = 0;            // PUMP pwm (0-255)
  float seconds = 0;      // PUMP seconds
  bool flag = false;      // LOG 0/1
};

// Parse one line (no newline).  Unknown or malformed input gives Cmd::Unknown.
Command parse_command(const char* line);
