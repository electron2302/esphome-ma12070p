# ESPHome Component for MA12070P DAC

The MA12070P is a high-performance audio DAC (Digital-to-Analog Converter) designed for portable and automotive audio applications. This ESPHome external component provides easy integration of the MA12070P DAC with ESP32-based systems for professional audio output.

## Usage: MA12070P Component from GitHub

This component requires ESPHome version 2025.7.0 or later.

The following YAML can be used so ESPHome accesses the component files:

```yaml
external_components:
  - source: github://sonocotta/esphome-ma12070p@main
```

## Overview

The MA12070P is a versatile audio DAC that communicates with the ESP32 via I2C interface (control) and I2S (audio). This component allows basic control of the DAC's features including digital volume adjustment, mute control, analog gain, basic mixer controls and power management through Home Assistant or direct ESPHome automation. A

### Key Specifications

- **Interface**: I2C (Two-wire interface) and I2S (Digital Audio)
- **Digital Volume Range**: -103 dB to +24 dB in 0.5 dB steps
- **Volume Control**: 0-100% via ESPHome (internally remapped to dB range)
- **Analog Gain**: Configurable (-6 dB or 0 dB) for 1V or 2V RMS output levels
- **Mute Control**: Register-based (always available) and optional GPIO-based mute
- **Default I2C Address**: 0x4D (7-bit)
- **Power Modes**: Play mode (active audio) and Sleep mode (low power)

## Component Features

The MA12070P ESPHome component provides the following capabilities:

- **Initialize MA12070P DAC** - Configure and initialize the audio codec on startup
- **Enable/Disable DAC** - Place the DAC into Play mode or low-power Sleep mode via optional switch
- **Volume Control** - Set and get volume level from 0-100% (remapped to -103 dB to +24 dB register range)
- **Analog Gain Control** - Set desired output voltage level
- **Mute Control** - Mute and unmute the DAC via I2C register and optional GPIO pin
- **Power Management** - Enable Deep Sleep mode for low-power operation
- **Volume Range Configuration** - Define maximum and minimum volume limits in dB
- **Basic Mixer control** - Stereo (default), Stereo (inverted channels), Left or Right channel only 

## Hardware Requirements

### I2C Interface

The MA12070P requires I2C communication:
- **SDA Pin**: Data line (configurable)
- **SCL Pin**: Clock line (configurable)

### Optional GPIO Enable Pin

For hardware-based mute control via GPIO:
- **Mute Pin**: GPIO pin for direct mute control (optional, when XSMT pin is wired to it)
  - `HIGH` = Unmute (audio enabled)
  - `LOW` = Mute (audio disabled)
  - Can also be controlled programmatically via DAC Enable Switch

## YAML Configuration

### Basic Audio DAC Configuration

```yaml
audio_dac:
  - platform: ma12070p
```

### Complete Configuration Example

```yaml
i2c:
  sda: GPIO21
  scl: GPIO27
  id: i2c_bus

audio_dac:
  - platform: ma12070p
    i2c_id: i2c_bus
    enable_pin: GPIO13
    volume_max: 0
    volume_min: -103
```

### Configuration Variables

#### audio_dac

- **platform** (*Required*): `ma12070p`
- **i2c_id** (*Optional*): ID of the I2C bus. Leave unset if only one I2C bus is defined.
- **address** (*Optional*): I2C address of the DAC. Defaults to `0x4D` (7-bit address).
- **enable_pin** (*Optional*): GPIO pin used to control DAC mute pin (if connected to XSMT pin).
- **analog_gain** (*Optional*): Analog gain in dB. Valid values: `0dB` (default) or `-6dB`.
  - `0dB`: Use for 2V RMS output signal
  - `-6dB`: Use for 1V RMS output signal
- **volume_max** (*Optional*): Maximum volume level in dB. Valid range: -103 to 24. Default: `0` dB, to guarantee no clipping
- **volume_min** (*Optional*): Minimum volume level in dB. Valid range: -103 to 24. Default: `-103` dB, but maybe raised to limit dynamic range. Reasonable value is -60dB.

## Switches

An optional switch can be configured to control the DAC enable/disable state in Home Assistant.

## Volume Control

Volume is controlled as a percentage (0-100%) which is internally remapped to the DAC's digital volume register range (-103 dB to 0 dB, unless remaped in yaml to custom values).

### Setting Volume in YAML

```yaml
media_player:
  - platform: i2s_audio
    # ... other configuration
    volume: 0.8  # 80% volume
```

### Volume Control via Automations

```yaml
automation:
  - trigger:
      platform: state
      entity_id: input_number.audio_volume
    action:
      service: media_player.volume_set
      data:
        entity_id: media_player.my_speaker
        volume_level: "{{ trigger.to_state.state | float / 100 }}"
```

## Mute Control

The MA12070P supports two types of mute control:

### I2C Register-based Mute

Mute is controlled via the MA12070P register (Register 0x03):
- Setting bits enable/disable mute on both channels
- Works regardless of GPIO configuration

### Optional GPIO Mute Pin

If a GPIO mute pin connected to the XSMT pin:
- Provides hardware-level mute control
- Works in parallel with register-based mute
- `HIGH` = Unmute (enabled)
- `LOW` = Mute (disabled)

### Mute Control in YAML

```yaml
automation:
  - trigger:
      platform: homeassistant
      event: call_service
      service: media_player.mute_volume
    action:
      - service: media_player.mute_volume
        data:
          entity_id: media_player.my_speaker
          is_volume_muted: true
```

## Integration with I2S Audio

The MA12070P DAC is typically used with the ESPHome I2S Audio component:

```yaml
i2s_audio:
  - i2s_lrclk_pin: GPIO25
    i2s_bclk_pin: GPIO26
    i2s_dout_pin: GPIO27

media_player:
  - platform: i2s_audio
    name: "My Audio Player"
    dac: ma12070p_dac
    speaker: my_speaker

speaker:
  - platform: i2s_audio
    name: "Speaker"
    dac_type: external
    i2s_audio_id: i2s_audio
```

## Power Modes

### Play Mode (Enabled)

When the DAC is enabled:
- Normal audio operation
- Audio can be produced at configured volume
- Current consumption: ~100-150 mA typical

### Sleep Mode (Deep Sleep)

When the DAC is disabled:
- Reduced power consumption
- Audio output disabled
- Current consumption: < 50 µA typical

## Complete Example YAML

Can be found in the example for [ESP32](/components/ma12070p/yaml/esp32-idf-media-player.yaml) and [ESP32-S3](/components/ma12070p/yaml/esp32s3-idf-media-player.yaml)

## Troubleshooting

### DAC Not Initializing

1. **Check I2C Connection**
   ```bash
   esphome logs components/ma12070p/yaml/idf-media-player.yaml
   ```
   Look for I2C initialization messages and any I2C errors.

2. **Verify I2C Address**
   - Default address is `0x4D` (7-bit)
   - Check hardware configuration if using a different address

3. **Check GPIO Pins**
   - Enable pin is now optional - DAC will work without it
   - Verify pull-up resistors on SDA/SCL lines (typically 4.7k ohms)
   - Ensure I2C pins are not used for other purposes

### No Audio Output

1. **Check Mute Status**
   - Ensure DAC is not muted via register or GPIO
   - Verify speaker is properly connected

2. **Verify I2S Configuration**
   - Ensure I2S audio pins are correctly configured
   - Check I2S clock and data line connections

3. **Volume Level**
   - Confirm volume is not set to 0%
   - Check that volume_min/volume_max are within valid range

### I2C Communication Errors

1. **Enable Debug Logging**
   ```yaml
   logger:
     level: DEBUG
   ```

2. **Check Voltage Levels**
   - I2C bus should be pulled to 3.3V (for ESP32)
   - Add 4.7k ohm pull-up resistors if missing (pins have no PULLUP support)

## Component Files

The MA12070P component consists of:

- `ma12070p.h` - Component header file with class definitions
- `ma12070p.cpp` - Component implementation
- `audio_dac.py` - ESPHome integration for YAML configuration
- `binary_sensor.py` - Optional binary sensors for fault detection
- `switch/__init__.py` - Optional switches for DAC control
- `ma12070p_cfg.h` - Configuration and register definitions

## License

This component is licensed under GPLv3

## References

- [MA12070P Datasheet](https://www.ti.com/product/MA12070P)
- [ESPHome Audio DAC Component](https://esphome.io/components/audio_dac/index.html)
- [ESPHome I2S Audio](https://esphome.io/components/i2s_audio/index.html)
