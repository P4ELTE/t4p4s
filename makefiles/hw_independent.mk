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
CDIR := $(dir $(lastword $(MAKEFILE_LIST)))
P4_SRCDIR := $(dir $(lastword $(MAKEFILE_LIST)))/../../src

# the directories of the source files
VPATH += $(CDIR)/src_hardware_indep

VPATH += $(P4_SRCDIR)/hardware_dep/shared/
VPATH += $(P4_SRCDIR)/hardware_dep/shared/includes
VPATH += $(P4_SRCDIR)/hardware_dep/shared/ctrl_plane
VPATH += $(P4_SRCDIR)/hardware_dep/shared/data_plane

# the names of the source files
SRCS-y += dataplane.c
SRCS-y += tables.c
SRCS-y += parser.c
SRCS-y += actions.c
SRCS-y += controlplane.c

CFLAGS += -I "$(P4_SRCDIR)/hardware_dep/shared/includes"
CFLAGS += -I "$(P4_SRCDIR)/hardware_dep/shared/ctrl_plane"
CFLAGS += -I "$(P4_SRCDIR)/hardware_dep/shared/data_plane"

CFLAGS += -I "$(CDIR)/src_hardware_indep"
