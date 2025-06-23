#pragma once
#include <AsyncTCP.h>

// Call once (after Wi-Fi is up) to start listening on port 23
void initTelnetServer();

// Call from your logger to push raw bytes to the one (or zero) connected client
void broadcastTelnet(const char* data, size_t len);
