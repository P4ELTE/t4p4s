
ifeq ($(CC),)
CLANG_EXISTS := $(shell command -v clang 2> /dev/null)

#ifdef CLANG_EXISTS
CC="ccache clang"
#else
CC="ccache gcc"
#fi
endif

ifeq ($(LD),)
LLD_EXISTS := $(shell command -v ld.lld 2> /dev/null)
#ifdef LLD_EXISTS
LD=ld.lld
#else
LD=ld.bfd
#fi
endif

CFLAGS=-Wall -pthread -std=c99
LIB=

ifneq ($(P4_GCC_OPTS),)
CFLAGS += $(P4_GCC_OPTS)
endif


all: dpdk_dummy_controller dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_l3fwd_controller dpdk_smgw_controller

controllers_common: handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_ctrl_common.c dpdk_controller_dictionary.c
	@$(CC) $(CFLAGS) $(LIB) handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_ctrl_common.c dpdk_controller_dictionary.c -c
	@$(LD) -r handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o dpdk_ctrl_common.o dpdk_controller_dictionary.o -o dpdk_ctrl.o

dpdk_dummy_controller: controllers_common dpdk_dummy_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_dummy_controller.c -o dpdk_dummy_controller

dpdk_portfwd_controller: controllers_common dpdk_portfwd_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_portfwd_controller.c -o dpdk_portfwd_controller

dpdk_l2fwd_controller: controllers_common dpdk_l2fwd_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l2fwd_controller.c -o dpdk_l2fwd_controller

dpdk_psa_l2fwd_controller: controllers_common dpdk_psa_l2fwd_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_psa_l2fwd_controller.c -o dpdk_psa_l2fwd_controller

dpdk_test-digest_controller: controllers_common dpdk_test-digest_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_test-digest_controller.c -o dpdk_test-digest_controller

dpdk_l3fwd_controller: controllers_common dpdk_l3fwd_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l3fwd_controller.c -o dpdk_l3fwd_controller

dpdk_l3-routing-full_controller: controllers_common dpdk_l3-routing-full_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l3-routing-full_controller.c -o dpdk_l3-routing-full_controller

dpdk_smgw_controller: controllers_common dpdk_smgw_controller.c
	@$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_smgw_controller.c -o dpdk_smgw_controller

clean:
	rm -f handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o \
	      dpdk_ctrl_common.o dpdk_ctrl_common.o dpdk_ctrl.o dpdk_controller_dictionary.o \
	      dpdk_dummy_controller dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_test-digest_controller dpdk_l3fwd_controller dpdk_l3-routing-full_controller dpdk_smgw_controller
