#pragma once

// Only works on Arduino framework!
#ifdef USE_ARDUINO

#ifdef ARDUINO_ARCH_ESP8266
#include <ESPAsyncTCP.h>
#else
// AsyncTCP.h includes parts of freertos, which require FreeRTOS.h header to be included first
#include <freertos/FreeRTOS.h>
#include <AsyncTCP.h>
#endif

#include "esphome/core/component.h"

namespace esphome {
namespace telnet_server {

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAXLINELENGTH 1280
#define DEFAULT_MAX_CLIENTS 3
#define TELNET_PORT 23

class TelnetServer : public Component {
 public:
  TelnetServer(const uint16_t port = TELNET_PORT) : server(port) {
    // nothing to do here
  }

  float get_setup_priority() const override;

  void set_client_count_sensor(sensor::Sensor *client_count_sensor);
  void set_client_ip_text_sensor(text_sensor::TextSensor *client_ip_text_sensor);

  void setup() override;
  void loop() override;

  void on_shutdown() override;

 protected:
  void cleanup();

  void readSerial();
  void writeSerial();

  void updateClientSensors();

  struct Client {
    Client(AsyncClient *client);
    ~Client() { delete this->tcp_client; }

    AsyncClient *tcp_client{nullptr};
    std::string identifier{};
    bool disconnected{false};
  };

  AsyncServer server;
  std::vector<std::unique_ptr<Client>> clients_{};
  bool clients_updated_ = false;

  //  Set to store received telegram
  char buffer[MAXLINELENGTH];
  uint16_t char_idx = 0;

  sensor::Sensor *client_count_sensor_{nullptr};
  text_sensor::TextSensor *client_ip_text_sensor_{nullptr};
};

}  // namespace telnet_server
}  // namespace esphome

#endif  // USE_ARDUINO
