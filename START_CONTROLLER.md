# Starting Your BACnet Pi Controller

## Step 1: Transfer Files to Pi

Download and copy the deployment package to your Raspberry Pi:

```bash
scp bacnet_pi_controller.tar.gz pi@192.168.52.12:~/
```

## Step 2: Extract and Setup

On your Raspberry Pi:

```bash
ssh pi@192.168.52.12
tar -xzf bacnet_pi_controller.tar.gz
cd pi_deployment
```

## Step 3: Install Dependencies

```bash
sudo apt update
sudo apt install python3-flask python3-rpi.gpio build-essential
```

## Step 4: Start the Controller

```bash
python3 pi_bacnet4linux_fixed.py
```

## Expected Output

When successfully started, you'll see:

```
======================================================================
BACnet GPIO Controller for Raspberry Pi
Using BACnet4Linux C Stack - BACnet/IP Mode
Device ID: 25411
Network: 192.168.52.12:47808
======================================================================
Initializing GPIO...
GPIO 18 configured as output: Test LED
GPIO 19 configured as input: Motion Sensor
GPIO 20 configured as input: Temperature Sensor
GPIO 21 configured as output: Fan Control
GPIO 26 configured as output: Main Relay
GPIO initialized successfully
Starting BACnet4Linux service...
Compiling BACnet4Linux for ARM with BACnet/IP configuration...
BACnet4Linux compiled successfully for ARM
Starting BACnet4Linux with BACnet/IP: ./bacnet4linux/bacnet4linux -d25411 -v999 -p47808 -e0 -t10 -D1 -q0 -h0
BACnet4Linux started! Device 25411 using BACnet/IP at 192.168.52.12:47808
System ready!
Device discoverable at 192.168.52.12:47808
Web interface: http://192.168.52.12:5000
Using industry-standard BACnet4Linux C implementation
 * Running on all addresses (0.0.0.0)
 * Running on http://127.0.0.1:5000
 * Running on http://192.168.52.12:5000
```

## Verification

1. **Web Interface:** Open http://192.168.52.12:5000 in your browser
2. **BACnet Discovery:** Your discovery tools should find Device ID 25411
3. **Network Test:** `sudo netstat -tulpn | grep 47808` should show the service listening

## Key Configuration

- **BACnet4Linux uses BACnet/IP over UDP** (not Ethernet 802.2)
- **Device ID:** 25411
- **Network:** 192.168.52.12:47808
- **Ethernet disabled:** `-e0` parameter prevents socket errors
- **ARM compilation:** Happens automatically on your Pi

Your controller is now running with the industry-standard BACnet4Linux C stack and should be discoverable by all BACnet tools.