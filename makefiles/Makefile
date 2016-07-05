
ifeq ($(RTE_SDK),)
$(error "Please define the RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk
include $(SRCDIR)/common.mk

# binary name
APP = example_dpdk1

include $(SRCDIR)/dpdk_backend.mk
include $(SRCDIR)/hw_independent.mk

include $(RTE_SDK)/mk/rte.extapp.mk
