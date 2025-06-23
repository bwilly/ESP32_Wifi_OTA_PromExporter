// BufferedLogger.h
#ifndef BUFFEREDLOGGER_H
#define BUFFEREDLOGGER_H

#include <Arduino.h>

// Adjust buffer size as needed
#define LOG_BUFFER_SIZE 200

class BufferedLogger {
private:
  String buf[LOG_BUFFER_SIZE];  // circular buffer storage
  int head = 0;                  // next write index
  int tail = 0;                  // next read index
  int count = 0;                 // number of stored entries

  // core printf-style formatter
  void vlog(const char* fmt, va_list ap);

public:
  BufferedLogger() = default;

  // printf-style logging
  void logf(const char* fmt, ...);

  // simple overloads
  void log(const char* msg);
  void log(const String& msg);
  void log(int val);
  void log(unsigned long val);
  void log(float val);

  void dumpBufferTo(void (*sink)(const char*, size_t));

};



// Global instance
extern BufferedLogger logger;

#endif // BUFFEREDLOGGER_H