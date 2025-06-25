#!/bin/bash
echo "BACnet GPIO Controller - Deployment Verification"
echo "================================================"

# Check if we're in the right directory
if [ ! -d "pi_deployment" ]; then
    echo "Error: Run this script from the bacnet-gpio-controller root directory"
    exit 1
fi

cd pi_deployment/bacnet4linux || {
    echo "Error: pi_deployment/bacnet4linux directory not found"
    exit 1
}

# Check for Makefile
if [ ! -f "Makefile" ]; then
    echo "Error: Makefile missing from pi_deployment/bacnet4linux/"
    exit 1
fi
echo "✓ Makefile found"

# Check critical source files
echo "Checking critical source files..."
critical_files="main.c options.c net.c bacnet_text.c invoke_id.c bacdcode.c html.c return_properties.c signal_handler.c check_online_status.c query_new_device.c bacnet_object.c bacnet_device.c send_npdu.c send_read_property.c send_write_property.c send_subscribe_cov.c send_whois.c send_iam.c send_time_synch.c send_bip.c packet.c ethernet.c receive_apdu.c receive_readproperty.c receive_writeproperty.c receive_npdu.c receive_readpropertyACK.c receive_COV.c receive_iam.c receive_bip.c debug.c pdu.c reject.c keylist.c dstring.c dbuffer.c bigendian.c version.c gpio_objects.c"

missing_files=""
present_count=0
for file in $critical_files; do
    if [ -f "$file" ]; then
        present_count=$((present_count + 1))
    else
        missing_files="$missing_files $file"
    fi
done

total_files=$(echo $critical_files | wc -w)
echo "Source files: $present_count/$total_files present"

if [ $present_count -ge 35 ]; then
    echo "✓ Sufficient source files for compilation"
    
    echo "Testing compilation..."
    make clean >/dev/null 2>&1
    if make >/dev/null 2>&1; then
        echo "✓ Compilation successful"
        if [ -f "bacnet4linux" ]; then
            echo "✓ BACnet4Linux binary created"
            ls -la bacnet4linux
            echo ""
            echo "DEPLOYMENT READY"
            echo "================"
            echo "Run: sudo ./install.sh"
        else
            echo "✗ Binary not found after compilation"
        fi
    else
        echo "✗ Compilation failed - checking errors:"
        make 2>&1 | head -10
    fi
else
    echo "✗ Insufficient source files for compilation"
    echo "Missing:$missing_files"
fi