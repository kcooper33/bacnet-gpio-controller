#!/bin/bash
echo "BACnet GPIO Controller - Deployment Verification"
echo "================================================"

# Check if we're in the right directory
if [ ! -f "bacnet_config.json" ]; then
    echo "Error: Not in pi_deployment directory"
    echo "Run: cd bacnet-gpio-controller/pi_deployment/"
    exit 1
fi

# Check BACnet4Linux directory
if [ ! -d "bacnet4linux" ]; then
    echo "Error: bacnet4linux directory missing"
    exit 1
fi

cd bacnet4linux

# Check Makefile
if [ ! -f "Makefile" ]; then
    echo "Error: Makefile missing"
    exit 1
fi

# Check for 'all' target in Makefile
if ! grep -q "^all:" Makefile; then
    echo "Error: Makefile missing 'all' target"
    exit 1
fi

# Check required source files
missing_files=""
required_files="main.c options.c net.c bacnet_text.c invoke_id.c bacdcode.c html.c return_properties.c signal_handler.c check_online_status.c query_new_device.c bacnet_object.c bacnet_device.c send_npdu.c send_read_property.c send_write_property.c send_subscribe_cov.c send_whois.c send_iam.c send_time_synch.c send_bip.c packet.c ethernet.c receive_apdu.c receive_readproperty.c receive_writeproperty.c receive_npdu.c receive_readpropertyACK.c receive_COV.c receive_iam.c receive_bip.c debug.c pdu.c reject.c keylist.c dstring.c dbuffer.c bigendian.c version.c gpio_objects.c"

for file in $required_files; do
    if [ ! -f "$file" ]; then
        missing_files="$missing_files $file"
    fi
done

if [ -z "$missing_files" ]; then
    echo "✓ All required source files present"
    echo "✓ Makefile has 'all' target"
    echo "✓ Ready for compilation"
    echo ""
    echo "To compile:"
    echo "make clean && make"
else
    echo "✗ Missing files: $missing_files"
    echo "Download missing files from repository"
fi
