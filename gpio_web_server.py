#!/usr/bin/env python3
"""
Standalone GPIO Configuration Web Interface
Runs independently to manage GPIO pin configuration for BACnet controller
"""

from flask import Flask, render_template, jsonify, request
import json
import os

app = Flask(__name__)

def create_default_gpio_config():
    """Create default GPIO configuration for all 24 pins"""
    config = {'gpio_pins': {}}
    
    for i in range(24):  # GPIO 0-23
        pin_str = str(i)
        instance = 24 if i == 0 else i  # GPIO 0 gets instance 24
        
        # Set defaults for existing configured pins
        if i == 18:
            config['gpio_pins'][pin_str] = {
                'name': 'Test LED',
                'direction': 'output',
                'high_unit': 'ON',
                'low_unit': 'OFF',
                'enabled': True,
                'instance': instance
            }
        elif i == 19:
            config['gpio_pins'][pin_str] = {
                'name': 'Motion Sensor',
                'direction': 'input',
                'high_unit': 'Motion',
                'low_unit': 'No Motion',
                'enabled': True,
                'instance': instance
            }
        else:
            config['gpio_pins'][pin_str] = {
                'name': f'GPIO {i}',
                'direction': 'input',
                'high_unit': 'High',
                'low_unit': 'Low',
                'enabled': False,
                'instance': instance
            }
    
    return config

@app.route('/')
def index():
    """Main page with navigation"""
    enabled_pins = 0
    try:
        with open('gpio_pin_config.json', 'r') as f:
            config = json.load(f)
            enabled_pins = len([p for p in config['gpio_pins'].values() if p.get('enabled')])
    except:
        pass
    
    return f'''
    <!DOCTYPE html>
    <html>
    <head>
        <title>BACnet GPIO Controller</title>
        <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
    </head>
    <body>
        <div class="container mt-5">
            <div class="row justify-content-center">
                <div class="col-md-8">
                    <div class="card">
                        <div class="card-header bg-primary text-white">
                            <h1 class="card-title mb-0">BACnet GPIO Controller</h1>
                        </div>
                        <div class="card-body">
                            <p class="card-text">Configure GPIO pins for BACnet integration</p>
                            
                            <div class="alert alert-info">
                                <strong>Status:</strong> {enabled_pins} GPIO pins currently enabled for BACnet objects
                            </div>
                            
                            <div class="d-grid gap-2">
                                <a href="/gpio_config" class="btn btn-primary btn-lg">
                                    GPIO Pin Configuration
                                </a>
                                <a href="/api/gpio_config" class="btn btn-outline-secondary">
                                    View Raw Configuration (JSON)
                                </a>
                            </div>
                            
                            <hr>
                            <h5>Instructions:</h5>
                            <ul>
                                <li>Configure all 24 GPIO pins (GPIO 0-23)</li>
                                <li>Set custom names, directions, and active/inactive text</li>
                                <li>Enable pins to create BACnet objects automatically</li>
                                <li>Instance mapping: GPIO 0 → Instance 24, others direct mapping</li>
                            </ul>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
    </html>
    '''

@app.route('/gpio_config')
def gpio_config_page():
    """GPIO configuration interface"""
    try:
        with open('gpio_pin_config.json', 'r') as f:
            gpio_config = json.load(f)
    except FileNotFoundError:
        gpio_config = create_default_gpio_config()
        with open('gpio_pin_config.json', 'w') as f:
            json.dump(gpio_config, f, indent=2)
    
    return render_template('gpio_config.html', gpio_config=gpio_config)

@app.route('/api/gpio_config', methods=['GET', 'POST'])
def api_gpio_config():
    """GPIO configuration API"""
    if request.method == 'GET':
        try:
            with open('gpio_pin_config.json', 'r') as f:
                return jsonify(json.load(f))
        except FileNotFoundError:
            config = create_default_gpio_config()
            with open('gpio_pin_config.json', 'w') as f:
                json.dump(config, f, indent=2)
            return jsonify(config)
    
    elif request.method == 'POST':
        try:
            config = request.get_json()
            if not config or 'gpio_pins' not in config:
                return jsonify({'success': False, 'error': 'Invalid configuration format'})
            
            # Save configuration
            with open('gpio_pin_config.json', 'w') as f:
                json.dump(config, f, indent=2)
            
            # Generate BACnet configuration
            generate_bacnet_config_from_gpio(config)
            
            enabled_count = len([p for p in config['gpio_pins'].values() if p.get('enabled')])
            print(f'Configuration saved: {enabled_count} GPIO pins enabled')
            
            return jsonify({
                'success': True, 
                'message': f'Configuration saved successfully. {enabled_count} pins enabled.',
                'enabled_pins': enabled_count
            })
        except Exception as e:
            return jsonify({'success': False, 'error': str(e)})

def generate_bacnet_config_from_gpio(gpio_config):
    """Generate BACnet object configuration from GPIO settings"""
    bacnet_objects = []
    
    for pin_str, pin_config in gpio_config['gpio_pins'].items():
        if not pin_config.get('enabled', False):
            continue
            
        pin_num = int(pin_str)
        instance = pin_config['instance']
        
        if pin_config['direction'] == 'input':
            obj = {
                'object_type': 'binary_input',
                'instance': 3000 + instance,  # Binary Input instances
                'name': pin_config['name'],
                'gpio_pin': pin_num,
                'active_text': pin_config['high_unit'],
                'inactive_text': pin_config['low_unit']
            }
        else:  # output
            obj = {
                'object_type': 'binary_output',
                'instance': 4000 + instance,  # Binary Output instances
                'name': pin_config['name'],
                'gpio_pin': pin_num,
                'active_text': pin_config['high_unit'],
                'inactive_text': pin_config['low_unit']
            }
        
        bacnet_objects.append(obj)
    
    # Save BACnet configuration
    bacnet_config = {
        'device_id': 25411,
        'device_name': 'RPi GPIO Controller',
        'generated_at': 'Configuration interface',
        'total_objects': len(bacnet_objects),
        'objects': bacnet_objects
    }
    
    with open('gpio_bacnet_config.json', 'w') as f:
        json.dump(bacnet_config, f, indent=2)
    
    print(f'Generated BACnet configuration with {len(bacnet_objects)} objects')

if __name__ == '__main__':
    print("="*60)
    print("GPIO Configuration Web Interface")
    print("="*60)
    print("Starting server on port 5000...")
    print("Access at: http://192.168.52.12:5000")
    print("GPIO Config: http://192.168.52.12:5000/gpio_config")
    print("="*60)
    
    # Ensure templates directory exists
    if not os.path.exists('templates'):
        print("Error: templates/ directory not found")
        print("Please ensure gpio_config.html template is available")
        exit(1)
    
    app.run(host='0.0.0.0', port=5000, debug=False)