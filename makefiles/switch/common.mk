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

CFLAGS += -O3
CFLAGS += -Wall
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-but-set-variable
CFLAGS += -Wno-unused-variable
CFLAGS += -g
CFLAGS += -std=gnu99
CFLAGS += -pthread

P4_GCC_OPTS := $(strip $(P4_GCC_OPTS))
ifneq ($(P4_GCC_OPTS),)
CFLAGS += $(P4_GCC_OPTS)
endif

CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/includes"
CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/ctrl_plane"
CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/data_plane"

CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/shared/includes"
CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/shared/ctrl_plane"
CFLAGS += -iquote "$(P4_SRCDIR)/hardware_dep/shared/data_plane"

VPATH += $(P4_SRCDIR)/hardware_dep/$(P4_TARGET)
VPATH += $(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/includes
VPATH += $(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/ctrl_plane
VPATH += $(P4_SRCDIR)/hardware_dep/$(P4_TARGET)/data_plane

VPATH += $(P4_SRCDIR)/hardware_dep/shared
VPATH += $(P4_SRCDIR)/hardware_dep/shared/includes
VPATH += $(P4_SRCDIR)/hardware_dep/shared/ctrl_plane
VPATH += $(P4_SRCDIR)/hardware_dep/shared/data_plane

# control plane related sources
SRCS-y += ctrl_plane_backend.c
SRCS-y += fifo.c
SRCS-y += handlers.c
SRCS-y += messages.c
SRCS-y += sock_helpers.c
SRCS-y += threadpool.c

include $(MK_DIR)/hw_independent.mk
