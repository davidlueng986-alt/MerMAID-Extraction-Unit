#include "common/log.h"
#include <stdio.h>

void Logger::header(const char* module, const char* fw, const char* kv) {
  char b[200];
  snprintf(b, sizeof b, "H,%s,fw=%s%s%s", module, fw, kv && *kv ? "," : "", kv ? kv : "");
  emit(b);
}
void Logger::event(uint32_t ms, const char* state, const char* ev, const char* detail) {
  char b[160];
  snprintf(b, sizeof b, "E,%lu,%s,%s,%s", (unsigned long)ms, state, ev, detail ? detail : "");
  emit(b);
}
void Logger::data(const char* f) {
  if (!dEnabled_) return;
  char b[160];
  snprintf(b, sizeof b, "D,%s", f);
  emit(b);
}
void Logger::summary(const char* f) {
  char b[200];
  snprintf(b, sizeof b, "S,%s", f);
  emit(b);
}
