"""
Configuration settings for BACnet GPIO Controller
Contains application-wide configuration parameters
"""

import os
import json

class Config:
    """Application configuration class"""
    
    # Flask configuration
    HOST = '0.0.0.0'
    PORT = int(os.getenv('PORT', 5001))
    DEBUG = os.getenv('DEBUG', 'False').lower() == 'true'
    SECRET_KEY = os.getenv('SECRET_KEY', 'your-secret-key-change-in-production')
    
    # GPIO configuration
    GPIO_CONFIG_FILE = os.getenv('GPIO_CONFIG_FILE', 'gpio_config.json')
    
    # BACnet configuration
    BACNET_DEVICE_ID = int(os.getenv('BACNET_DEVICE_ID', '25411'))
    BACNET_DEVICE_NAME = os.getenv('BACNET_DEVICE_NAME', 'RPi GPIO Controller')
    BACNET_VENDOR_ID = int(os.getenv('BACNET_VENDOR_ID', '999'))
    BACNET_IP_ADDRESS = os.getenv('BACNET_IP_ADDRESS', None)  # Auto-detect if None
    BACNET_PORT = int(os.getenv('BACNET_PORT', '47808'))
    BACNET_NETWORK_NUMBER = int(os.getenv('BACNET_NETWORK_NUMBER', '0'))
    
    # Raspberry Pi specific configuration
    GPIO_MODE = 'BCM'  # BCM or BOARD pin numbering
    
    # Analog configuration (for ADC/DAC)
    SPI_ENABLED = os.getenv('SPI_ENABLED', 'False').lower() == 'true'
    I2C_ENABLED = os.getenv('I2C_ENABLED', 'False').lower() == 'true'
    
    # ADC configuration (MCP3008 example)
    ADC_CHANNELS = int(os.getenv('ADC_CHANNELS', '8'))
    ADC_REFERENCE_VOLTAGE = float(os.getenv('ADC_REFERENCE_VOLTAGE', '3.3'))
    
    # PWM configuration for analog outputs
    PWM_FREQUENCY = int(os.getenv('PWM_FREQUENCY', '1000'))  # Hz
    
    # Application settings
    AUTO_SAVE_CONFIG = os.getenv('AUTO_SAVE_CONFIG', 'True').lower() == 'true'
    CONFIG_BACKUP_COUNT = int(os.getenv('CONFIG_BACKUP_COUNT', '5'))
    
    # Monitoring settings
    MONITORING_INTERVAL = float(os.getenv('MONITORING_INTERVAL', '1.0'))  # seconds
    MAX_LOG_ENTRIES = int(os.getenv('MAX_LOG_ENTRIES', '1000'))
    
    # Safety settings
    EMERGENCY_STOP_PIN = os.getenv('EMERGENCY_STOP_PIN', None)  # GPIO pin for emergency stop
    WATCHDOG_TIMEOUT = int(os.getenv('WATCHDOG_TIMEOUT', '30'))  # seconds
    
    @classmethod
    def validate_config(cls):
        """Validate configuration parameters"""
        errors = []
        
        # Validate BACnet device ID range
        if not (0 <= cls.BACNET_DEVICE_ID <= 4194303):
            errors.append("BACNET_DEVICE_ID must be between 0 and 4194303")
        
        # Validate vendor ID
        if not (0 <= cls.BACNET_VENDOR_ID <= 65535):
            errors.append("BACNET_VENDOR_ID must be between 0 and 65535")
        
        # Validate port numbers
        if not (1024 <= cls.PORT <= 65535):
            errors.append("PORT must be between 1024 and 65535")
        
        if not (1024 <= cls.BACNET_PORT <= 65535):
            errors.append("BACNET_PORT must be between 1024 and 65535")
        
        # Validate GPIO mode
        if cls.GPIO_MODE not in ['BCM', 'BOARD']:
            errors.append("GPIO_MODE must be 'BCM' or 'BOARD'")
        
        # Validate monitoring interval
        if cls.MONITORING_INTERVAL <= 0:
            errors.append("MONITORING_INTERVAL must be greater than 0")
        
        return errors
    
    @classmethod
    def get_config_dict(cls):
        """Return configuration as dictionary"""
        return {
            'host': cls.HOST,
            'port': cls.PORT,
            'debug': cls.DEBUG,
            'bacnet_device_id': cls.BACNET_DEVICE_ID,
            'bacnet_device_name': cls.BACNET_DEVICE_NAME,
            'bacnet_vendor_id': cls.BACNET_VENDOR_ID,
            'bacnet_ip_address': cls.BACNET_IP_ADDRESS,
            'bacnet_port': cls.BACNET_PORT,
            'gpio_mode': cls.GPIO_MODE,
            'monitoring_interval': cls.MONITORING_INTERVAL,
            'auto_save_config': cls.AUTO_SAVE_CONFIG
        }
    
    @classmethod
    def load_from_file(cls, config_file):
        """Load configuration from JSON file"""
        try:
            if os.path.exists(config_file):
                with open(config_file, 'r') as f:
                    config_data = json.load(f)
                
                # Update class attributes
                for key, value in config_data.items():
                    if hasattr(cls, key.upper()):
                        setattr(cls, key.upper(), value)
                
                print(f"Configuration loaded from {config_file}")
            else:
                print(f"Configuration file {config_file} not found, using defaults")
                
        except Exception as e:
            print(f"Error loading configuration from {config_file}: {e}")
    
    @classmethod
    def save_to_file(cls, config_file):
        """Save current configuration to JSON file"""
        try:
            config_data = cls.get_config_dict()
            
            with open(config_file, 'w') as f:
                json.dump(config_data, f, indent=4, sort_keys=True)
            
            print(f"Configuration saved to {config_file}")
            
        except Exception as e:
            print(f"Error saving configuration to {config_file}: {e}")

# Available GPIO pins on Raspberry Pi (BCM numbering)
AVAILABLE_GPIO_PINS = {
    'gpio': list(range(2, 28)),  # GPIO 2-27
    'i2c': [2, 3],  # I2C pins (can be used as GPIO if I2C disabled)
    'spi': [7, 8, 9, 10, 11],  # SPI pins (can be used as GPIO if SPI disabled)
    'uart': [14, 15],  # UART pins (can be used as GPIO if UART disabled)
    'pwm': [12, 13, 18, 19],  # Hardware PWM capable pins
    'reserved': [0, 1]  # Reserved pins (EEPROM)
}

# Pin type definitions
PIN_TYPES = {
    'digital': {
        'input': 'Digital Input',
        'output': 'Digital Output'
    },
    'analog': {
        'input': 'Analog Input (ADC)',
        'output': 'Analog Output (PWM)'
    }
}

# BACnet object types mapping
BACNET_OBJECT_TYPES = {
    'digital_input': 'binaryInput',
    'digital_output': 'binaryOutput',
    'analog_input': 'analogInput',
    'analog_output': 'analogOutput'
}
