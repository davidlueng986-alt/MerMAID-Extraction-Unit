#pragma once
// Serial log lines, 115200 baud, one record per line, first field is the type (spec 11 / 6.4):
//   H boot header   D per-second data   E event / error   S run summary
#include <stdint.h>
#include <stddef.h>

typedef void (*LogSink)(const char* line);

class Logger {
 public:
  explicit Logger(LogSink sink) : sink_(sink) {}
  void setEnabledD(bool on) { dEnabled_ = on; }
  bool enabledD() const { return dEnabled_; }
  void header(const char* module, const char* fw, const char* kvList);  // "k=v,k=v"
  void event(uint32_t ms, const char* state, const char* event, const char* detail = "");
  void data(const char* fields);                                         // caller formats the D line body
  void summary(const char* fields);
  void raw(const char* line) { emit(line); }  // command replies (ID,/STATUS,/CFG,/ERR,)
 private:
  void emit(const char* line) { if (sink_) sink_(line); }
  LogSink sink_;
  bool dEnabled_ = true;
};
