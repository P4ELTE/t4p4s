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

# the directories of the source files
VPATH += $(P4_INDEP_SRCDIR)/src_hardware_indep

# the names of the source files
SRCS-y += dataplane.c
SRCS-y += tables.c
SRCS-y += parser.c
SRCS-y += actions.c
SRCS-y += controlplane.c

CFLAGS += -I "$(P4_INDEP_SRCDIR)/src_hardware_indep"

