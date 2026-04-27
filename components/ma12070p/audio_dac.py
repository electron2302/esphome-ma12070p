import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.components.audio_dac import AudioDac
from esphome import pins

CONF_MUTE_PIN = "mute_pin"

from esphome.const import (
    CONF_ID,
    CONF_ENABLE_PIN,
    CONF_DEBUG,
)

CODEOWNERS = ["@sonocotta"]
DEPENDENCIES = ["i2c"]

# TODO: 20/26 dB gain switch
# TODO: Power profiles: 0..4, refer to datasheet, page 13
# TODO: Error sensors
# TODO: Power limiter settings: attack, release, level 
# TODO: Soft clipping settings
# TODO: MSEL configuration monitor: SE, BTL, PBTL

CONF_VOLUME_MIN = "volume_min"
CONF_VOLUME_MAX = "volume_max"
CONF_MA12070P_ID = "ma12070p_id"

ma12070p_ns = cg.esphome_ns.namespace("ma12070p")
Ma12070Component = ma12070p_ns.class_("Ma12070Component", AudioDac, cg.PollingComponent, i2c.I2CDevice)

def validate_config(config):
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Ma12070Component),
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_MUTE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_VOLUME_MAX, default="24dB"): cv.All(
                        cv.decibel, cv.int_range(-144, 24)
            ),
            cv.Optional(CONF_VOLUME_MIN, default="-103dB"): cv.All(
                        cv.decibel, cv.int_range(-144, 24)
            ),
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x20))
    .add_extra(validate_config),
    cv.only_on_esp32,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    if CONF_ENABLE_PIN in config:
        enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable))
    
    if CONF_MUTE_PIN in config:
        mute = await cg.gpio_pin_expression(config[CONF_MUTE_PIN])
        cg.add(var.set_mute_pin(mute))
    
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))
    cg.add(var.set_debug_mode(config[CONF_DEBUG]))
