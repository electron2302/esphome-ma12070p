#pragma once

namespace esphome
{
  namespace ma12070p
  {

    typedef struct
    {
      uint8_t offset;
      uint8_t value;
    } ma12070p_cfg_reg_t;

    #define MA12070P_CFG_META_DELAY 0xFF

    static const ma12070p_cfg_reg_t ma12070p_init_seq[] = {
      // TODO: add registers configred during setup
    };

  } // namespace ma12070p
} // namespace esphome