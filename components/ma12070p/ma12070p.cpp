#include "ma12070p.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome
{
  namespace ma12070p
  {

    static const char *const TAG = "ma12070p";
    static const char *const ERROR = "Error ";

    // maximum delay allowed in "ma12070p_minimal.h" used in configure_registers()
    static const uint8_t ESPHOME_MAXIMUM_DELAY = 5; // milliseconds

    void Ma12070Component::setup()
    {
      ESP_LOGCONFIG(TAG, "Running setup");
      if (this->enable_pin_ != nullptr)
      {
        this->enable_pin_->setup();
        this->enable_pin_->digital_write(false); // LOW = active (device on for I2C config)
      }

      if (this->mute_pin_ != nullptr)
      {
        this->mute_pin_->setup();
        this->mute_pin_->digital_write(false); // LOW = muted until setup complete
      }

      // rescale configured dB range to register values (0x00=+24dB, 0x18=0dB, 0xA8=-144dB, step=-1dB/LSB)
      // raw = 24 - db  (e.g. 0dB -> 0x18=24, -60dB -> 0x54=84)
      this->ma12070p_state_.raw_volume_max = (uint8_t)(24 - this->ma12070p_state_.volume_max);
      this->ma12070p_state_.raw_volume_min = (uint8_t)(24 - this->ma12070p_state_.volume_min);
      this->ma12070p_state_.raw_volume_current = this->ma12070p_state_.raw_volume_max; // start at max volume

      if (!configure_registers_())
      {
        this->error_code_ = CONFIGURATION_FAILED;
        this->mark_failed();
      }
    }

    bool Ma12070Component::configure_registers_()
    {
      if (!this->set_deep_sleep_off_())
        return false;

      this->start_time_ = millis();
      return true;
    }

    bool Ma12070Component::write_init_seq_()
    {
      uint16_t i = 0;
      uint16_t counter = 0;
      uint16_t number_configurations = sizeof(ma12070p_init_seq) / sizeof(ma12070p_init_seq[0]);

      while (i < number_configurations)
      {
        switch (ma12070p_init_seq[i].offset)
        {
        case MA12070P_CFG_META_DELAY:
          if (ma12070p_init_seq[i].value > ESPHOME_MAXIMUM_DELAY)
            return false;
          delay(ma12070p_init_seq[i].value);
          break;
        default:
          if (!this->ma12070p_write_byte_(ma12070p_init_seq[i].offset, ma12070p_init_seq[i].value))
            return false;
          counter++;
          break;
        }
        i++;
      }
      this->number_registers_configured_ = counter;
      return true;
    }

    bool Ma12070Component::set_deep_sleep_off_()
    {
      ESP_LOGD(TAG, "Exiting power down mode");
      // save mute state before touching the pin
      bool was_muted = this->is_muted_;

      // Mute before restarting the DAC
      if (this->mute_pin_ != nullptr)
        this->mute_pin_->digital_write(false); // LOW = muted during power up, will be restored to pre-sleep state at the end of this function

      if (this->enable_pin_ != nullptr)
      {
        this->enable_pin_->digital_write(false); // LOW = active
        delay(100);                              // wait for PVDD to stabilize
      }

      // DAC lost all settings while in reset — re-send init sequence
      if (!this->write_init_seq_())
        return false;

      // restore volume (register was cleared by reset)
      if (!this->set_digital_volume_(this->ma12070p_state_.raw_volume_current))
        return false;
      
      // restore mute pin to its pre-sleep state
      if (this->mute_pin_ != nullptr)
        this->mute_pin_->digital_write(!was_muted); // HIGH = unmuted, LOW = muted
      this->is_muted_ = was_muted;
      
      this->ma12070p_state_.control_state = STATE_RUNNING;
      return true;
    }

    bool Ma12070Component::set_deep_sleep_on_()
    {
      ESP_LOGD(TAG, "Entering power down mode");
      if (this->enable_pin_ != nullptr)
        this->enable_pin_->digital_write(true); // HIGH = reset/inactive
      this->ma12070p_state_.control_state = STATE_IDLE;
      return true;
    }

    bool Ma12070Component::set_state_(ControlState state)
    {
      this->ma12070p_state_.control_state = state;
      return true;
    }

    void Ma12070Component::loop()
    {
    }

    void Ma12070Component::update()
    {
    }

    void Ma12070Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "MA12070P Audio Dac:");

      switch (this->error_code_)
      {
      case CONFIGURATION_FAILED:
        ESP_LOGE(TAG, "  %s setting up MA12070P: %i", ERROR, this->i2c_error_);
        break;
      case NONE:
        LOG_PIN("  Enable Pin: ", this->enable_pin_);
        LOG_PIN("  Mute Pin: ", this->mute_pin_);
        LOG_I2C_DEVICE(this);
        ESP_LOGCONFIG(TAG,
                      "  Registers configured: %i\n"
                      "  Maximum Volume: %idB\n"
                      "  Minimum Volume: %idB\n"
                      "  Debug mode: %s",
                      this->number_registers_configured_,
                      this->ma12070p_state_.volume_max,
                      this->ma12070p_state_.volume_min,
                      this->debug_mode_ ? "enabled" : "disabled"
        );
        LOG_UPDATE_INTERVAL(this);
        break;
      }
    }

    // public
    void Ma12070Component::enable_dac(bool enable)
    {
      if (enable)
        set_deep_sleep_off_();
      else
        set_deep_sleep_on_();
    }

    bool Ma12070Component::set_mute_off()
    {
      ESP_LOGD(TAG, "Mute OFF");
      if (this->mute_pin_ != nullptr)
        this->mute_pin_->digital_write(true); // HIGH = unmuted (playing)
      this->is_muted_ = false;
      return true;
    }

    bool Ma12070Component::set_mute_on()
    {
      ESP_LOGD(TAG, "Mute ON");
      if (this->mute_pin_ != nullptr)
        this->mute_pin_->digital_write(false); // LOW = muted
      this->is_muted_ = true;
      return true;
    }

    float Ma12070Component::volume()
    {
      uint8_t raw_volume;
      get_digital_volume_(&raw_volume);

      return remap<float, uint8_t>(raw_volume, this->ma12070p_state_.raw_volume_min,
                                   this->ma12070p_state_.raw_volume_max,
                                   0.0f, 1.0f);
    }

    bool Ma12070Component::set_volume(float volume)
    {
      float new_volume = clamp(volume, 0.0f, 1.0f);
      uint8_t raw_volume = remap<uint8_t, float>(new_volume, 0.0f, 1.0f,
                                                 this->ma12070p_state_.raw_volume_min,
                                                 this->ma12070p_state_.raw_volume_max);
      if (!this->set_digital_volume_(raw_volume))
        return false;

      int8_t dB = 24 - (int8_t)raw_volume;
      ESP_LOGD(TAG, "Volume: %idB", dB);
      return true;
    }

    bool Ma12070Component::get_digital_volume_(uint8_t *raw_volume)
    {
      uint8_t current = 0xA8; // lowest raw volume (-144dB)
      if (!this->ma12070p_read_byte_(MA12070P_REG_VOL, &current))
        return false;
      *raw_volume = current;
      return true;
    }

    // master volume register: 0x00=+24dB, 0x18=0dB, 0xA8=-144dB, step=-1dB/LSB
    bool Ma12070Component::set_digital_volume_(uint8_t raw_volume)
    {
      if (!this->ma12070p_write_byte_(MA12070P_REG_VOL, raw_volume))
        return false;
      this->ma12070p_state_.raw_volume_current = raw_volume;
      return true;
    }

    bool Ma12070Component::get_state_(ControlState *state)
    {
      *state = this->ma12070p_state_.control_state;
      return true;
    }

    bool Ma12070Component::ma12070p_read_byte_(uint8_t a_register, uint8_t *data)
    {
      return ma12070p_read_bytes_(a_register, data, 1);
    }

    bool Ma12070Component::ma12070p_read_bytes_(uint8_t a_register, uint8_t *data, uint8_t number_bytes)
    {
      i2c::ErrorCode error_code;
      error_code = this->write(&a_register, 1);
      if (error_code != i2c::ERROR_OK)
      {
        ESP_LOGE(TAG, "Write error:: %i", error_code);
        this->i2c_error_ = (uint8_t)error_code;
        return false;
      }
      error_code = this->read_register(a_register, data, number_bytes);
      if (error_code != i2c::ERROR_OK)
      {
        ESP_LOGE(TAG, "Read error: %i", error_code);
        this->i2c_error_ = (uint8_t)error_code;
        return false;
      }
      if (this->debug_mode_)
      {
        for (uint8_t i = 0; i < number_bytes; i++)
          ESP_LOGD(TAG, "I2C RD reg=0x%02X val=0x%02X", a_register + i, data[i]);
      }
      return true;
    }

    bool Ma12070Component::ma12070p_write_byte_(uint8_t a_register, uint8_t data)
    {
      return this->ma12070p_write_bytes_(a_register, &data, 1);
    }

    bool Ma12070Component::ma12070p_write_bytes_(uint8_t a_register, uint8_t *data, uint8_t len)
    {
      if (this->debug_mode_)
      {
        for (uint8_t i = 0; i < len; i++)
          ESP_LOGD(TAG, "I2C WR reg=0x%02X val=0x%02X", a_register + i, data[i]);
      }
      i2c::ErrorCode error_code = this->write_register(a_register, data, len);
      if (error_code != i2c::ERROR_OK)
      {
        ESP_LOGE(TAG, "Write error: %i", error_code);
        this->i2c_error_ = (uint8_t)error_code;
        return false;
      }
      return true;
    }

  } // namespace ma12070p
} // namespace esphome
