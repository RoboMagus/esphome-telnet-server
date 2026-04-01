#pragma once

#include "esphome/core/component.h"
#include "esphome/components/async_tcp/async_tcp.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#include <set>
#include <map>

namespace esphome {
namespace telnet_server {

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifndef TELNET_SERVER_BUFFER_SIZE
#define TELNET_SERVER_BUFFER_SIZE 1280
#endif
#define DEFAULT_MAX_CLIENTS 3

class TelnetServer : public Component {
 public:
  TelnetServer(const uint16_t port) : server(port) {
    this->port_ = port;
  }

  float get_setup_priority() const override;

  void set_uart(uart::UARTComponent *uart);
  void set_client_count_sensor(sensor::Sensor *client_count_sensor);
  void set_client_ip_text_sensor(text_sensor::TextSensor *client_ip_text_sensor);

  void set_disconnect_delay(uint32_t disconnect_delay) { this->client_disconnect_delay = disconnect_delay; }

  void setup() override;
  void dump_config() override;
  void loop() override;
  void on_shutdown() override;

 protected:
  void cleanup();

  void readSerial();

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
  std::map<std::string, uint32_t> client_disconnect_times{};
  std::set<std::string> last_published_values{};
  bool clients_updated_ = false;
  uint16_t port_;

  uint32_t client_disconnect_delay = 5000; // ms

  //  Set to store received telegram
  char buffer[TELNET_SERVER_BUFFER_SIZE];
  uint16_t char_idx = 0;

  sensor::Sensor *client_count_sensor_{nullptr};
  text_sensor::TextSensor *client_ip_text_sensor_{nullptr};
  uart::UARTComponent *uart_{nullptr};
};

}  // namespace telnet_server
}  // namespace esphome

