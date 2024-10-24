import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_PORT,
)

# ToDo: Max number of clients
CONF_CLIENT_COUNT = "client_count"
CONF_CLIENT_IPS = "client_ips"
CONF_DISCONNECT_DELAY = "disconnect_delay"

DEPENDENCIES = ["network"]
AUTO_LOAD = ["async_tcp", "sensor", "text_sensor"]

telnet_ns = cg.esphome_ns.namespace("telnet_server")
TelnetServer = telnet_ns.class_("TelnetServer", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TelnetServer),
        cv.Optional(CONF_PORT, default=23): cv.port,
        cv.Optional(CONF_CLIENT_COUNT): sensor.sensor_schema(
            unit_of_measurement=" ",
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_CLIENT_IPS): text_sensor.text_sensor_schema(),
        cv.Optional(
            CONF_DISCONNECT_DELAY, default="2500ms"
        ): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_PORT])
    await cg.register_component(var, config)

    if CONF_CLIENT_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_CLIENT_COUNT])
        cg.add(var.set_client_count_sensor(sens))

    if CONF_CLIENT_IPS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CLIENT_IPS])
        cg.add(var.set_client_ip_text_sensor(sens))

    cg.add(var.set_disconnect_delay(config[CONF_DISCONNECT_DELAY].total_milliseconds))
