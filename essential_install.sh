#!/bin/bash
# BACnet Pi Controller Installation Script
# Compiles BACnet4Linux for ARM and starts the controller with configuration

set -e  # Exit on any error

CONFIG_FILE="bacnet_config.json"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACNET_DIR="$SCRIPT_DIR/bacnet4linux"
CONTROLLER_SCRIPT="pi_bacnet4linux_fixed.py"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1"
}

check_requirements() {
    log "Checking system requirements..."
    
    # Check if we're on ARM architecture
    ARCH=$(uname -m)
    if [[ "$ARCH" != "aarch64" && "$ARCH" != "armv7l" && "$ARCH" != "armv6l" ]]; then
        warning "Not running on ARM architecture ($ARCH). Controller may not work properly."
    else
        success "Running on ARM architecture: $ARCH"
    fi
    
    # Check for required tools
    for tool in gcc make python3 jq; do
        if ! command -v $tool &> /dev/null; then
            error "$tool is not installed"
            echo "Run: sudo apt install build-essential python3 jq"
            exit 1
        fi
    done
    success "Required tools are installed"
    
    # Check for Python packages
    python3 -c "import json, subprocess, socket, threading, time" 2>/dev/null || {
        error "Required Python modules missing"
        echo "Run: sudo apt install python3-flask python3-gpiozero"
        exit 1
    }
    success "Python dependencies available"
}

read_config() {
    log "Reading configuration from $CONFIG_FILE..."
    
    if [[ ! -f "$CONFIG_FILE" ]]; then
        error "Configuration file $CONFIG_FILE not found"
        exit 1
    fi
    
    # Extract values using jq
    DEVICE_ID=$(jq -r '.device.id' "$CONFIG_FILE")
    DEVICE_NAME=$(jq -r '.device.name' "$CONFIG_FILE")
    VENDOR_ID=$(jq -r '.device.vendor_id' "$CONFIG_FILE")
    NETWORK_INTERFACE=$(jq -r '.network.interface' "$CONFIG_FILE")
    BACNET_PORT=$(jq -r '.network.port' "$CONFIG_FILE")
    ETHERNET_ENABLE=$(jq -r '.bacnet_options.ethernet_enable' "$CONFIG_FILE")
    APDU_TIMEOUT=$(jq -r '.bacnet_options.apdu_timeout' "$CONFIG_FILE")
    DEBUG_LEVEL=$(jq -r '.bacnet_options.debug_level' "$CONFIG_FILE")
    INITIAL_QUERY=$(jq -r '.bacnet_options.initial_query' "$CONFIG_FILE")
    HTTP_SERVER=$(jq -r '.bacnet_options.http_server' "$CONFIG_FILE")
    
    success "Configuration loaded:"
    echo "  Device ID: $DEVICE_ID"
    echo "  Device Name: $DEVICE_NAME"
    echo "  Network Interface: $NETWORK_INTERFACE"
    echo "  BACnet Port: $BACNET_PORT"
}

detect_network_interface() {
    log "Auto-detecting network interface..."
    
    # Get the interface with the default route
    DEFAULT_INTERFACE=$(ip route | grep default | awk '{print $5}' | head -n1)
    
    if [[ -n "$DEFAULT_INTERFACE" ]]; then
        NETWORK_INTERFACE="$DEFAULT_INTERFACE"
        success "Detected active interface: $NETWORK_INTERFACE"
        
        # Update config file with detected interface
        jq ".network.interface = \"$NETWORK_INTERFACE\"" "$CONFIG_FILE" > "${CONFIG_FILE}.tmp" && mv "${CONFIG_FILE}.tmp" "$CONFIG_FILE"
    else
        warning "Could not detect network interface, using configured: $NETWORK_INTERFACE"
    fi
}

compile_bacnet4linux() {
    log "Compiling BACnet4Linux for ARM..."
    
    cd "$BACNET_DIR"
    
    # Clean previous build
    make clean >/dev/null 2>&1 || true
    rm -f *.o bacnet4linux
    
    # Set compilation flags for ARM
    export CC=gcc
    export CFLAGS="-march=native -O2 -Wall -I. -g"
    
    # Compile
    log "Starting compilation..."
    if make all; then
        success "Compilation successful"
    else
        error "Compilation failed"
        exit 1
    fi
    
    # Verify the binary
    if [[ ! -f "bacnet4linux" ]]; then
        error "Binary not created"
        exit 1
    fi
    
    # Check architecture
    BINARY_INFO=$(file bacnet4linux)
    echo "Binary info: $BINARY_INFO"
    
    if [[ "$BINARY_INFO" == *"ARM"* ]] || [[ "$BINARY_INFO" == *"aarch64"* ]]; then
        success "Binary compiled for ARM architecture"
    else
        warning "Binary may not be ARM architecture: $BINARY_INFO"
    fi
    
    chmod +x bacnet4linux
    cd "$SCRIPT_DIR"
}

test_bacnet4linux() {
    log "Testing BACnet4Linux startup..."
    
    cd "$BACNET_DIR"
    
    # Build command line arguments
    ETHERNET_FLAG=$([ "$ETHERNET_ENABLE" = "true" ] && echo "1" || echo "0")
    QUERY_FLAG=$([ "$INITIAL_QUERY" = "true" ] && echo "1" || echo "0")
    HTTP_FLAG=$([ "$HTTP_SERVER" = "true" ] && echo "1" || echo "0")
    
    CMD="./bacnet4linux -d${DEVICE_ID} -v${VENDOR_ID} -p${BACNET_PORT} -i${NETWORK_INTERFACE} -e${ETHERNET_FLAG} -t${APDU_TIMEOUT} -D${DEBUG_LEVEL} -q${QUERY_FLAG} -h${HTTP_FLAG}"
    
    echo "Test command: $CMD"
    
    # Test for 3 seconds
    timeout 3s $CMD >/dev/null 2>&1 && TEST_RESULT=0 || TEST_RESULT=$?
    
    if [[ $TEST_RESULT -eq 124 ]]; then
        success "BACnet4Linux started successfully (timed out as expected)"
    elif [[ $TEST_RESULT -eq 0 ]]; then
        success "BACnet4Linux test completed"
    else
        error "BACnet4Linux failed to start (exit code: $TEST_RESULT)"
        echo "Try running manually: $CMD"
        exit 1
    fi
    
    cd "$SCRIPT_DIR"
}

start_controller() {
    log "Starting BACnet Controller..."
    
    # Export config for Python script
    export BACNET_CONFIG_FILE="$SCRIPT_DIR/$CONFIG_FILE"
    
    echo ""
    echo "======================================================================="
    echo "BACnet Pi Controller Starting"
    echo "Device ID: $DEVICE_ID ($DEVICE_NAME)"
    echo "Network: $NETWORK_INTERFACE:$BACNET_PORT"
    echo "Configuration: $CONFIG_FILE"
    echo "======================================================================="
    echo ""
    
    # Start the Python controller
    python3 "$CONTROLLER_SCRIPT"
}

# Main execution
main() {
    echo ""
    echo "BACnet Pi Controller Installation & Startup"
    echo "==========================================="
    echo ""
    
    check_requirements
    read_config
    detect_network_interface
    compile_bacnet4linux
    test_bacnet4linux
    start_controller
}

# Handle interruption
trap 'echo -e "\n${YELLOW}Installation interrupted${NC}"; exit 130' INT TERM

# Check if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi