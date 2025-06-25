#!/usr/bin/env python3
"""
BACnet Network Test - Diagnose Why Discovery Isn't Working
Tests network binding, packet capture, and BACnet communication
"""

import socket
import struct
import time
import subprocess
import threading

PI_IP = "192.168.52.12"
BACNET_PORT = 47808
DEVICE_ID = 25411

def test_port_binding():
    """Test if port 47808 is properly bound"""
    print("Testing port binding...")
    try:
        # Check if port is in use
        result = subprocess.run(['ss', '-tulpn'], capture_output=True, text=True)
        if '47808' in result.stdout:
            print("✓ Port 47808 is bound")
            for line in result.stdout.split('\n'):
                if '47808' in line:
                    print(f"  {line}")
        else:
            print("✗ Port 47808 is NOT bound")
            
        # Try to bind to the port ourselves
        test_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        test_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            test_sock.bind(('0.0.0.0', BACNET_PORT))
            print("✗ Port 47808 is available (not in use by BACnet4Linux)")
            test_sock.close()
        except OSError as e:
            print(f"✓ Port 47808 is in use: {e}")
            
    except Exception as e:
        print(f"Port test error: {e}")

def send_whois():
    """Send a Who-Is packet to test BACnet response"""
    print("\nSending Who-Is packet...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.settimeout(5.0)
        
        # Who-Is packet format
        whois_packet = bytes([
            0x81, 0x0b, 0x00, 0x0c,  # BVLC header
            0x01, 0x20, 0xff, 0xff, 0x00, 0xff,  # NPDU
            0x10, 0x08  # Who-Is service
        ])
        
        print(f"Sending to broadcast 192.168.52.255:{BACNET_PORT}")
        sock.sendto(whois_packet, ('192.168.52.255', BACNET_PORT))
        
        # Listen for responses
        print("Listening for I-Am responses...")
        start_time = time.time()
        responses = 0
        
        while time.time() - start_time < 5:
            try:
                data, addr = sock.recvfrom(1024)
                responses += 1
                hex_data = ' '.join(f'{b:02x}' for b in data)
                print(f"Response {responses} from {addr[0]}: {hex_data}")
                
                # Check if it's an I-Am from our device
                if len(data) >= 20 and data[10:12] == bytes([0x10, 0x00]):
                    # Extract device ID
                    if len(data) >= 17:
                        device_bytes = data[13:17]
                        if len(device_bytes) == 4:
                            device_id = struct.unpack('>I', device_bytes)[0] & 0x3FFFFF
                            print(f"  I-Am from Device ID: {device_id}")
                            if device_id == DEVICE_ID:
                                print("  ✓ This is our device!")
                
            except socket.timeout:
                break
                
        if responses == 0:
            print("✗ No I-Am responses received")
        else:
            print(f"✓ Received {responses} I-Am responses")
            
        sock.close()
        
    except Exception as e:
        print(f"Who-Is test error: {e}")

def test_direct_connection():
    """Test direct connection to BACnet4Linux"""
    print("\nTesting direct connection...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(2.0)
        
        # Send Who-Is directly to our Pi
        whois_packet = bytes([
            0x81, 0x0b, 0x00, 0x0c,
            0x01, 0x00, 0x20, 0xff, 0xff, 0x00, 0xff,
            0x10, 0x08
        ])
        
        print(f"Sending Who-Is directly to {PI_IP}:{BACNET_PORT}")
        sock.sendto(whois_packet, (PI_IP, BACNET_PORT))
        
        try:
            data, addr = sock.recvfrom(1024)
            hex_data = ' '.join(f'{b:02x}' for b in data)
            print(f"Direct response from {addr[0]}: {hex_data}")
        except socket.timeout:
            print("✗ No direct response received")
            
        sock.close()
        
    except Exception as e:
        print(f"Direct test error: {e}")

def check_bacnet4linux_process():
    """Check if BACnet4Linux is actually running"""
    print("\nChecking BACnet4Linux process...")
    
    try:
        result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
        bacnet_processes = [line for line in result.stdout.split('\n') if 'bacnet4linux' in line]
        
        if bacnet_processes:
            print("✓ BACnet4Linux processes found:")
            for proc in bacnet_processes:
                print(f"  {proc}")
        else:
            print("✗ No BACnet4Linux processes found")
            
    except Exception as e:
        print(f"Process check error: {e}")

def main():
    print("BACnet Network Diagnostic Test")
    print("=" * 50)
    
    check_bacnet4linux_process()
    test_port_binding()
    send_whois()
    test_direct_connection()
    
    print("\nDiagnostic complete.")
    print("\nIf BACnet4Linux is running but not responding:")
    print("1. Check if it's binding to the correct interface")
    print("2. Verify network interface name (eth0, wlan0, etc.)")
    print("3. Ensure UDP port 47808 is not blocked by firewall")
    print("4. Try restarting with different network parameters")

if __name__ == "__main__":
    main()