# Complete BACnet GPIO Controller Deployment

## Download and Extract Method

Due to GitHub API upload limitations, use these complete tar bundles:

```bash
# Clone repository
git clone https://github.com/kcooper33/bacnet-gpio-controller.git
cd bacnet-gpio-controller

# Extract complete BACnet4Linux files
tar -xzf bacnet4linux_complete.tar.gz -C pi_deployment/bacnet4linux/

# Extract Python deployment files  
tar -xzf pi_python_files.tar.gz

# Verify extraction
ls pi_deployment/bacnet4linux/*.c | wc -l  # Should show ~40 files
ls pi_deployment/bacnet4linux/*.h | wc -l  # Should show ~29 files

# Run installation
cd pi_deployment
sudo ./install.sh
```

## Files Included

**bacnet4linux_complete.tar.gz:**
- All 74 BACnet4Linux C source and header files
- Makefile with proper formatting
- Complete build environment

**pi_python_files.tar.gz:**
- Python Flask web interface
- GPIO configuration files
- Installation scripts
- Configuration templates

This ensures ALL files are present for successful compilation.