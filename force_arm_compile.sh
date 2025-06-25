#!/bin/bash
# Force ARM compilation of BACnet4Linux on Raspberry Pi

echo "Forcing clean ARM compilation of BACnet4Linux..."

cd bacnet4linux

# Remove all compiled files
echo "Cleaning all object files and binaries..."
make clean
rm -f *.o bacnet4linux

# Verify we're on ARM
echo "Architecture: $(uname -m)"

# Set ARM compilation flags
export CC=gcc
export CFLAGS="-march=native -O2"

echo "Starting compilation..."
make all

if [ -f "bacnet4linux" ]; then
    echo "Compilation successful!"
    echo "Binary info:"
    file bacnet4linux
    
    echo "Testing startup..."
    chmod +x bacnet4linux
    timeout 5s ./bacnet4linux -d25411 -v999 -p47808 -iwlan0 -e0 -t10 -D2 -q1 -h0 || echo "Test completed"
else
    echo "Compilation failed!"
    echo "Check for missing dependencies:"
    echo "sudo apt install build-essential libc6-dev"
fi