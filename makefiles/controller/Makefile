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

COMMA = ,

CFLAGS = -O2 -std=c99 -Wall -pthread
LDLIBS = -lpthread

CTRL_PLANE_DIR := $(abspath ../../src/hardware_dep/shared/ctrl_plane)

CFLAGS += -iquote "$(CTRL_PLANE_DIR)"
VPATH := $(CTRL_PLANE_DIR)

SRCS-y += handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c
OBJS-y := $(strip $(patsubst %.c,%.o,$(SRCS-y)))

%.o: %.c
	@echo "  CC $(notdir $<)"
	@$(CC) $(CFLAGS) -c -o $@ $<
	
l2_controller: $(OBJS-y) l2_controller.o
	@echo "  CC $@"
	@$(CC) -o $@ $^ $(addprefix -Wl$(COMMA),$(LDLIBS))

l3_controller: $(OBJS-y) l3_controller.o
	@echo "  CC $@"
	@$(CC) -o $@ $^ $(addprefix -Wl$(COMMA),$(LDLIBS))

l3-full_controller: $(OBJS-y) l3-full_controller.o
	@echo "  CC $@"
	@$(CC) -o $@ $^ $(addprefix -Wl$(COMMA),$(LDLIBS))

l2_l3_controller: $(OBJS-y) l2_l3_controller.o
	@echo "  CC $@"
	@$(CC) -o $@ $^ $(addprefix -Wl$(COMMA),$(LDLIBS))

.PHONY: all
all: l2_controller l3_controller l3-full_controller l2_l3_controller
	

.PHONY: clean
clean:
	@rm -f *.o *_controller

