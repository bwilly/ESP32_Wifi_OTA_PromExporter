#include "Logger.h"
#include <stdarg.h>
#include <string.h>

bool Logger::begin(const char* hostname,
                   const char* password,
                   uint16_t port,
                   size_t queueLen,
                   size_t msgSize)
{
  _msgSize = msgSize;

  // Create a queue of fixed-size message slots.
  // Each item is msgSize bytes (a null-terminated C string).
  _q = xQueueCreate(queueLen, _msgSize);
  if (!_q) return false;

  _drainBuf = (char*)malloc(_msgSize);
  if (!_drainBuf) return false;

  _dbg.begin(hostname);
  _dbg.setSerialEnabled(true);          // keep USB serial mirrored
  _dbg.setPassword(String(password));   // plaintext, but fine on trusted LAN

  // Port: API differs between forks.
  // If your library doesn't have setPort(), comment it out.
  // _dbg.setPort(port);

  // Small banner
  enqueueCstr("RemoteDebug2 logger ready\n");
  return true;
}

void Logger::handle() {
  // Must be called from loop (single owner thread)
  _dbg.handle();
}

void Logger::flush(size_t maxDrain) {
  if (!_q) return;

  for (size_t i = 0; i < maxDrain; i++) {
    // Non-blocking: only drains what's available right now
    if (xQueueReceive(_q, _drainBuf, 0) != pdTRUE) break;

    // All actual telnet/serial output happens here (loop thread only)
    _dbg.print(_drainBuf);
  }

  // Optional: occasionally report drops (also from loop thread)
  // If you want, we can add a throttled “dropped=N” line.
}

bool Logger::enqueueCstr(const char* s) {
  if (!_q || !s) return false;

  // Copy into a fixed-size slot (stack buffer -> queue item)
  // Keep this small to avoid stack blow-ups.
  // If msgSize is large, consider making this static or reduce msgSize.
  char tmp[256];

  // If msgSize > 256, we cap to 256 here. Keep msgSize <= 256 in begin().
  size_t cap = (_msgSize <= sizeof(tmp)) ? _msgSize : sizeof(tmp);
  strncpy(tmp, s, cap - 1);
  tmp[cap - 1] = '\0';

  // Non-blocking send: if queue is full, drop the message
  if (xQueueSend(_q, tmp, 0) != pdTRUE) {
    _dropped++;
    return false;
  }
  return true;
}

void Logger::log(const char* msg) { enqueueCstr(msg); }
void Logger::log(const String& msg) { enqueueCstr(msg.c_str()); }

void Logger::log(int val) {
  char b[32];
  snprintf(b, sizeof(b), "%d", val);
  enqueueCstr(b);
}

void Logger::log(unsigned long val) {
  char b[32];
  snprintf(b, sizeof(b), "%lu", val);
  enqueueCstr(b);
}

void Logger::log(float val, uint8_t decimals) {
  char b[48];
  // Arduino printf float support varies; dtostrf is safer:
  char f[32];
  dtostrf(val, 0, decimals, f);
  snprintf(b, sizeof(b), "%s", f);
  enqueueCstr(b);
}

void Logger::logf(const char* fmt, ...) {
  char b[256]; // cap formatted line size; keep <= msgSize
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  enqueueCstr(b);
}
