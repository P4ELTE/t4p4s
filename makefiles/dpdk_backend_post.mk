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

VPATH += $(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/)
VPATH += $(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/includes)
VPATH += $(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/ctrl_plane)
VPATH += $(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/data_plane)

# dpdk main
SRCS-y += main.c

# control plane related sources
SRCS-y += ctrl_plane_backend.c
SRCS-y += fifo.c
SRCS-y += handlers.c
SRCS-y += messages.c
SRCS-y += sock_helpers.c
SRCS-y += threadpool.c

# data plane related includes
SRCS-y += dpdk_lib.c
SRCS-y += dpdk_tables.c
SRCS-y += ternary_naive.c

CFLAGS += -I "$(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/includes)"
CFLAGS += -I "$(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/ctrl_plane)"
CFLAGS += -I "$(realpath -sm $(CDIR)/../../src/hardware_dep/dpdk/data_plane)"

include $(RTE_SDK)/mk/rte.extapp.mk
