"""
GPIO Manager for Raspberry Pi BACnet Controller
Handles GPIO pin configuration, reading, and writing operations
"""

import json
import os
import threading
import time
from datetime import datetime
from typing import Dict, Any, Optional, Union

# Conditional imports for Raspberry Pi 5 compatibility
try:
    from gpiozero import LED, Button, PWMLED, MCP3008
    GPIOZERO_AVAILABLE = True
    print("Using gpiozero for Raspberry Pi 5 compatibility")
except ImportError:
    GPIOZERO_AVAILABLE = False
    print("Warning: gpiozero not available. Running in simulation mode.")

try:
    import spidev
    SPI_AVAILABLE = True
except ImportError:
    SPI_AVAILABLE = False
    spidev = None
    print("Warning: spidev not available. Analog inputs disabled.")

class GPIOManager:
    """Manages GPIO pin configuration and operations"""
    
    def __init__(self, config_file='gpio_config.json'):
        """Initialize GPIO manager"""
        self.config_file = config_file
        self.configuration = {}
        self.pin_locks = {}
        self.pwm_instances = {}
        self.adc = None
        self.config_lock = threading.Lock()
        self.simulation_mode = not RASPBERRY_PI_AVAILABLE
        
        # Initialize GPIO if available
        if RASPBERRY_PI_AVAILABLE and GPIO:
            try:
                GPIO.setmode(GPIO.BCM)
                GPIO.setwarnings(False)
                print("GPIO initialized in BCM mode")
            except Exception as e:
                print(f"Error initializing GPIO: {e}")
                self.simulation_mode = True
        
        # Initialize SPI for analog inputs if available
        if SPI_AVAILABLE and spidev and not self.simulation_mode:
            try:
                self.adc = spidev.SpiDev()
                self.adc.open(0, 0)  # Bus 0, Device 0
                self.adc.max_speed_hz = 1000000
                self.adc.mode = 0
                print("SPI ADC initialized successfully")
            except Exception as e:
                print(f"Info: SPI ADC not available ({e}). Analog inputs will be disabled.")
                print("To enable analog inputs: run 'sudo raspi-config' → Interface Options → SPI → Enable")
                self.adc = None
        
        # Load existing configuration
        self.load_configuration()
    
    def load_configuration(self):
        """Load GPIO configuration from file"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    self.configuration = json.load(f)
                print(f"GPIO configuration loaded from {self.config_file}")
                
                # Apply loaded configuration
                self._apply_configuration()
            else:
                print(f"No configuration file found at {self.config_file}")
                self.configuration = {}
                
        except Exception as e:
            print(f"Error loading GPIO configuration: {e}")
            self.configuration = {}
    
    def save_configuration(self):
        """Save current GPIO configuration to file"""
        try:
            with self.config_lock:
                # Create backup
                backup_file = f"{self.config_file}.backup"
                if os.path.exists(self.config_file):
                    os.rename(self.config_file, backup_file)
                
                # Save current configuration
                with open(self.config_file, 'w') as f:
                    json.dump(self.configuration, f, indent=4, sort_keys=True)
                
                print(f"GPIO configuration saved to {self.config_file}")
                
        except Exception as e:
            print(f"Error saving GPIO configuration: {e}")
            # Restore backup if save failed
            backup_file = f"{self.config_file}.backup"
            if os.path.exists(backup_file):
                os.rename(backup_file, self.config_file)
    

    
    def update_configuration(self, pin_config: Dict[str, Dict[str, Any]]):
        """Update GPIO configuration"""
        try:
            # Clean up old configuration first (outside lock to prevent deadlock)
            self._cleanup_pins()
            
            # Build new configuration
            new_configuration = {}
            for pin_str, config in pin_config.items():
                pin = int(pin_str)
                new_configuration[pin_str] = {
                    'direction': config.get('direction', 'input'),
                    'type': config.get('type', 'digital'),
                    'name': config.get('name', f'GPIO{pin}'),
                    'description': config.get('description', ''),
                    'pull_up_down': config.get('pull_up_down', 'none'),
                    'initial_value': config.get('initial_value', 0),
                    'pwm_frequency': config.get('pwm_frequency', 1000),
                    'enabled': config.get('enabled', True)
                }
            
            # Update configuration atomically
            with self.config_lock:
                self.configuration = new_configuration
            
            # Apply new configuration
            self._apply_configuration()
            
            # Save to file (outside lock)
            self.save_configuration()
            
            print(f"Configuration updated successfully with {len(pin_config)} pins")
            
        except Exception as e:
            print(f"Error updating configuration: {e}")
            raise
    
    def _apply_configuration(self):
        """Apply the current configuration to GPIO pins"""
        for pin_str, config in self.configuration.items():
            if not config.get('enabled', True):
                continue
                
            try:
                pin = int(pin_str)
                self._configure_pin(pin, config)
                
            except Exception as e:
                print(f"Error configuring pin {pin_str}: {e}")
    
    def _configure_pin(self, pin: int, config: Dict[str, Any]):
        """Configure a single GPIO pin"""
        if self.simulation_mode or not GPIO:
            print(f"[SIM] Configuring pin {pin}: {config}")
            return
        
        try:
            direction = config['direction']
            pin_type = config['type']
            
            # Create pin lock if not exists
            if pin not in self.pin_locks:
                self.pin_locks[pin] = threading.Lock()
            
            with self.pin_locks[pin]:
                # Clean up existing PWM instance
                if pin in self.pwm_instances:
                    self.pwm_instances[pin].stop()
                    del self.pwm_instances[pin]
                
                if direction == 'input':
                    # Configure as input
                    pull_up_down = config.get('pull_up_down', 'none')
                    pud = GPIO.PUD_OFF
                    if pull_up_down == 'up':
                        pud = GPIO.PUD_UP
                    elif pull_up_down == 'down':
                        pud = GPIO.PUD_DOWN
                    
                    GPIO.setup(pin, GPIO.IN, pull_up_down=pud)
                    
                elif direction == 'output':
                    # Configure as output
                    initial_value = config.get('initial_value', 0)
                    GPIO.setup(pin, GPIO.OUT, initial=initial_value)
                    
                    # Setup PWM for analog output
                    if pin_type == 'analog':
                        frequency = config.get('pwm_frequency', 1000)
                        pwm = GPIO.PWM(pin, frequency)
                        pwm.start(initial_value)
                        self.pwm_instances[pin] = pwm
                
                print(f"Pin {pin} configured as {direction} ({pin_type})")
                
        except Exception as e:
            print(f"Error configuring pin {pin}: {e}")
            raise
    
    def read_pin(self, pin: int) -> Dict[str, Any]:
        """Read value from GPIO pin"""
        pin_str = str(pin)
        
        if pin_str not in self.configuration:
            raise ValueError(f"Pin {pin} not configured")
        
        config = self.configuration[pin_str]
        
        if not config.get('enabled', True):
            return {'error': f'Pin {pin} is disabled'}
        
        try:
            direction = config['direction']
            pin_type = config['type']
            
            if direction != 'input':
                # For outputs, return the last set value
                if pin_type == 'analog' and pin in self.pwm_instances:
                    # Get PWM duty cycle (approximate)
                    return {
                        'value': 'unknown',  # PWM duty cycle not directly readable
                        'type': 'analog_output',
                        'timestamp': datetime.now().isoformat()
                    }
                else:
                    return {
                        'value': 'unknown',  # Digital output state not cached
                        'type': 'digital_output',
                        'timestamp': datetime.now().isoformat()
                    }
            
            if self.simulation_mode:
                # Return simulated values
                import random
                if pin_type == 'analog':
                    value = round(random.uniform(0, 3.3), 2)
                else:
                    value = random.choice([0, 1])
                
                return {
                    'value': value,
                    'type': f'{pin_type}_input',
                    'timestamp': datetime.now().isoformat(),
                    'simulated': True
                }
            
            # Read actual GPIO value
            if pin_type == 'digital':
                # Digital input
                with self.pin_locks.get(pin, threading.Lock()):
                    value = GPIO.input(pin)
                
                return {
                    'value': value,
                    'type': 'digital_input',
                    'timestamp': datetime.now().isoformat()
                }
            
            elif pin_type == 'analog':
                # Analog input via ADC
                if self.adc is None:
                    return {
                        'error': 'ADC not available',
                        'timestamp': datetime.now().isoformat()
                    }
                
                # Read from MCP3008 ADC (assuming pin maps to ADC channel)
                channel = pin % 8  # Map GPIO pin to ADC channel
                adc_value = self._read_adc_channel(channel)
                voltage = (adc_value / 1023.0) * 3.3  # Convert to voltage
                
                return {
                    'value': round(voltage, 3),
                    'raw_adc': adc_value,
                    'type': 'analog_input',
                    'channel': channel,
                    'timestamp': datetime.now().isoformat()
                }
            
        except Exception as e:
            return {
                'error': str(e),
                'timestamp': datetime.now().isoformat()
            }
    
    def write_pin(self, pin: int, value: Union[int, float]):
        """Write value to GPIO pin"""
        pin_str = str(pin)
        
        if pin_str not in self.configuration:
            raise ValueError(f"Pin {pin} not configured")
        
        config = self.configuration[pin_str]
        
        if not config.get('enabled', True):
            raise ValueError(f"Pin {pin} is disabled")
        
        if config['direction'] != 'output':
            raise ValueError(f"Pin {pin} is not configured as output")
        
        if self.simulation_mode:
            print(f"[SIM] Writing {value} to pin {pin}")
            return
        
        try:
            pin_type = config['type']
            
            with self.pin_locks.get(pin, threading.Lock()):
                if pin_type == 'digital':
                    # Digital output
                    GPIO.output(pin, int(value))
                    
                elif pin_type == 'analog':
                    # Analog output via PWM
                    if pin in self.pwm_instances:
                        # Convert value to duty cycle (0-100)
                        if isinstance(value, float) and 0 <= value <= 3.3:
                            duty_cycle = (value / 3.3) * 100
                        else:
                            duty_cycle = max(0, min(100, float(value)))
                        
                        self.pwm_instances[pin].ChangeDutyCycle(duty_cycle)
                    else:
                        raise ValueError(f"PWM not initialized for pin {pin}")
            
            print(f"Pin {pin} set to {value}")
            
        except Exception as e:
            print(f"Error writing to pin {pin}: {e}")
            raise
    
    def _read_adc_channel(self, channel: int) -> int:
        """Read value from ADC channel (MCP3008)"""
        if self.adc is None:
            return 0
        
        try:
            # Send command to MCP3008
            cmd = 0x18 | channel  # Start bit + single-ended + channel
            reply = self.adc.xfer2([cmd << 4, 0, 0])
            
            # Extract 10-bit value
            value = ((reply[0] & 0x03) << 8) | reply[1]
            return value
            
        except Exception as e:
            print(f"Error reading ADC channel {channel}: {e}")
            return 0
    
    def _cleanup_pins(self):
        """Clean up all configured GPIO pins"""
        if self.simulation_mode:
            return
        
        try:
            # Stop all PWM instances
            for pwm in self.pwm_instances.values():
                try:
                    pwm.stop()
                except:
                    pass
            self.pwm_instances.clear()
            
            # Reset configured pins
            for pin_str in self.configuration.keys():
                try:
                    pin = int(pin_str)
                    GPIO.setup(pin, GPIO.IN)  # Reset to input
                except:
                    pass
            
            print("GPIO pins cleaned up")
            
        except Exception as e:
            print(f"Error during GPIO cleanup: {e}")
    
    def get_configuration(self) -> Dict[str, Dict[str, Any]]:
        """Get current GPIO configuration"""
        with self.config_lock:
            return self.configuration.copy()
    
    def get_pin_info(self, pin: int) -> Dict[str, Any]:
        """Get detailed information about a specific pin"""
        pin_str = str(pin)
        
        info = {
            'pin': pin,
            'configured': pin_str in self.configuration,
            'available': 2 <= pin <= 27,
            'special_function': self._get_special_function(pin)
        }
        
        if info['configured']:
            config = self.configuration[pin_str]
            info.update({
                'direction': config['direction'],
                'type': config['type'],
                'name': config.get('name', f'GPIO{pin}'),
                'description': config.get('description', ''),
                'enabled': config.get('enabled', True)
            })
            
            # Get current state
            try:
                state = self.read_pin(pin)
                info['current_state'] = state
            except Exception as e:
                info['current_state'] = {'error': str(e)}
        
        return info
    
    def _get_special_function(self, pin: int) -> Optional[str]:
        """Get special function of GPIO pin if any"""
        special_pins = {
            2: 'I2C SDA',
            3: 'I2C SCL',
            7: 'SPI CE1',
            8: 'SPI CE0',
            9: 'SPI MISO',
            10: 'SPI MOSI',
            11: 'SPI SCLK',
            14: 'UART TXD',
            15: 'UART RXD',
            18: 'PWM0',
            19: 'PWM1'
        }
        
        return special_pins.get(pin)
    
    def cleanup(self):
        """Cleanup GPIO resources"""
        print("Cleaning up GPIO resources...")
        
        self._cleanup_pins()
        
        if self.adc:
            try:
                self.adc.close()
                print("SPI ADC closed")
            except:
                pass
        
        if RASPBERRY_PI_AVAILABLE and not self.simulation_mode:
            try:
                GPIO.cleanup()
                print("GPIO cleanup completed")
            except:
                pass
