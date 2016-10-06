CONTIKI_PROJECT=rpi-bridge
all: $(CONTIKI_PROJECT)
WORKING_DIR=$(CURDIR)

CONTIKI = $(WORKING_DIR)/../../contiki
include $(CONTIKI)/Makefile.include

CFLAGS += -DRF_CORE_CONF_CHANNEL=11

prog: 
	$(HOME)/Downloads/DSLite/DebugServer/bin/DSLite load -c ~/Downloads/DSLite/CC2650F128_TIXDS110_Connection.ccxml -f $(CONTIKI_PROJECT).elf
