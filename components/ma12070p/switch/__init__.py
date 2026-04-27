import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_SWITCH

CONF_ENABLE_DAC = "enable_dac"

from ..audio_dac import CONF_MA12070P_ID, Ma12070Component, ma12070p_ns

EnableDacSwitch = ma12070p_ns.class_("EnableDacSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MA12070P_ID): cv.use_id(Ma12070Component),

        cv.Optional(CONF_ENABLE_DAC): switch.switch_schema(
            EnableDacSwitch,
            device_class=DEVICE_CLASS_SWITCH,
        )
        .extend(cv.COMPONENT_SCHEMA)
    }
)

async def to_code(config):
  ma12070p_component = await cg.get_variable(config[CONF_MA12070P_ID])
  if enable_dac_config := config.get(CONF_ENABLE_DAC):
    s = await switch.new_switch(enable_dac_config)
    await cg.register_component(s, enable_dac_config)
    await cg.register_parented(s, ma12070p_component)

