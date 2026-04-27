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
        this->enable_pin_->digital_write(true); // default to power off until setup complete
      }

      if (this->mute_pin_ != nullptr)
      {
        this->mute_pin_->setup();
        this->mute_pin_->digital_write(false); // default to muted until setup complete
      }

      if (!configure_registers_())
      {
        this->error_code_ = CONFIGURATION_FAILED;
        this->mark_failed();
      }

      // TODO: readjust scaler for MA12060P which has a different volume range
      // rescale -103db to 24db digital volume range to register digital volume range 254 to 0
      this->ma12070p_state_.raw_volume_max = (uint8_t)((this->ma12070p_state_.volume_max - 24) * -2);
      this->ma12070p_state_.raw_volume_min = (uint8_t)((this->ma12070p_state_.volume_min - 24) * -2);
    }

    bool Ma12070Component::configure_registers_()
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

      if (!this->set_deep_sleep_off_())
        return false;

      this->start_time_ = millis();
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
  
                      this->number_registers_configured_,
                      this->ma12070p_state_.volume_max,
                      this->ma12070p_state_.volume_min
        );
        LOG_UPDATE_INTERVAL(this);
        break;
      }
    }

    // public
    void Ma12070Component::enable_dac(bool enable)
    {
      // TODO: implement power on/off using Mute pin
    }

    bool Ma12070Component::set_mute_off()
    {
      ESP_LOGD(TAG, "Mute OFF entered");
      if (!this->is_muted_)
        return true;

      // TODO: implement mute on/off using Mute pin

      this->is_muted_ = false;
      ESP_LOGD(TAG, "Mute OFF done");
      return true;
    }

    bool Ma12070Component::set_mute_on()
    {
      ESP_LOGD(TAG, "Mute ON entered");
      if (this->is_muted_)
        return true;

      // TODO: implement mute on/off using Mute pin

      this->is_muted_ = true;
      ESP_LOGD(TAG, "Mute ON done");
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

      int8_t dB = -(raw_volume / 2) + 24;
      ESP_LOGD(TAG, "Volume: %idB", dB);
      return true;
    }

    bool Ma12070Component::get_digital_volume_(uint8_t *raw_volume)
    {
      uint8_t current = 254; // lowest raw volume
      if (!this->ma12070p_read_byte_(MA12070P_REG_VOL_L, &current))
        return false;
      *raw_volume = current;
      return true;
    }

    // controls both left and right channel digital volume
    // digital volume is 24 dB to -103 dB in -0.5 dB step
    // 00000000: +24.0 dB
    // 00000001: +23.5 dB
    // 00101111: +0.5 dB
    // 00110000: 0.0 dB
    // 00110001: -0.5 dB
    // 11111110: -103 dB
    // 11111111: Mute
    bool Ma12070Component::set_digital_volume_(uint8_t raw_volume)
    {
      // TODO: implement volume register
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
      return true;
    }

    bool Ma12070Component::ma12070p_write_byte_(uint8_t a_register, uint8_t data)
    {
      return this->ma12070p_write_bytes_(a_register, &data, 1);
    }

    bool Ma12070Component::ma12070p_write_bytes_(uint8_t a_register, uint8_t *data, uint8_t len)
    {
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
