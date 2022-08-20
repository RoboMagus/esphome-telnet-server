# ESPHome Telnet-Server component

Custom component for ESPHome that implements a Telnet server, a wireless Serial bridge.

Operates on port 23 by default, supports multiple clients.

Can be configured with the following sensors:
* **client_count:** Number of currently connected clients
* **client_ips:** A list of IP addresses of the currently connected clients (text sensor)

## Usage

Requires the Arduino framework (not esp-idf).

Add the following lines to your config.

```yaml
external_components:
  - source: github://RoboMagus/esphome-telnet-server

telnet_server:
  port: 23
  client_count:
    name: "Telnet client count"
  client_ips:
    name: "Telnet client IPs"
```

## Credits
Heavily inspired by the following projects:
* [oxan/esphome-stream-server](https://github.com/oxan/esphome-stream-server)
* [thegroove/esphome-serial-server](https://github.com/thegroove/esphome-serial-server)