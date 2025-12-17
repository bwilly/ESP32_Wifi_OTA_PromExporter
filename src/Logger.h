#pragma once
#include <Arduino.h>
#include <RemoteDebug.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class Logger {
public:
  // queueLen: number of queued messages
  // msgSize: max bytes per message INCLUDING null terminator
  bool begin(const char* hostname,
             const char* password,
             uint16_t port = 23,
             size_t queueLen = 32,
             size_t msgSize = 192);

  // Call in loop() frequently
  void handle();

  // Call in loop() frequently (after handle())
  // maxDrain: how many queued messages to drain per call
  void flush(size_t maxDrain = 16);

  // Enqueue-only API (safe from any task)
  void log(const char* msg);
  void log(const String& msg);
  void log(int val);
  void log(unsigned long val);
  void log(float val, uint8_t decimals = 2);
  void logf(const char* fmt, ...);

  // Stats
  uint32_t dropped() const { return _dropped; }

private:
  bool enqueueCstr(const char* s);

  RemoteDebug _dbg;
  QueueHandle_t _q = nullptr;
  size_t _msgSize = 0;
  uint32_t _dropped = 0;

  // One reusable buffer used ONLY in loop() for draining
  char* _drainBuf = nullptr;
};
