#!/bin/bash

# Find the first USB serial device matching the pattern
DEVICE=$(ls /dev/tty.usbserial* 2>/dev/null | head -1)

if [ -z "$DEVICE" ]; then
    echo "Error: No USB serial device matching /dev/tty.usbserial* found"
    exit 1
fi

echo "Using device: $DEVICE"

# Get the esphome executable from the virtual environment
ESPHOME="${BASH_SOURCE%/*}/../.venv/bin/esphome"

# Run esphome with the found device
"$ESPHOME" run components/ma12070p/yaml/esp32-idf-media-player.yaml --device "$DEVICE"
