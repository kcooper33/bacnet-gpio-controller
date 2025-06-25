# GitHub Deployment Instructions

## Quick GitHub Setup

1. **Create a new repository on GitHub:**
   - Repository name: `bacnet-gpio-controller`
   - Description: "Professional BACnet/IP GPIO controller for Raspberry Pi with web configuration"
   - Initialize with README: No (we have our own)

2. **Clone and push from your local machine:**

```bash
# Download the project bundle
curl -o bacnet-pi-controller.bundle http://192.168.52.12:5000/static/bacnet-pi-controller.bundle

# Create local repository
git clone bacnet-pi-controller.bundle bacnet-gpio-controller
cd bacnet-gpio-controller

# Add GitHub remote (replace USERNAME with your GitHub username)
git remote add origin https://github.com/USERNAME/bacnet-gpio-controller.git

# Push to GitHub
git push -u origin main
```

## Project Structure

The repository contains:

### Core Files
- `gpio_web_server.py` - Standalone web interface for GPIO configuration
- `pi_bacnet4linux_fixed.py` - Complete BACnet controller with integrated web interface
- `gpio_manager.py` - Hardware abstraction layer for GPIO operations
- `config.py` - Application configuration settings

### BACnet4Linux C Stack
- `bacnet4linux/` - Industry-standard BACnet protocol implementation
- `install.sh` - Automated deployment script for Raspberry Pi
- `pi_deployment/` - Complete deployment package

### Web Interface
- `templates/gpio_config.html` - Bootstrap-based GPIO configuration interface
- `templates/index.html` - Main dashboard interface
- `static/` - CSS, JavaScript, and image assets

### Configuration
- `gpio_pin_config.json` - GPIO pin configuration (all 24 pins)
- `gpio_bacnet_config.json` - Generated BACnet object configuration
- `bacnet_config.json` - BACnet device settings

### Documentation
- `README.md` - Comprehensive project documentation
- `DEPLOY_TO_PI.md` - Raspberry Pi deployment instructions
- `START_CONTROLLER.md` - Controller startup guide
- `replit.md` - Development history and architecture notes

## Key Features for GitHub

✅ **Complete GPIO Management**: All 24 GPIO pins (GPIO 0-23) configurable via web interface

✅ **Dynamic BACnet Objects**: Automatic Binary Input/Output creation based on configuration

✅ **Professional Protocol Stack**: Industry-standard BACnet4Linux C implementation

✅ **Raspberry Pi 5 Compatible**: Multi-method GPIO control with hardware compatibility

✅ **Web-Based Configuration**: Bootstrap responsive interface at `/gpio_config`

✅ **Production Ready**: Automated deployment with `install.sh` script

## Release Tags Suggested

- `v1.0.0` - Initial release with complete GPIO configuration system
- `v1.1.0` - Enhanced web interface and Bootstrap UI
- `v1.2.0` - Raspberry Pi 5 compatibility and multi-method GPIO

## GitHub Repository Settings

**Topics to add:**
- `bacnet`
- `raspberry-pi`
- `gpio`
- `industrial-automation`
- `iot`
- `building-automation`
- `python`
- `flask`
- `bootstrap`

**License:** MIT (for industrial automation and educational use)

## Deployment URLs

After GitHub deployment, users can:

1. **Clone the repository:**
   ```bash
   git clone https://github.com/USERNAME/bacnet-gpio-controller.git
   ```

2. **Deploy to Raspberry Pi:**
   ```bash
   cd bacnet-gpio-controller
   tar -czf deployment.tar.gz pi_deployment/
   scp deployment.tar.gz pi@192.168.52.12:~/
   ssh pi@192.168.52.12
   tar -xzf deployment.tar.gz
   cd pi_deployment/
   chmod +x install.sh
   sudo ./install.sh
   ```

3. **Access web interface:**
   - Main controller: `http://192.168.52.12:5000`
   - GPIO configuration: `http://192.168.52.12:5000/gpio_config`
   - BACnet device: `192.168.52.12:47808` (Device ID 25411)

## GitHub Actions (Optional)

Consider adding automated testing and deployment workflows:

- **Testing**: Validate JSON configuration files and Python syntax
- **Raspberry Pi Images**: Auto-generate deployment packages
- **Documentation**: Auto-update API documentation from code