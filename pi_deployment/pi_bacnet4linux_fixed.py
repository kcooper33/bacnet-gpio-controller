#!/usr/bin/env python3
"""
BACnet Controller using BACnet4Linux C Stack (Fixed for BACnet/IP)
Device ID: 25411 for Raspberry Pi at 192.168.52.12
Properly configures BACnet4Linux to use BACnet/IP over UDP instead of Ethernet 802.2
"""

import os
import sys
import signal
import atexit
import subprocess
import threading
import time
import json
from flask import Flask, render_template, jsonify

# Configuration for your Pi
DEVICE_ID = 25411
DEVICE_NAME = "RPi GPIO Controller"
VENDOR_ID = 999
PI_IP = "192.168.52.12"
BACNET_PORT = 47808
WEB_PORT = 5000

app = Flask(__name__)
bacnet_process = None
gpio_manager = None

class RPiGPIOManager:
    """GPIO manager for Raspberry Pi"""
    
    def __init__(self):
        self.pins = {
            18: {"name": "Test LED", "type": "binary_output", "value": 0, "enabled": True},
            19: {"name": "Motion Sensor", "type": "binary_input", "value": 0, "enabled": True},
            20: {"name": "Temperature Sensor", "type": "analog_input", "value": 20.5, "enabled": True},
            21: {"name": "Fan Control", "type": "analog_output", "value": 0.0, "enabled": True},
            26: {"name": "Main Relay", "type": "binary_output", "value": 0, "enabled": True}
        }
        
        self.gpio_available = False
        try:
            # Try newer gpiozero first (works on Pi 5 and newer)
            import gpiozero
            self.use_gpiozero = True
            self.gpio_available = True
            self._setup_pins_gpiozero()
            print("GPIO initialized successfully (using gpiozero)")
        except ImportError:
            try:
                # Fallback to RPi.GPIO for older Pi models
                import RPi.GPIO as GPIO
                GPIO.setmode(GPIO.BCM)
                self.use_gpiozero = False
                self.gpio_available = True
                self._setup_pins()
                print("GPIO initialized successfully (using RPi.GPIO)")
            except ImportError:
                print("GPIO simulation mode - no GPIO libraries available")
                self.use_gpiozero = False
            except Exception as e:
                print(f"RPi.GPIO error (trying simulation mode): {e}")
                self.use_gpiozero = False
    
    def _setup_pins_gpiozero(self):
        """Configure GPIO pins using gpiozero (newer Pi models)"""
        from gpiozero import LED, Button, MCP3008
        self.gpio_objects = {}
        
        for pin, config in self.pins.items():
            if not config.get('enabled', False):
                continue
                
            try:
                if config['type'] == 'binary_output':
                    self.gpio_objects[pin] = LED(pin)
                    if config['value']:
                        self.gpio_objects[pin].on()
                    else:
                        self.gpio_objects[pin].off()
                    print(f"GPIO {pin} configured as output: {config['name']}")
                elif config['type'] == 'binary_input':
                    self.gpio_objects[pin] = Button(pin)
                    print(f"GPIO {pin} configured as input: {config['name']}")
                else:
                    print(f"GPIO {pin} simulated: {config['name']} ({config['type']})")
            except Exception as e:
                print(f"GPIO {pin} simulation: {e}")

    def _setup_pins(self):
        """Configure GPIO pins using RPi.GPIO (older Pi models)"""
        if not self.gpio_available:
            return
            
        import RPi.GPIO as GPIO
        for pin, config in self.pins.items():
            if not config.get('enabled', False):
                continue
                
            try:
                if 'output' in config['type']:
                    GPIO.setup(pin, GPIO.OUT)
                    GPIO.output(pin, GPIO.HIGH if config['value'] else GPIO.LOW)
                    print(f"GPIO {pin} configured as output: {config['name']}")
                else:
                    GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
                    print(f"GPIO {pin} configured as input: {config['name']}")
            except Exception as e:
                print(f"Error configuring GPIO {pin}: {e}")
    
    def get_pins(self):
        return self.pins
    
    def cleanup(self):
        if self.gpio_available:
            try:
                import RPi.GPIO as GPIO
                GPIO.cleanup()
                print("GPIO cleanup completed")
            except:
                pass

def compile_bacnet4linux_for_pi():
    """Compile BACnet4Linux for ARM with proper configuration"""
    print("Compiling BACnet4Linux for ARM with BACnet/IP configuration...")
    
    if not os.path.exists('./bacnet4linux'):
        print("Error: bacnet4linux source directory not found!")
        return False
    
    try:
        # Remove any existing x86_64 binary and object files
        subprocess.run(['make', 'clean'], cwd='./bacnet4linux', check=False)
        
        # Remove the executable specifically to force recompilation
        if os.path.exists('./bacnet4linux/bacnet4linux'):
            os.remove('./bacnet4linux/bacnet4linux')
            print("Removed existing x86_64 binary")
        
        # Check architecture
        arch_result = subprocess.run(['uname', '-m'], capture_output=True, text=True)
        print(f"Target architecture: {arch_result.stdout.strip()}")
        
        # Compile for ARM with explicit architecture flags
        compile_env = os.environ.copy()
        compile_env['CC'] = 'gcc'
        compile_env['CFLAGS'] = '-march=native -O2'
        
        print("Starting ARM compilation...")
        result = subprocess.run(
            ['make', 'all'], 
            cwd='./bacnet4linux',
            capture_output=True,
            text=True,
            timeout=300,
            env=compile_env
        )
        
        print(f"Make output: {result.stdout}")
        if result.stderr:
            print(f"Make errors: {result.stderr}")
        
        # Verify the binary was created and is ARM
        if os.path.exists('./bacnet4linux/bacnet4linux'):
            # Check if it's ARM architecture
            file_result = subprocess.run(
                ['file', './bacnet4linux/bacnet4linux'], 
                capture_output=True, 
                text=True
            )
            print(f"Binary info: {file_result.stdout}")
            
            if 'ARM' in file_result.stdout or 'aarch64' in file_result.stdout:
                print("BACnet4Linux compiled successfully for ARM")
                return True
            else:
                print("Warning: Binary may not be ARM architecture")
                return True  # Try anyway
        else:
            print(f"Compilation failed - no binary created")
            print(f"Return code: {result.returncode}")
            return False
            
    except subprocess.TimeoutExpired:
        print("Compilation timed out")
        return False
    except Exception as e:
        print(f"Compilation error: {e}")
        return False

def start_bacnet4linux():
    """Start BACnet4Linux with BACnet/IP configuration"""
    global bacnet_process
    
    # Ensure it's compiled for ARM
    if not os.path.exists('./bacnet4linux/bacnet4linux'):
        print("BACnet4Linux not found, compiling...")
        if not compile_bacnet4linux_for_pi():
            print("Failed to compile BACnet4Linux")
            return False
    
    try:
        # Use the configured network interface
        interface_name = "wlan0"  # Use wlan0 for Pi
        print(f"Using configured network interface: {interface_name}")

        # BACnet4Linux command with proper BACnet/IP configuration
        cmd = [
            './bacnet4linux/bacnet4linux',
            f'-d{DEVICE_ID}',     # Device ID: 25411
            f'-v{VENDOR_ID}',     # Vendor ID: 999  
            f'-p{BACNET_PORT}',   # BACnet/IP UDP port: 47808
            f'-i{interface_name}', # Network interface
            '-e0',                # DISABLE Ethernet 802.2 (this was the problem!)
            '-t10',               # APDU timeout 10 seconds
            '-D2',                # Debug level 2 for more info
            '-q1',                # Enable initial I-Am broadcast
            '-h0'                 # Disable HTTP server
        ]
        
        print(f"Starting BACnet4Linux with BACnet/IP: {' '.join(cmd)}")
        
        # Test if binary exists and is executable
        binary_path = './bacnet4linux/bacnet4linux'
        if not os.path.exists(binary_path):
            print(f"Error: Binary not found at {binary_path}")
            return False
        
        # Check binary architecture
        file_result = subprocess.run(['file', binary_path], capture_output=True, text=True)
        print(f"Binary info: {file_result.stdout.strip()}")
        
        bacnet_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd='.'
        )
        
        # Monitor output with better error detection
        startup_success = False
        error_messages = []
        
        def monitor():
            nonlocal startup_success
            try:
                while bacnet_process and bacnet_process.poll() is None:
                    line = bacnet_process.stdout.readline()
                    if line:
                        line = line.strip()
                        print(f"BACnet4Linux: {line}")
                        
                        # Check for success indicators
                        if "LocalIP=" in line or "Ready to go" in line:
                            startup_success = True
                        
                        # Check for error indicators
                        if any(err in line.lower() for err in ['error', 'failed', 'cannot', 'unable']):
                            error_messages.append(line)
            except Exception as e:
                print(f"Monitor error: {e}")
        
        monitor_thread = threading.Thread(target=monitor, daemon=True)
        monitor_thread.start()
        
        # Wait longer for startup
        time.sleep(10)
        
        if bacnet_process.poll() is None and startup_success:
            print(f"BACnet4Linux started! Device {DEVICE_ID} using BACnet/IP at {PI_IP}:{BACNET_PORT}")
            return True
        else:
            if bacnet_process.poll() is not None:
                print(f"BACnet4Linux exited with code: {bacnet_process.returncode}")
            else:
                print("BACnet4Linux running but no startup confirmation")
            
            if error_messages:
                print("Error messages:")
                for msg in error_messages:
                    print(f"  {msg}")
            
            return bacnet_process.poll() is None  # Return True if still running
            
    except Exception as e:
        print(f"Error starting BACnet4Linux: {e}")
        return False

def stop_bacnet4linux():
    """Stop BACnet4Linux"""
    global bacnet_process
    
    if bacnet_process:
        try:
            bacnet_process.terminate()
            bacnet_process.wait(timeout=5)
            print("BACnet4Linux stopped")
        except:
            try:
                bacnet_process.kill()
                print("BACnet4Linux force stopped")
            except:
                pass
        finally:
            bacnet_process = None

# Flask routes
@app.route('/')
def index():
    return f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>BACnet Pi Controller</title>
        <style>
            body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
            .container {{ max-width: 800px; margin: 0 auto; }}
            .card {{ background: white; padding: 20px; margin: 10px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
            .running {{ color: green; font-weight: bold; }}
            .stopped {{ color: red; font-weight: bold; }}
            h1 {{ text-align: center; color: #333; }}
            h2 {{ color: #555; border-bottom: 2px solid #007acc; padding-bottom: 5px; }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>BACnet GPIO Controller</h1>
            
            <div class="card">
                <h2>Device Status</h2>
                <p><strong>Device:</strong> {DEVICE_NAME} (ID: {DEVICE_ID})</p>
                <p><strong>Network:</strong> {PI_IP}:{BACNET_PORT}</p>
                <p><strong>Implementation:</strong> BACnet4Linux C Stack (BACnet/IP mode)</p>
                <p><strong>Vendor ID:</strong> {VENDOR_ID}</p>
                <p><strong>Status:</strong> <span id="status">Loading...</span></p>
            </div>
            
            <div class="card">
                <h2>GPIO Pin States</h2>
                <div id="pins">Loading...</div>
            </div>
            
            <div class="card">
                <h2>Discovery Information</h2>
                <p>Your BACnet discovery tool should find this device at:</p>
                <ul>
                    <li><strong>IP Address:</strong> {PI_IP}</li>
                    <li><strong>Port:</strong> {BACNET_PORT}</li>
                    <li><strong>Device ID:</strong> {DEVICE_ID}</li>
                    <li><strong>Protocol:</strong> BACnet/IP (UDP)</li>
                    <li><strong>Stack:</strong> Industry-standard BACnet4Linux</li>
                </ul>
                <p><em>Note: Ethernet 802.2 is disabled, using pure BACnet/IP over UDP</em></p>
            </div>
        </div>
        
        <script>
        function updateStatus() {{
            fetch('/api/status').then(r => r.json()).then(data => {{
                document.getElementById('status').innerHTML = data.running ? 
                    '<span class="running">Running</span>' : '<span class="stopped">Stopped</span>';
            }});
            
            fetch('/api/gpio').then(r => r.json()).then(data => {{
                let html = '';
                for (let pin in data.pins) {{
                    let p = data.pins[pin];
                    if (p.enabled) {{
                        html += `<p><strong>GPIO ${{pin}}:</strong> ${{p.name}} (${{p.type}}) = ${{p.value}}</p>`;
                    }}
                }}
                document.getElementById('pins').innerHTML = html || '<p>No GPIO pins configured</p>';
            }});
        }}
        
        setInterval(updateStatus, 2000);
        updateStatus();
        </script>
    </body>
    </html>
    """

@app.route('/api/status')
def api_status():
    global bacnet_process
    return jsonify({
        'running': bacnet_process and bacnet_process.poll() is None,
        'device_id': DEVICE_ID,
        'device_name': DEVICE_NAME,
        'ip': PI_IP,
        'port': BACNET_PORT,
        'implementation': 'BACnet4Linux C Stack'
    })

@app.route('/api/gpio') 
def api_gpio():
    global gpio_manager
    if gpio_manager:
        return jsonify({'pins': gpio_manager.get_pins()})
    return jsonify({'pins': {}})

def cleanup():
    global gpio_manager
    print("\nShutting down...")
    stop_bacnet4linux()
    if gpio_manager:
        gpio_manager.cleanup()

def signal_handler(sig, frame):
    cleanup()
    sys.exit(0)

def main():
    global gpio_manager
    
    print("=" * 70)
    print("BACnet GPIO Controller for Raspberry Pi")
    print("Using BACnet4Linux C Stack - BACnet/IP Mode")
    print(f"Device ID: {DEVICE_ID}")
    print(f"Network: {PI_IP}:{BACNET_PORT}")
    print("=" * 70)
    
    signal.signal(signal.SIGINT, signal_handler)
    atexit.register(cleanup)
    
    try:
        print("Initializing GPIO...")
        gpio_manager = RPiGPIOManager()
        
        print("Starting BACnet4Linux service...")
        if not start_bacnet4linux():
            print("Failed to start BACnet service")
            print("Check that bacnet4linux/ directory is present")
            return 1
        
        print("System ready!")
        print(f"Device discoverable at {PI_IP}:{BACNET_PORT}")
        print(f"Web interface: http://{PI_IP}:{WEB_PORT}")
        print("Using industry-standard BACnet4Linux C implementation")
        
        app.run(host='0.0.0.0', port=WEB_PORT, debug=False)
        
    except KeyboardInterrupt:
        print("Shutdown requested")
    except Exception as e:
        print(f"Error: {e}")
        return 1
    finally:
        cleanup()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())