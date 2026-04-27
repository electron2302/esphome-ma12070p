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

    // Register addresses (MA12070P / MA12040P family)
    #define MA12070P_REG_AUDIO_FMT  0x35  // i2s_format(b2:0) | audio_proc_enable(b3) | release (b5:4) | attack(b7:6)
    #define MA12070P_REG_EH_CFG     0x2D  // error handler: eh_clear(b2) | thermal_compr_en(b5)
    #define MA12070P_REG_VOL        0x40  // master volume: 0x00=+24dB, 0x18=0dB, 0xA8=-144dB

    static const ma12070p_cfg_reg_t ma12070p_init_seq[] = {
      // Standard I2S format (bits 2:0 = 0b000) + audio_proc_enable (bit3 = 1)
      { MA12070P_REG_AUDIO_FMT, 0x08 },
      // Clear accumulated errors
      { MA12070P_REG_EH_CFG, 0x00 },  
      { MA12070P_REG_EH_CFG, 0x04 },  
      { MA12070P_REG_EH_CFG, 0x00 },  
    };

  } // namespace ma12070p
} // namespace esphome