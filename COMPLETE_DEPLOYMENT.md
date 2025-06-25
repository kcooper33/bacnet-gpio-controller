# Complete BACnet Pi Controller Deployment

## Quick Deployment (Recommended)

### Option 1: Download Individual Packages
```bash
# Download BACnet4Linux C source
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/bacnet4linux_complete.tar.gz

# Download Python application files  
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/pi_python_files.tar.gz

# Extract and setup
tar -xzf bacnet4linux_complete.tar.gz -C bacnet4linux/
tar -xzf pi_python_files.tar.gz
chmod +x install.sh && sudo ./install.sh
```

### Option 2: Clone Repository
```bash
git clone https://github.com/kcooper33/bacnet-gpio-controller.git
cd bacnet-gpio-controller/pi_deployment/
chmod +x install.sh && sudo ./install.sh
```

## What Each Package Contains

**bacnet4linux_complete.tar.gz**:
- Complete C source code for BACnet4Linux protocol stack
- Working Makefile with proper ARM compilation rules
- GPIO integration and custom object handling
- All header files and dependencies

**pi_python_files.tar.gz**:
- Python Flask applications and GPIO managers
- Configuration files for all 24 GPIO pins
- Web interface templates
- Installation automation scripts

## Fixes the Issues

This resolves:
- "No rule to make target 'all'" Makefile error
- Missing BACnet4Linux source files
- Incomplete deployment structure
- ARM compilation problems

## Verified Components

✓ Complete BACnet4Linux build environment
✓ Working Makefile for ARM (aarch64) compilation  
✓ All Python dependencies and scripts
✓ GPIO configuration for pins 0-23
✓ Web interface at port 5000
✓ BACnet/IP device ID 25411 on port 47808

After installation, access GPIO configuration at:
http://[raspberry-pi-ip]:5000/gpio_config