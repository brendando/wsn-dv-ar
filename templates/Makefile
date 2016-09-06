CONTIKI_PROJECT=template
all: $(CONTIKI_PROJECT)
WORKING_DIR=$(CURDIR)

CONTIKI = $(WORKING_DIR)/../contiki
include $(CONTIKI)/Makefile.include


prog: 
	$(HOME)/Downloads/DSLite/DebugServer/bin/DSLite load -c ~/Downloads/DSLite/CC2650F128_TIXDS110_Connection.ccxml -f $(CONTIKI_PROJECT).elf
