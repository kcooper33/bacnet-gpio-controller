# Fixed Deployment for BACnet GPIO Controller

## Issue Resolved
The tar files were corrupted due to GitHub API size limitations. Use individual file downloads instead.

## Working Deployment Method

```bash
# Clone the complete repository (recommended)
git clone https://github.com/kcooper33/bacnet-gpio-controller.git
cd bacnet-gpio-controller

# Copy deployment files
cp -r pi_deployment/* .
cd bacnet4linux && make && cd ..

# Or download essential files individually
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/essential_Makefile
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/essential_main.c  
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/essential_gpio_objects.c
wget https://github.com/kcooper33/bacnet-gpio-controller/raw/main/essential_install.sh

# Setup directory structure
mkdir -p bacnet4linux
mv essential_Makefile bacnet4linux/Makefile
mv essential_main.c bacnet4linux/main.c  
mv essential_gpio_objects.c bacnet4linux/gpio_objects.c
chmod +x essential_install.sh && mv essential_install.sh install.sh

# Run installation
sudo ./install.sh
```

## Alternative: Direct Repository Use

Since the repository contains all source files, simply use:

```bash
git clone https://github.com/kcooper33/bacnet-gpio-controller.git
cd bacnet-gpio-controller/pi_deployment/
chmod +x install.sh && sudo ./install.sh
```

## What This Provides

✓ Complete BACnet4Linux C source code
✓ Working Makefile for ARM compilation  
✓ GPIO configuration for all 24 pins
✓ Web interface at port 5000
✓ BACnet/IP device ID 25411

No corrupted files - everything downloads properly from the repository.