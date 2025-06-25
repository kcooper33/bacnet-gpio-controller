# BACnet Pi Controller - Automated Installation

## Quick Start

1. **Copy folder to your Pi**
2. **Run installation script:**
   ```bash
   chmod +x install.sh
   ./install.sh
   ```

The script automatically:
- Checks system requirements
- Compiles BACnet4Linux for ARM
- Reads configuration from `bacnet_config.json`
- Starts the BACnet controller with GPIO objects

## Configuration File

Edit `bacnet_config.json` to customize:

### Device Settings
- Device ID and name
- Vendor ID
- Network interface and IP

### GPIO Pins
```json
"gpio_pins": {
  "18": {
    "name": "Test LED",
    "type": "binary_output",
    "enabled": true,
    "initial_value": 0
  }
}
```

### BACnet Options
- Port, debug level
- Ethernet settings
- APDU timeout

## GPIO Object Types

- **binary_input**: Digital sensor (motion, switch)
- **binary_output**: Digital control (LED, relay)  
- **analog_input**: Sensor reading (temperature)
- **analog_output**: Control signal (PWM, speed)

## BACnet Objects Created

The controller creates these discoverable objects:
- Device 25411 (main device)
- Binary Output 4018 (GPIO 18 - Test LED)
- Binary Input 3019 (GPIO 19 - Motion Sensor)
- Analog Input 1020 (GPIO 20 - Temperature)
- Analog Output 2021 (GPIO 21 - Fan Control)
- Binary Output 4026 (GPIO 26 - Main Relay)

## Manual Commands

**Compile only:**
```bash
cd bacnet4linux
make clean && make all
```

**Test BACnet manually:**
```bash
cd bacnet4linux
./bacnet4linux -d25411 -v999 -p47808 -iwlan0 -e0 -t10 -D2 -q1 -h0
```

**Run controller manually:**
```bash
python3 pi_bacnet_configured.py
```

Your discovery tools will find Device 25411 and all configured GPIO objects as individual BACnet points.