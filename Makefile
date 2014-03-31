CONTIKI_PROJECT = sdf-server sdf-client
all: $(CONTIKI_PROJECT)

# include SDF libraries
PROJECTDIRS += ./sdf ./sdf/sensors
PROJECT_SOURCEFILES += battery.c circularbuffer.c consumptionrate.c drandom.c energymeter.c fpint.c gccbugs.c samplingrate.c solarpanel.c time.c udphelper.c

# include IPv6 stack with RPL routing
WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

# size optimizations
SMALL=1

CONTIKI = ../../contiki
include $(CONTIKI)/Makefile.include