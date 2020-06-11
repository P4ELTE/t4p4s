CC=gcc
CFLAGS=-Wall -pthread -std=c99
LIB=

ifneq ($(P4_GCC_OPTS),)
CFLAGS += $(P4_GCC_OPTS)
endif


all: dpdk_dummy_controller dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_l3fwd_controller dpdk_smgw_controller

controllers_common: handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c
	$(CC) $(CFLAGS) $(LIB) handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_ctrl_common.c -c
	ld -r handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o dpdk_ctrl_common.o -o dpdk_ctrl.o

dpdk_dummy_controller: controllers_common dpdk_dummy_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_dummy_controller.c -o dpdk_dummy_controller

dpdk_portfwd_controller: controllers_common dpdk_portfwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_portfwd_controller.c -o dpdk_portfwd_controller

dpdk_l2fwd_controller: controllers_common dpdk_l2fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l2fwd_controller.c -o dpdk_l2fwd_controller

dpdk_psa_l2fwd_controller: controllers_common dpdk_psa_l2fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_psa_l2fwd_controller.c -o dpdk_psa_l2fwd_controller

dpdk_test-digest_controller: controllers_common dpdk_test-digest_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_test-digest_controller.c -o dpdk_test-digest_controller

dpdk_l3fwd_controller: controllers_common dpdk_l3fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l3fwd_controller.c -o dpdk_l3fwd_controller

dpdk_smgw_controller: controllers_common dpdk_smgw_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_smgw_controller.c -o dpdk_smgw_controller

clean:
	rm -f handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o dpdk_ctrl_common.o dpdk_ctrl_common.o dpdk_ctrl.o dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_l3fwd_controller dpdk_smgw_controller dpdk_dummy_controller

