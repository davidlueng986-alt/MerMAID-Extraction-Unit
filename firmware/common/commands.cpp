#include "common/commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool starts(const char* s, const char* p) { return strncmp(s, p, strlen(p)) == 0; }

Command parse_command(const char* line) {
  Command c;
  while (*line == ' ') line++;
  if (!*line) return c;
  c.type = Cmd::Unknown;
  if (!strcmp(line, "ID?")) c.type = Cmd::Id;
  else if (!strcmp(line, "STATUS?")) c.type = Cmd::Status;
  else if (!strcmp(line, "CFG?")) c.type = Cmd::CfgList;
  else if (!strcmp(line, "START")) c.type = Cmd::Start;
  else if (!strcmp(line, "ABORT")) c.type = Cmd::Abort;
  else if (!strcmp(line, "TARE")) c.type = Cmd::Tare;
  else if (starts(line, "CFG ")) {
    const char* eq = strchr(line + 4, '=');
    if (eq && eq > line + 4 && (size_t)(eq - line - 4) < sizeof c.key && strlen(eq + 1) < sizeof c.value && eq[1]) {
      memcpy(c.key, line + 4, eq - line - 4);
      c.key[eq - line - 4] = 0;
      strcpy(c.value, eq + 1);
      c.type = Cmd::CfgSet;
    }
  } else if (starts(line, "CAL ")) {
    char* end;
    float g = strtof(line + 4, &end);
    if (end != line + 4 && *end == 0 && g > 0) { c.num = g; c.type = Cmd::Cal; }
  } else if (starts(line, "PUMP ")) {
    int n, pwm; float s;
    if (sscanf(line + 5, "%d %d %f", &n, &pwm, &s) == 3 && n >= 1 && pwm >= 0 && pwm <= 255 && s > 0) {
      c.pump = n; c.pwm = pwm; c.seconds = s; c.type = Cmd::Pump;
    }
  } else if (starts(line, "OUT ")) {
    char name[24]; float v;
    if (sscanf(line + 4, "%23s %f", name, &v) == 2) { strcpy(c.key, name); c.num = v; c.type = Cmd::Out; }
  } else if (!strcmp(line, "LOG 0") || !strcmp(line, "LOG 1")) {
    c.flag = line[4] == '1'; c.type = Cmd::Log;
  } else if (starts(line, "FI ")) {  // fault injection: FI <name> [count]   (only acted on by FI builds)
    char name[24]; float n = 1;
    int k = sscanf(line + 3, "%23s %f", name, &n);
    if (k >= 1) { strcpy(c.key, name); c.num = k == 2 ? n : 1; c.type = Cmd::Fi; }
  }
  return c;
}
