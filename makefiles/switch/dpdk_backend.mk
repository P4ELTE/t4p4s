# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifndef RTE_SDK
$(error "Please define the RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# override output dir set in rte.vars.mk
RTE_OUTPUT = $(RTE_SRCDIR)/dpdk

include $(MK_DIR)/common.mk

CFLAGS += -D P4_DPDK_TARGET

# dpdk main
SRCS-y += main.c
ifdef P4_DPDK_VARIANT
SRCS-y += main_loop_$(P4_DPDK_VARIANT).c
else
SRCS-y += main_loop.c
endif

# data plane related sources
SRCS-y += dpdk_lib.c
SRCS-y += dpdk_tables.c
SRCS-y += dpdk_primitives.c
SRCS-y += ternary_naive.c
SRCS-y += vector.c

include $(RTE_SDK)/mk/rte.extapp.mk
