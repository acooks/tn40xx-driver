#
# tn40xx_ Driver
#
# Makefile for the tx40xx Linux driver

#######################################
# Build Configuration:
#######################################

PWD           := $(shell pwd)
KVERSION      := $(shell uname -r)
EXPECTED_KDIR := /lib/modules/$(KVERSION)/build
INSTDIR       := /lib/modules/$(KVERSION)/kernel/drivers/net
PM_DIR        := /etc/pm/config.d
RESUME_FILE   := ._RESUME_
MODULE_DIR    :=M=$(PWD)
DRV_NAME      := tn40xx
JUMO_OBJS     := tn40.o MV88X3120_phy.o MV88X3120_phy_Linux.o MV88X3310_phy.o MV88X3310_phy_Linux.o \
                 QT2025_phy.o QT2025_phy_Linux.o TLK10232_phy.o TLK10232_phy_Linux.o \
                 AQR105_phy.o AQR105_phy_Linux.o
JUMBO_PHYS    := -DPHY_MV88X3120 -DPHY_MV88X3310 -DPHY_MV88E2010 -DPHY_QT2025 -DPHY_TLK10232 -DPHY_AQR105
DRV_NAME      := tn40xx
DRB_OBJS      := tn40.o CX4.o CX4_Linux.o
RESUME ?= YES

ifeq ($(MV88X3120),YES)
	DRB_OBJS += MV88X3120_phy.o MV88X3120_phy_Linux.o
	OPT_PHYS += -DPHY_MV88X3120
endif

ifeq ($(MV88X3310),YES)
	DRB_OBJS += MV88X3310_phy.o MV88X3310_phy_Linux.o
	OPT_PHYS += -DPHY_MV88X3310
endif

ifeq ($(MV88E2010),YES)
	DRB_OBJS += MV88E2010_phy.o MV88E2010_phy_Linux.o
	OPT_PHYS += -DPHY_MV88E2010
endif

ifeq ($(QT),YES)
	DRB_OBJS += QT2025_phy.o QT2025_phy_Linux.o
	OPT_PHYS += -DPHY_QT2025
endif

ifeq ($(TL),YES)
	DRB_OBJS += TLK10232_phy.o TLK10232_phy_Linux.o
	OPT_PHYS += -DPHY_TLK10232
endif

ifeq ($(AQ),YES)
	DRB_OBJS += AQR105_phy.o AQR105_phy_Linux.o
	OPT_PHYS += -DPHY_AQR105
endif
#
# Resume
#
ifeq ($(RESUME),YES)
       # RESUME=YES
	OPT_RESUME += -D_DRIVER_RESUME_
endif
ifdef OPT_RESUME
	EXTRA_CFLAGS += $(OPT_RESUME)
	MAKE_MSG += resume supported
endif
ifeq ($(EEE), YES)
	EXTRA_CFLAGS += -D_EEE_
endif
#
# No selected PHYs default to Jumbo driver
#
ifndef OPT_PHYS
	DRB_OBJS += $(JUMO_OBJS)
	EXTRA_CFLAGS += $(JUMBO_PHYS)	
else
	EXTRA_CFLAGS += $(OPT_PHYS)	
endif
#
# Trace
#         
ifeq ($(TRACE),YES)
	DRB_OBJS += trace.o
	EXTRA_CFLAGS += -D_TRACE_LOG_
endif
#
# memLog
#         
ifeq ($(MEMLOG),YES)
	DRB_OBJS += memLog.o
	EXTRA_CFLAGS += -DTN40_MEMLOG
endif
obj-m += $(DRV_NAME).o
$(DRV_NAME)-objs := $(DRB_OBJS)
#
# Check existance of kernel build directory
#
KDIR=$(shell [ -e $(EXPECTED_KDIR) ] && echo $(EXPECTED_KDIR))
ifeq (,$(KDIR))
  $(error Aborting the build: Linux kernel $(EXPECTED_KDIR) source not found)
endif

.PHONY: help clean install uninstall

all: clean
	@echo Building kernel $(KVERSION) $(MAKE_MSG)
	$(MAKE) -C $(KDIR) $(MODULE_DIR) modules 
	@if [ -z $(OPT_RESUME) ]; then \
		rm -f $(RESUME_FILE) > /dev/null 2>&1; \
	else \
		touch  $(RESUME_FILE); \
	fi

clean: 
	$(MAKE) -C $(KDIR) $(MODULE_DIR) clean

help usage:
	@echo " usage:"
	@echo " make target [options]"
	@echo "  Available targets:" 
	@echo "    all            - build the driver" 
	@echo "    clean          - Clean the driver"
	@echo "    help           - Print this help message"
	@echo "    install        - Install driver to system directory"
	@echo "                     usually, it is /lib/modules/VER/kernel/drivers/net"
	@echo "    default        - all"
	@echo
	@echo "  Available options:" 
	@echo "    MV88X3120=YES  - include Marvel MV88X3120 phy"
	@echo "    MV88X3310=YES  - include Marvel MV88X3310 phy"
	@echo "    MV88E2010=YES  - include Marvel MV88E2010 phy"
	@echo "    QT=YES         - include QT2025 phy"
	@echo "    TL=YES         - include TI phy"
	@echo "    AQ=YES         - include Aquantia phy"
	@echo "    RESUME=NO      - do not support in driver system resume"
	@echo "    EEE=YES        - support EEE"
	@echo "    default        - phys = all, RESUME=YES, EEE=NO"

install: $(DRV_NAME).ko
	install -d $(INSTDIR)
	install -m 644 $(DRV_NAME).ko $(INSTDIR)
	depmod $(KVERSION)
	@if [ -f $(RESUME_FILE) ]; then \
		test -f $(PM_DIR)/$(DRV_NAME) && rm -f $(PM_DIR)/$(DRV_NAME) > /dev/null 2>&1 || true; \
	else \
		test ! -d $(PM_DIR) || echo 'SUSPEND_MODULES=$(DRV_NAME)' > $(PM_DIR)/$(DRV_NAME); \
	fi

uninstall:
	rm $(INSTDIR)/$(DRV_NAME).ko 
	test -f $(PM_DIR)/$(DRV_NAME) && rm $(PM_DIR)/$(DRV_NAME) || true
	depmod $(KVERSION)

#endif # else of ifneq ($(KERNELRELEASE), )
