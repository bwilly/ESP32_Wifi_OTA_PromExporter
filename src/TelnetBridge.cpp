#include "TelnetBridge.h"
#include "BufferedLogger.h"
extern BufferedLogger logger;

static AsyncServer telnetServer(23);
static AsyncClient* telnetClient = nullptr;

// void initTelnetServer() {
//   // when a new client connects…
//   telnetServer.onClient([](void* arg, AsyncClient* client){
//     // if we already have one, drop the newcomer
//     if (telnetClient && telnetClient->connected()) {
//       client->close();
//       return;
//     }
//     // otherwise keep this one
//     telnetClient = client;
//     // and clear our pointer when they disconnect
//     telnetClient->onDisconnect([](void* arg, AsyncClient* c){
//       telnetClient = nullptr;
//     }, nullptr);
//   }, nullptr);

//   telnetServer.begin();
// }

void initTelnetServer() {
  telnetServer.onClient([](void* arg, AsyncClient* client){
    if (telnetClient && telnetClient->connected()) {
      client->close();
      return;
    }
    telnetClient = client;
    // replay buffer now that we have a client:
    logger.dumpBufferTo(broadcastTelnet);

    telnetClient->onDisconnect([](void*, AsyncClient* c){
      telnetClient = nullptr;
    }, nullptr);
  }, nullptr);

  telnetServer.begin();
}

void broadcastTelnet(const char* data, size_t len) {
  if (telnetClient && telnetClient->connected()) {
    // telnetClient->write((const uint8_t*)data, len);
    telnetClient->write(data);
  }
}
