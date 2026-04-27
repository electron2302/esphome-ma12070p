# ESPHome Component for MA12070P DAC

The MA12070P is a high-efficiency Class D audio amplifier with basic DSP capabilities, designed by Merus Audio (Infineon). This ESPHome external component provides integration of the MA12070P with ESP32-based systems, supporting digital volume control, mute, and power management via Home Assistant or ESPHome automations.

# Why

I've created a [Loud-ESP32-Plus](https://sonocotta.com/loud-esp32/) board, that uses this DAC, and I wanted to integrate this DAC into the Home Assistant. Even though it is discontinued by Infineon, they didn't really provide a comparable alternative. So while they are still in stock, why would I not use it 

## Usage: MA12070P Component from GitHub

This component requires ESPHome version 2026.4.0 or later and **ESP-IDF framework only** (Arduino framework is not supported).

```yaml
external_components:
  - source: github://sonocotta/esphome-ma12070p@main
```

## Overview

The MA12070P communicates with the ESP32 via:
- **I2C** — register control (volume, mute, init sequence)
- **I2S** — digital audio stream (**must be configured for 32-bit slot width**)

### Key Specifications

- **Interface**: I2C control + I2S audio
- **Default I2C Address**: `0x20` (7-bit)
- **Digital Volume Register**: `0x00` = +24 dB, `0x18` = 0 dB, `0xA8` = −144 dB; step = −1 dB/LSB
- **ENABLE pin**: HIGH = chip in reset (minimum power), LOW = active
- **MUTE pin**: LOW = muted, HIGH = playing

### I2S Requirement

The MA12070P requires **32-bit I2S slot width** (64 BCLK cycles per frame). Configure the ESPHome I2S speaker with:

```yaml
speaker:
  - platform: i2s_audio
    bits_per_sample: 32bit
    ...
```

## Component Features

- **Initialization** — writes standard I2S format + audio processor enable on startup; re-applies init sequence after waking from power-down (chip loses all registers during reset)
- **Volume control** — 0–100% ESPHome range remapped to a configurable dB window; volume register is restored after power-down wake
- **Mute control** — GPIO mute pin (MUTE pin: LOW = muted, HIGH = playing) tracked as ESPHome `is_muted` state
- **Power management** — `enable_dac` switch puts the chip into reset (ENABLE HIGH) or wakes it (ENABLE LOW); on wake the init sequence and volume are automatically restored, and the mute pin is restored to its pre-sleep state
- **Debug mode** — optional register-level I2C logging (all reads and writes at `DEBUG` level)

TODO: not yet implemented:

- TODO: 20/26 dB gain switch
- TODO: Power profiles: 0..4, refer to datasheet, page 13
- TODO: Error sensors read and report
- TODO: Power limiter settings: attack, release, level 
- TODO: Soft clipping settings
- TODO: MSEL configuration monitor: SE, BTL, PBTL

## Hardware Requirements

### I2C Interface

- **SDA / SCL**: configurable GPIO pins
- Recommended bus frequency: 400 kHz
- 4.7 kΩ pull-up resistors on SDA/SCL (pins have no internal pull-up)

### Optional GPIO Pins

| Pin | Direction | Active level | Function |
|-----|-----------|--------------|----------|
| ENABLE | Output | HIGH = reset | Chip power-down / reset; LOW = active |
| MUTE | Output | LOW = muted | Audio output mute |

Both pins are optional. If `enable_pin` is not wired, the chip stays powered at all times. If `mute_pin` is not wired, mute state is software-only (no hardware muting).

## YAML Configuration

### Minimal Example

```yaml
i2c:
  sda: GPIO8
  scl: GPIO9
  frequency: 400kHz

audio_dac:
  - platform: ma12070p
    id: ma12070p_dac
```

### Full Example (ESP32-S3)

```yaml
i2c:
  sda: GPIO8
  scl: GPIO9
  frequency: 400kHz

audio_dac:
  - platform: ma12070p
    id: ma12070p_dac
    address: 0x20                # default, 7-bit
    enable_pin: GPIO17           # ENABLE: HIGH = reset, LOW = active
    mute_pin: GPIO18             # MUTE: LOW = muted, HIGH = playing
    volume_max: 0dB              # top of volume slider = 0 dB
    volume_min: -60dB            # bottom of volume slider = -60 dB
    debug: false                 # set true to log all I2C register traffic

i2s_audio:
  i2s_lrclk_pin: GPIO15
  i2s_bclk_pin: GPIO14

speaker:
  - platform: i2s_audio
    id: speaker_id
    dac_type: external
    i2s_dout_pin: GPIO16
    audio_dac: ma12070p_dac
    channel: stereo
    sample_rate: 48000
    bits_per_sample: 32bit       # required by MA12070P
    timeout: never
```

### Configuration Variables

#### `audio_dac` platform `ma12070p`

| Key | Required | Default | Description |
|-----|----------|---------|-------------|
| `address` | No | `0x20` | I2C 7-bit address |
| `enable_pin` | No | — | GPIO wired to chip ENABLE pin (HIGH = reset) |
| `mute_pin` | No | — | GPIO wired to chip MUTE pin (LOW = muted) |
| `volume_max` | No | `24dB` | Upper bound of the volume slider in dB (range −144…+24) |
| `volume_min` | No | `-103dB` | Lower bound of the volume slider in dB (range −144…+24) |
| `debug` | No | `false` | Log every I2C register read/write at DEBUG level |

## Switches

Two optional switches can be exposed to Home Assistant:

```yaml
switch:
  - platform: ma12070p
    ma12070p_id: ma12070p_dac
    enable_dac:
      name: "DAC Power"         # toggles enable_pin (wakes / resets the chip)
    mute:
      name: "DAC Mute"          # toggles mute_pin
```

## Volume Control

Volume is set as a float 0.0–1.0 by the ESPHome media player and remapped linearly to the register range [`volume_min` … `volume_max`]. The formula is:

```
raw = 24 - dB         (e.g. 0 dB → 0x18, −60 dB → 0x54)
```

The cached volume value is automatically restored after a power-down/wake cycle.

## Mute Control

Mute is driven exclusively by the GPIO MUTE pin (if configured):
- `set_mute_on()` → MUTE pin LOW
- `set_mute_off()` → MUTE pin HIGH

On wake from power-down (`set_deep_sleep_off`), the mute pin is restored to its pre-sleep state.

## Power Management

| State | ENABLE pin | Chip state | Init on wake |
|-------|-----------|-----------|--------------|
| Active | LOW | Running | — |
| Power-down | HIGH | Reset (all regs lost) | Full init + volume restore |

Wake sequence: ENABLE LOW → 100 ms stabilization → init sequence → volume register → restore mute pin.

## Complete Example YAML

Full working configurations for [ESP32](/components/ma12070p/yaml/esp32-idf-media-player.yaml) and [ESP32-S3](/components/ma12070p/yaml/esp32s3-idf-media-player.yaml).

## Troubleshooting

### DAC Not Initializing

1. Check I2C wiring and pull-up resistors (4.7 kΩ to 3.3 V)
2. Verify I2C address — default is `0x20`; check ADDR pin strapping on your board
3. Enable `i2c: scan: true` in YAML to confirm the device is visible on the bus
4. Enable `debug: true` on the component and set `logger: level: DEBUG` to see all register traffic

### No Audio Output

1. Confirm `bits_per_sample: 32bit` is set on the I2S speaker — this is mandatory
2. Check that the DAC is not muted (MUTE pin HIGH = unmuted)
3. Check that `enable_pin` is LOW (chip active)
4. Confirm volume is not at minimum

### I2C Errors

- Ensure SDA/SCL have 4.7 kΩ pull-ups to 3.3 V
- Keep I2C wires short; use 100 kHz if experiencing errors at 400 kHz

## Component Files

| File | Purpose |
|------|---------|
| `ma12070p.h` | Class declaration |
| `ma12070p.cpp` | Implementation |
| `ma12070p_cfg.h` | Register addresses and init sequence |
| `audio_dac.py` | ESPHome YAML integration |
| `switch/__init__.py` | Enable DAC and Mute switches |

## License

This component is licensed under GPLv3.

## References

- [Infineon MA12070P Product Page](https://www.infineon.com/part/MA12070P)
- [ESPHome Audio DAC Component](https://esphome.io/components/audio_dac/)
- [ESPHome I2S Audio Speaker](https://esphome.io/components/speaker/i2s_audio/)
