# BACnet Pi Controller

A professional BACnet/IP controller for Raspberry Pi using the industry-standard BACnet4Linux C stack.

## Quick Start

1. **Copy files to your Pi:**
   ```bash
   scp bacnet_pi_controller.tar.gz pi@192.168.52.12:~/
   ssh pi@192.168.52.12
   tar -xzf bacnet_pi_controller.tar.gz
   cd pi_deployment
   ```

2. **Install dependencies:**
   ```bash
   sudo apt update
   sudo apt install python3-flask python3-rpi.gpio build-essential
   ```

3. **Run the controller:**
   ```bash
   python3 pi_bacnet4linux_fixed.py
   ```

## Device Configuration

- **Device ID:** 25411
- **IP Address:** 192.168.52.12
- **Port:** 47808 (BACnet/IP standard)
- **Protocol:** BACnet/IP over UDP
- **Implementation:** BACnet4Linux C stack

## GPIO Pin Configuration

| Pin | Type | Name | Description |
|-----|------|------|-------------|
| 18  | Binary Output | Test LED | Status indicator |
| 19  | Binary Input | Motion Sensor | PIR sensor input |
| 20  | Analog Input | Temperature Sensor | Temperature reading |
| 21  | Analog Output | Fan Control | PWM fan speed |
| 26  | Binary Output | Main Relay | Control relay |

## Web Interface

Access the controller web interface at: http://192.168.52.12:5000

- Real-time GPIO status
- BACnet service status
- Device discovery information

## BACnet Discovery

Your BACnet discovery tools will find this device automatically:

- **Network:** 192.168.52.x/24
- **Device ID:** 25411
- **Vendor ID:** 999
- **Device Name:** "RPi GPIO Controller"

## Technical Details

- Uses BACnet4Linux C stack (proven industry implementation)
- Compiled for ARM architecture on Pi
- BACnet/IP over UDP (Ethernet 802.2 disabled)
- Automatic I-Am broadcasts for device discovery
- Responds to Who-Is queries and ReadProperty requests

## Troubleshooting

If the controller fails to start:
1. Ensure `bacnet4linux` directory is present
2. Check that build tools are installed: `sudo apt install build-essential`
3. Verify network configuration: `ip addr show`
4. Check port availability: `sudo netstat -tulpn | grep 47808`

## File Structure

```
pi_deployment/
├── pi_bacnet4linux_fixed.py    # Main controller
├── bacnet4linux/               # C stack source code
├── config.py                   # Configuration settings
├── gpio_config.json           # GPIO pin configuration
├── gpio_manager.py            # GPIO management
├── templates/                 # Web interface templates
└── static/                    # Web interface assets
```