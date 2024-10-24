#ifdef USE_ARDUINO

#include "telnet_server.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <list>
#include <HardwareSerial.h>

namespace esphome {
namespace telnet_server {

static const char *const TAG = "telnet_server";

TelnetServer::Client::Client(AsyncClient *client)
    : tcp_client{client}, identifier{client->remoteIP().toString().c_str()}, disconnected{false} {
  ESP_LOGD(TAG, "New client connected from %s", this->identifier.c_str());

  this->tcp_client->onError([this](void *h, AsyncClient *client, int8_t error) { this->disconnected = true; });
  this->tcp_client->onDisconnect([this](void *h, AsyncClient *client) { this->disconnected = true; });
  this->tcp_client->onTimeout([this](void *h, AsyncClient *client, uint32_t time) { this->disconnected = true; });
}

float TelnetServer::get_setup_priority() const { return esphome::setup_priority::AFTER_WIFI - 10.0f; }

void TelnetServer::set_client_count_sensor(sensor::Sensor *client_count_sensor) {
  client_count_sensor_ = client_count_sensor;
}

void TelnetServer::set_client_ip_text_sensor(text_sensor::TextSensor *client_ip_text_sensor) {
  client_ip_text_sensor_ = client_ip_text_sensor;
}

void TelnetServer::setup() {
  // server.setTimeout(10);
  server.begin();
  server.onClient(
      [this](void *h, AsyncClient *tcpClient) {
        if (tcpClient == nullptr) {
          return;
        }
        this->clients_.push_back(std::unique_ptr<Client>(new Client(tcpClient)));
        this->clients_updated_ = true;
      },
      this);

  // Ensure client sensors are updated after startup.
  clients_updated_ = true;
}

void TelnetServer::dump_config() {
  ESP_LOGCONFIG(TAG, "Config:");
  ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
  LOG_SENSOR(TAG, "  Client Count Sensor", this->client_count_sensor_);
  LOG_TEXT_SENSOR(TAG, "  Client IP Text Sensor: %s", this->client_ip_text_sensor_);
}

void TelnetServer::loop() {
  cleanup();
  readSerial();
  writeSerial();
  updateClientSensors();
}

void TelnetServer::on_shutdown() {
  for (auto &client : this->clients_) {
    client->tcp_client->close(true);
  }
}

void TelnetServer::readSerial() {
  if (Serial.available()) {
    while (Serial.available()) {
      delay(5);  // Ensure full lines are available..

      uint16_t lineStartIdx = char_idx;
      int len = Serial.readBytesUntil('\n', &buffer[char_idx], (MAXLINELENGTH - char_idx));

      // Add terminator for Verbose logging:
      buffer[char_idx + len] = '\0';
#ifdef TELNET_SERVER_VERBOSE_LOGGING
      ESP_LOGV(TAG, " %s", &buffer[lineStartIdx]);
#endif

      // Add the newline for TelNet
      char_idx += len;
      buffer[char_idx] = '\n';
      buffer[char_idx + 1] = '\0';
      char_idx++;
      len++;

      // push UART data to all connected telnet clients
      for (auto const &client : this->clients_) {
        client->tcp_client->write(&buffer[lineStartIdx], len);
      }

      // Switch buffers once the telegram is complete
      if (buffer[lineStartIdx] == '!' && len >= 6) {  // including newline!
        memset(buffer, 0, sizeof(buffer));
        char_idx = 0;
      } else if (char_idx > MAXLINELENGTH - 1) {
        ESP_LOGW(TAG, "Telegram buffer overrun!");
        // Dump buffer contents
        char_idx = MIN(char_idx, MAXLINELENGTH - 1);
        buffer[char_idx] = '\0';
#ifdef TELNET_SERVER_VERBOSE_LOGGING
        ESP_LOGV(TAG, "\n%s", buffer);
#endif
        // Clear buffer
        memset(buffer, 0, sizeof(buffer));
        char_idx = 0;
      }
    }
  }
}

void TelnetServer::writeSerial() {}

void TelnetServer::cleanup() {
  uint32_t now = millis();
  auto discriminator = [](std::unique_ptr<Client> &client) { return !client->disconnected; };
  auto last_client = std::partition(this->clients_.begin(), this->clients_.end(), discriminator);
  for (auto it = last_client; it != this->clients_.end(); it++) {
    ESP_LOGD(TAG, "Client %s disconnected @ %6d ms", (*it)->identifier.c_str(), now);
    client_disconnect_times[(*it)->identifier] = now;
    clients_updated_ = true;
  }

  this->clients_.erase(last_client, this->clients_.end());
}

void TelnetServer::updateClientSensors() {
  if (clients_updated_) {
    clients_updated_ = false;

    bool has_client_count_sensor = (this->client_count_sensor_ != nullptr);
    bool has_client_list_sensor = (this->client_ip_text_sensor_ != nullptr);

    if (has_client_count_sensor || has_client_list_sensor) {
      std::set<std::string> active_clients;
      for (const auto &client : this->clients_) {
        active_clients.insert(client->identifier);
      }

      uint32_t now = millis();
      // Iterate using iterator in for loop
      std::list<std::map<std::string, uint32_t>::const_iterator> itrs;
      for (std::map<std::string, uint32_t>::const_iterator it = client_disconnect_times.begin(); it != client_disconnect_times.end(); it++) {
        if (it->second + client_disconnect_delay >= now) {
          active_clients.insert(it->first);
        }
        else {
          ESP_LOGD(TAG, "Removing Client %s from sensors after disconnect @ %6dms, now %6dms", it->first.c_str(), it->second, now);
          itrs.push_back(it);
        }
      }
      // Remove old clients
      for (auto it: itrs) {
          client_disconnect_times.erase(it);
      }

      if (last_published_values != active_clients) {
        // store 'last published' list
        last_published_values = active_clients;

        if (has_client_count_sensor) {
          this->client_count_sensor_->publish_state(active_clients.size());
        }

        if (has_client_list_sensor) {
          std::string ip_list = "[";
          bool first = true;
          for (const auto &client_ip : active_clients) {
            if (!first) {
              ip_list += ", ";
            }
            ip_list += client_ip;
            first = false;
          }
          ip_list += "]";
          this->client_ip_text_sensor_->publish_state(ip_list.c_str());
        }
      }
    }
  }
}

}  // namespace telnet_server
}  // namespace esphome

#endif  // USE_ARDUINO
