# Deploy BACnet Controller to Raspberry Pi

## Step 1: Download Files

From this development environment, download the deployment package:
```bash
# Download the fixed deployment package
bacnet_pi_controller_network_fixed.tar.gz
```

## Step 2: Transfer to Pi

Copy the package to your Raspberry Pi at 192.168.52.12:
```bash
scp bacnet_pi_controller_network_fixed.tar.gz pi@192.168.52.12:~/
```

## Step 3: Extract on Pi

SSH to your Pi and extract the files:
```bash
ssh pi@192.168.52.12
cd ~
tar -xzf bacnet_pi_controller_network_fixed.tar.gz
cd pi_deployment
```

## Step 4: Install Dependencies

Install required packages:
```bash
sudo apt update
sudo apt install python3-flask python3-gpiozero build-essential iproute2
```

## Step 5: Test Network (Optional)

Verify network configuration:
```bash
python3 test_bacnet_network.py
```

## Step 6: Start Controller

Run the BACnet controller:
```bash
python3 pi_bacnet4linux_fixed.py
```

## Expected Success Output

```
======================================================================
BACnet GPIO Controller for Raspberry Pi
Using BACnet4Linux C Stack - BACnet/IP Mode
Device ID: 25411
Network: 192.168.52.12:47808
======================================================================
Initializing GPIO...
GPIO 18 configured as output: Test LED
[GPIO messages may show errors on newer Pi models - this is normal]
GPIO initialized successfully (using gpiozero)
Starting BACnet4Linux service...
Using network interface: wlan0
Starting BACnet4Linux with BACnet/IP: ./bacnet4linux/bacnet4linux -d25411 -v999 -p47808 -iwlan0 -e0 -t10 -D2 -q1 -h0
BACnet4Linux: BACnet4Linux Version 0.3.12
BACnet4Linux: LocalIP=192.168.52.12
BACnet4Linux started! Device 25411 using BACnet/IP at 192.168.52.12:47808
System ready!
Device discoverable at 192.168.52.12:47808
Web interface: http://192.168.52.12:5000
 * Running on all addresses (0.0.0.0)
 * Running on http://192.168.52.12:5000
```

## Verification

1. **Web Interface**: Open http://192.168.52.12:5000 in browser
2. **Network Test**: Run `python3 test_bacnet_network.py` in another terminal
3. **Discovery Tools**: Your BACnet discovery tools should find Device ID 25411

## Troubleshooting

If still not working:
1. Check `python3 test_bacnet_network.py` output
2. Verify interface name: `ip route get 8.8.8.8`
3. Check firewall: `sudo ufw status`
4. Test port: `sudo netstat -tulpn | grep 47808`

## File Structure on Pi

```
~/pi_deployment/
├── pi_bacnet4linux_fixed.py    # Main controller
├── test_bacnet_network.py       # Network diagnostic tool
├── bacnet4linux/               # C stack source (compiled for ARM)
├── config.py                   # Configuration
├── gpio_config.json           # GPIO setup
├── NETWORK_FIX.txt            # Technical details
└── templates/, static/         # Web interface
```

The controller uses your industry-standard BACnet4Linux C stack with proper network interface detection and BACnet/IP configuration.