#ifdef USE_ARDUINO

#include "telnet_server.h"
#include "esphome/core/log.h"
#include "esphome.h"

namespace esphome {
namespace telnet_server {

static const char *const TAG = "TELNET";

TelnetServer::Client::Client(AsyncClient *client)
    : tcp_client{client}, identifier{client->remoteIP().toString().c_str()}, disconnected{false} 
{
    ESP_LOGD(TAG, "New client connected from %s", this->identifier.c_str());

    this->tcp_client->onError([this](void *h, AsyncClient *client, int8_t error) { this->disconnected = true; });
    this->tcp_client->onDisconnect([this](void *h, AsyncClient *client) { this->disconnected = true; });
    this->tcp_client->onTimeout([this](void *h, AsyncClient *client, uint32_t time) { this->disconnected = true; });
}

float TelnetServer::get_setup_priority() const { 
    return esphome::setup_priority::AFTER_WIFI; 
}

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
        delay(5);  // Ensure full lines are available..
        while (Serial.available()) {
            uint16_t lineStartIdx = char_idx;
            int len = Serial.readBytesUntil('\n', &buffer[char_idx], (MAXLINELENGTH - char_idx));

            // Add terminator for Verbose logging:
            buffer[char_idx + len] = '\0';
            ESP_LOGV(TAG, ":: %s", &buffer[lineStartIdx]);

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
                ESP_LOGV(TAG, "\n%s", buffer);
                // Clear buffer
                memset(buffer, 0, sizeof(buffer));
                char_idx = 0;
            }
        }
    }
}

void TelnetServer::writeSerial() {

}

void TelnetServer::cleanup() {
    auto discriminator = [](std::unique_ptr<Client> &client) { return !client->disconnected; };
    auto last_client = std::partition(this->clients_.begin(), this->clients_.end(), discriminator);
    for (auto it = last_client; it != this->clients_.end(); it++) {
        ESP_LOGD(TAG, "Client %s disconnected", (*it)->identifier.c_str());
        clients_updated_ = true;
    }

    this->clients_.erase(last_client, this->clients_.end());
}

void TelnetServer::updateClientSensors() {
    if (clients_updated_) {
        clients_updated_ = false;

        if (this->client_count_sensor_ != nullptr) {
            int numClients = clients_.size();
            this->client_count_sensor_->publish_state(numClients);
        }

        if (this->client_ip_text_sensor_ != nullptr) {
            std::string ip_list = "[";
            for (const auto &client : this->clients_) {
                if (&client != &(this->clients_)[0]) {
                    ip_list += ", ";
                }
                ip_list += client->identifier;
            }
            ip_list += "]";
            this->client_ip_text_sensor_->publish_state(ip_list.c_str());
        }
    }
}

}  // namespace telnet_server
}  // namespace esphome

#endif  // USE_ARDUINO
