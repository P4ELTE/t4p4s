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

include $(MK_DIR)/common.mk

COMMA = ,

CFLAGS += -march=native

CFLAGS += -D P4_LINUX_TARGET
CFLAGS += -D _GNU_SOURCE
CFLAGS += -D ETHDEV_RX_NOCOPY
CFLAGS += -D NDEBUG

LDLIBS += -lpthread

# linux main
SRCS-y += main.c
SRCS-y += main_loop.c

# data plane related sources
SRCS-y += linux_backend.c
SRCS-y += linux_tables.c
SRCS-y += linux_primitives.c
SRCS-y += crc32.c
SRCS-y += pktbuf.c
SRCS-y += ethdev.c
SRCS-y += hash_table.c
SRCS-y += bitwise_trie.c
SRCS-y += ternary_naive.c
SRCS-y += vector.c

OUTPUT_DIR := $(MK_DIR)/linux
$(shell mkdir -p $(OUTPUT_DIR))

OBJS-y := $(strip $(patsubst %.c,%.o,$(SRCS-y)))

$(OUTPUT_DIR)/%.o: %.c
	@echo "  CC $(notdir $<)"
	@$(CC) $(CFLAGS) -c -o $@ $< 
    
$(OUTPUT_DIR)/$(APP): $(addprefix $(OUTPUT_DIR)/,$(OBJS-y))
	@echo "  CC $(APP)"
	@$(CC) -o $@ $^ $(addprefix -Wl$(COMMA),$(LDLIBS))

.PHONY: all
all: $(OUTPUT_DIR)/$(APP)

.PHONY: clean
clean:
	@rm -rf $(OUTPUT_DIR)

