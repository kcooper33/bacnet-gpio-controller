#Makefile to build BACnet4Linux
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
# -O2 optimize for better performance
 CFLAGS  = -Wall -I. -g -O2

TARGET = bacnet4linux

# Source files
SOURCES = main.c options.c net.c bacnet_text.c invoke_id.c bacdcode.c html.c \
          return_properties.c signal_handler.c check_online_status.c \
          query_new_device.c bacnet_object.c bacnet_device.c send_npdu.c \
          send_read_property.c send_write_property.c send_subscribe_cov.c \
          send_whois.c send_iam.c send_time_synch.c send_bip.c packet.c \
          ethernet.c receive_apdu.c receive_readproperty.c receive_writeproperty.c \
          receive_npdu.c receive_readpropertyACK.c receive_COV.c receive_iam.c \
          receive_bip.c debug.c pdu.c reject.c keylist.c dstring.c \
          dbuffer.c bigendian.c version.c gpio_objects.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Build targets
all: $(TARGET)

$(TARGET): $(OBJECTS)
        $(CC) -o $@ $(OBJECTS)

%.o: %.c
        $(CC) $(CFLAGS) -c $< -o $@

clean:
        rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
        cp $(TARGET) /usr/local/bin/

.PHONY: all clean install

SRCS = main.c options.c net.c bacnet_text.c invoke_id.c bacdcode.c \
            html.c return_properties.c signal_handler.c check_online_status.c \
            query_new_device.c bacnet_object.c bacnet_device.c \
            send_npdu.c send_read_property.c send_write_property.c \
            send_subscribe_cov.c send_whois.c send_iam.c \
            send_time_synch.c send_bip.c packet.c ethernet.c \
            receive_apdu.c receive_readproperty.c receive_writeproperty.c \
            receive_npdu.c receive_readpropertyACK.c receive_COV.c \
            receive_iam.c receive_bip.c debug.c pdu.c reject.c \
            keylist.c dstring.c dbuffer.c bigendian.c \
            version.c gpio_objects.c

OBJS = ${SRCS:.c=.o}

all: ${TARGET}

${TARGET}: ${OBJS}
        ${CC} -o $@ ${OBJS} 

.c.o:
        ${CC} -c ${CFLAGS} $*.c
        
depend:
        rm -f .depend
        ${CC} -MM ${CFLAGS} *.c >> .depend
        
clean:
        rm -rf core ${OBJS} ${TARGET} *.bak

include: .depend
