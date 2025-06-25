#!/usr/bin/env python3
"""
BACnet GPIO Controller for Raspberry Pi - Configuration File Based
Reads all parameters from bacnet_config.json
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

# Load configuration
CONFIG_FILE = os.getenv('BACNET_CONFIG_FILE', 'bacnet_config.json')

def load_config():
    """Load configuration from JSON file"""
    try:
        with open(CONFIG_FILE, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: Configuration file {CONFIG_FILE} not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in {CONFIG_FILE}: {e}")
        sys.exit(1)

config = load_config()

# Extract configuration values
DEVICE_ID = config['device']['id']
DEVICE_NAME = config['device']['name']
VENDOR_ID = config['device']['vendor_id']
PI_IP = config['network']['ip']
BACNET_PORT = config['network']['port']
NETWORK_INTERFACE = config['network']['interface']
WEB_PORT = config['web_interface']['port']

app = Flask(__name__)
bacnet_process = None
gpio_manager = None

class ConfigurableGPIOManager:
    """GPIO manager that reads pin configuration from config file"""
    
    def __init__(self):
        self.pins = {}
        self.gpio_objects = {}
        self.gpio_available = False
        self.use_gpiozero = False
        
        # Load GPIO configuration
        for pin_str, pin_config in config['gpio_pins'].items():
            if pin_config.get('enabled', True):
                pin = int(pin_str)
                self.pins[pin] = pin_config
        
        self._initialize_gpio()
    
    def _initialize_gpio(self):
        """Initialize GPIO with fallback support"""
        try:
            import gpiozero
            self.use_gpiozero = True
            self.gpio_available = True
            self._setup_pins_gpiozero()
            print("GPIO initialized successfully (using gpiozero)")
        except ImportError:
            try:
                import RPi.GPIO as GPIO
                GPIO.setmode(GPIO.BCM)
                self.use_gpiozero = False
                self.gpio_available = True
                self._setup_pins_rpi()
                print("GPIO initialized successfully (using RPi.GPIO)")
            except ImportError:
                print("GPIO simulation mode - no GPIO libraries available")
                self.use_gpiozero = False
            except Exception as e:
                print(f"RPi.GPIO error (using simulation mode): {e}")
                self.use_gpiozero = False

    def _setup_pins_gpiozero(self):
        """Configure GPIO pins using gpiozero"""
        from gpiozero import LED, Button
        
        for pin, pin_config in self.pins.items():
            try:
                if pin_config['type'] == 'binary_output':
                    self.gpio_objects[pin] = LED(pin)
                    if pin_config.get('initial_value', 0):
                        self.gpio_objects[pin].on()
                    else:
                        self.gpio_objects[pin].off()
                    print(f"GPIO {pin} configured as output: {pin_config['name']}")
                elif pin_config['type'] == 'binary_input':
                    self.gpio_objects[pin] = Button(pin)
                    print(f"GPIO {pin} configured as input: {pin_config['name']}")
                else:
                    print(f"GPIO {pin} simulated: {pin_config['name']} ({pin_config['type']})")
            except Exception as e:
                print(f"GPIO {pin} simulation: {e}")

    def _setup_pins_rpi(self):
        """Configure GPIO pins using RPi.GPIO"""
        if not self.gpio_available:
            return
            
        import RPi.GPIO as GPIO
        for pin, pin_config in self.pins.items():
            try:
                if 'output' in pin_config['type']:
                    GPIO.setup(pin, GPIO.OUT)
                    GPIO.output(pin, GPIO.HIGH if pin_config.get('initial_value', 0) else GPIO.LOW)
                    print(f"GPIO {pin} configured as output: {pin_config['name']}")
                else:
                    GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
                    print(f"GPIO {pin} configured as input: {pin_config['name']}")
            except Exception as e:
                print(f"Error configuring GPIO {pin}: {e}")
    
    def get_pins(self):
        return self.pins
    
    def cleanup(self):
        if self.gpio_available and not self.use_gpiozero:
            try:
                import RPi.GPIO as GPIO
                GPIO.cleanup()
                print("GPIO cleanup completed")
            except:
                pass

def compile_bacnet4linux():
    """Compile BACnet4Linux for ARM"""
    print("Compiling BACnet4Linux for ARM...")
    
    if not os.path.exists('./bacnet4linux'):
        print("Error: bacnet4linux source directory not found!")
        return False
    
    try:
        subprocess.run(['make', 'clean'], cwd='./bacnet4linux', check=False)
        
        if os.path.exists('./bacnet4linux/bacnet4linux'):
            os.remove('./bacnet4linux/bacnet4linux')
            print("Removed existing binary")
        
        compile_env = os.environ.copy()
        compile_env['CC'] = 'gcc'
        compile_env['CFLAGS'] = '-march=native -O2'
        
        result = subprocess.run(
            ['make', 'all'], 
            cwd='./bacnet4linux',
            capture_output=True,
            text=True,
            timeout=300,
            env=compile_env
        )
        
        if os.path.exists('./bacnet4linux/bacnet4linux'):
            file_result = subprocess.run(
                ['file', './bacnet4linux/bacnet4linux'], 
                capture_output=True, 
                text=True
            )
            print(f"Binary compiled: {file_result.stdout.strip()}")
            return True
        else:
            print(f"Compilation failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"Compilation error: {e}")
        return False

def start_bacnet4linux():
    """Start BACnet4Linux with configuration parameters"""
    global bacnet_process
    
    if not os.path.exists('./bacnet4linux/bacnet4linux'):
        print("BACnet4Linux not found, compiling...")
        if not compile_bacnet4linux():
            return False
    
    try:
        # Auto-detect the actual network interface 
        detected_interface = NETWORK_INTERFACE  # Default from config
        try:
            # Get default route interface
            result = subprocess.run(['ip', 'route', 'show', 'default'], capture_output=True, text=True)
            if result.returncode == 0 and 'dev' in result.stdout:
                # Extract interface name after 'dev'
                parts = result.stdout.split()
                for i, part in enumerate(parts):
                    if part == 'dev' and i + 1 < len(parts):
                        detected_interface = parts[i + 1]
                        break
        except:
            pass
        
        print(f"Using network interface: {detected_interface}")
        
        # Build command from configuration
        ethernet_flag = "1" if config['bacnet_options']['ethernet_enable'] else "0"
        query_flag = "1" if config['bacnet_options']['initial_query'] else "0"
        http_flag = "1" if config['bacnet_options']['http_server'] else "0"
        
        cmd = [
            './bacnet4linux/bacnet4linux',
            f'-d{DEVICE_ID}',
            f'-v{VENDOR_ID}',
            f'-p{BACNET_PORT}',
            f'-i{detected_interface}',
            f'-e{ethernet_flag}',
            f'-t{config["bacnet_options"]["apdu_timeout"]}',
            f'-D{config["bacnet_options"]["debug_level"]}',
            f'-q{query_flag}',
            f'-h{http_flag}'
        ]
        
        print(f"Starting BACnet4Linux: {' '.join(cmd)}")
        
        bacnet_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd='.'
        )
        
        startup_success = False
        
        def monitor():
            nonlocal startup_success
            try:
                while bacnet_process and bacnet_process.poll() is None:
                    line = bacnet_process.stdout.readline()
                    if line:
                        line = line.strip()
                        print(f"BACnet4Linux: {line}")
                        if "LocalIP=" in line or "Ready to go" in line:
                            startup_success = True
            except Exception as e:
                print(f"Monitor error: {e}")
        
        threading.Thread(target=monitor, daemon=True).start()
        time.sleep(8)
        
        if bacnet_process.poll() is None and startup_success:
            print(f"BACnet4Linux started! Device {DEVICE_ID} at {PI_IP}:{BACNET_PORT}")
            return True
        else:
            if bacnet_process.poll() is not None:
                print(f"BACnet4Linux exited with code: {bacnet_process.returncode}")
            return bacnet_process.poll() is None
            
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
    gpio_pins_html = ""
    for pin, pin_config in config['gpio_pins'].items():
        if pin_config.get('enabled', True):
            gpio_pins_html += f"<p><strong>GPIO {pin}:</strong> {pin_config['name']} ({pin_config['type']})</p>"
    
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
        </style>
    </head>
    <body>
        <div class="container">
            <h1>BACnet GPIO Controller</h1>
            
            <div class="card">
                <h2>Device Configuration</h2>
                <p><strong>Device:</strong> {DEVICE_NAME} (ID: {DEVICE_ID})</p>
                <p><strong>Network:</strong> {PI_IP}:{BACNET_PORT} on {NETWORK_INTERFACE}</p>
                <p><strong>Vendor ID:</strong> {VENDOR_ID}</p>
                <p><strong>Status:</strong> <span id="status">Loading...</span></p>
            </div>
            
            <div class="card">
                <h2>GPIO Pins</h2>
                {gpio_pins_html}
            </div>
            
            <div class="card">
                <h2>Configuration File</h2>
                <p><strong>Config:</strong> {CONFIG_FILE}</p>
                <p>Edit the configuration file to modify device settings and GPIO pins.</p>
            </div>
        </div>
        
        <script>
        function updateStatus() {{
            fetch('/api/status').then(r => r.json()).then(data => {{
                document.getElementById('status').innerHTML = data.running ? 
                    '<span class="running">Running</span>' : '<span class="stopped">Stopped</span>';
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
        'interface': NETWORK_INTERFACE,
        'config_file': CONFIG_FILE
    })

@app.route('/api/config')
def api_config():
    return jsonify(config)

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
    print("Configuration-Based BACnet4Linux Implementation")
    print(f"Device: {DEVICE_NAME} (ID: {DEVICE_ID})")
    print(f"Network: {NETWORK_INTERFACE} - {PI_IP}:{BACNET_PORT}")
    print(f"Config: {CONFIG_FILE}")
    print("=" * 70)
    
    signal.signal(signal.SIGINT, signal_handler)
    atexit.register(cleanup)
    
    try:
        print("Initializing GPIO...")
        gpio_manager = ConfigurableGPIOManager()
        
        print("Starting BACnet4Linux service...")
        if not start_bacnet4linux():
            print("Failed to start BACnet service")
            return 1
        
        print("System ready!")
        print(f"Device discoverable at {PI_IP}:{BACNET_PORT}")
        if config['web_interface']['enabled']:
            print(f"Web interface: http://{PI_IP}:{WEB_PORT}")
        
        if config['web_interface']['enabled']:
            app.run(host=config['web_interface']['host'], port=WEB_PORT, debug=False)
        else:
            # Keep running without web interface
            while True:
                time.sleep(1)
        
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