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

MK_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

P4_TARGET ?= dpdk
P4_TARGET := $(strip $(P4_TARGET))
P4_INDEP_SRCDIR := $(MK_DIR)
P4_SRCDIR := $(abspath $(MK_DIR)/../../src)

APP = placeholder_name

ifeq ($(P4_TARGET),dpdk)
  include $(MK_DIR)/dpdk_backend.mk
else ifeq ($(P4_TARGET),linux)
  include $(MK_DIR)/linux_backend.mk
else
  $(error "Invalid target: $(P4_TARGET)")
endif
