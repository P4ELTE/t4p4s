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

from utils.codegen import format_declaration_16, format_statement_16_ctl, format_expr_16, format_type_16, type_env
from utils.misc import addError, addWarning

#[ #include <stdlib.h>
#[ #include <string.h>
#[ #include <stdbool.h>
#[ #include "dpdk_lib.h"
#[ #include "data_plane_data.h"
#[ #include "actions.h"
#[ #include "util.h"

#[ uint8_t* emit_addr;
#[ uint32_t ingress_pkt_len;

#[ extern void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables);
#[ extern void increase_counter (int counterid, int index);

# note: 0 is for the special case where there are no tables
max_key_length = max([t.key_length_bytes for t in hlir16.tables if hasattr(t, 'key')] + [0])
#[ uint8_t reverse_buffer[${max_key_length}];

################################################################################

STDPARAMS = "packet_descriptor_t* pd, lookup_table_t** tables"
STDPARAMS_IN = "pd, tables"

main = hlir16.declarations['Declaration_Instance'][0] # TODO what if there are more packet instances?
packet_name = main.type.baseType.type_ref.name
pipeline_elements = main.arguments

#[ struct apply_result_s {
#[     bool hit;
#[     enum actions action_run;
#[ };

for pe in pipeline_elements:
    c = hlir16.declarations.get(pe.type.name, 'P4Control')
    if c is None:
        continue

    #[ void control_${pe.type.name}(${STDPARAMS});
    for t in c.controlLocals['P4Table']:
        #[ struct apply_result_s ${t.name}_apply(${STDPARAMS});

################################################################################

# TODO move this to HAL
def match_type_order_16(t):
    if t == 'EXACT':   return 0
    if t == 'LPM':     return 1
    if t == 'TERNARY': return 2
    else:              return 3

################################################################################
# Table key calculation

for table in hlir16.tables:
    if not hasattr(table, 'key'):
        continue

    #{ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key) {
    sortedfields = sorted(table.key.keyElements, key=lambda k: match_type_order_16(k.match_type))
    #TODO variable length fields
    #TODO field masks
    for f in sortedfields:
        if f.get_attr('width') is None:
            # TODO find out why this is missing and fix it
            continue
        if f.width <= 32:
            #[ EXTRACT_INT32_BITS_PACKET(pd, header_instance_${f.header.name}, field_${f.header.type.type_ref.name}_${f.field_name}, *(uint32_t*)key)
            #[ key += sizeof(uint32_t);
        elif f.width > 32 and f.width % 8 == 0:
            byte_width = (f.width+7)/8
            #[ EXTRACT_BYTEBUF_PACKET(pd, header_instance_${f.header.name}, field_${f.header.type.type_ref.name}_${f.field_name}, key)
            #[ key += ${byte_width};
        else:
            add_error("table key calculation", "Unsupported field %s ignored." % f.id)

    if table.match_type == "LPM":
        #[ key -= ${table.key_length_bytes};
        #[ int c, d;
        #[ for(c = ${table.key_length_bytes-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${table.key_length_bytes}; c++) *(key+c) = *(reverse_buffer+c);
    #} }

################################################################################
# Table application

# for pe in pipeline_elements:
#     c = hlir16.declarations.get(pe.type.name, 'P4Control')
#     if c is None:
#         continue

#     for table in c.controlLocals['P4Table']:

for pe in pipeline_elements:
    c = hlir16.declarations.get(pe.type.name, 'P4Control')
    if c is None:
        continue

for table in hlir16.tables:
    lookupfun = {'LPM':'lpm_lookup', 'EXACT':'exact_lookup', 'TERNARY':'ternary_lookup'}
    #[ struct apply_result_s ${table.name}_apply(${STDPARAMS})
    #[ {
    #[     debug("  :::: EXECUTING TABLE ${table.name}\n");
    if hasattr(table, 'key'):
        #[     uint8_t* key[${table.key_length_bytes}];
        #[     table_${table.name}_key(pd, (uint8_t*)key);
        #[     uint8_t* value = ${lookupfun[table.match_type]}(tables[TABLE_${table.name}], (uint8_t*)key);
        #[     struct ${table.name}_action* res = (struct ${table.name}_action*)value;
        #[     bool hit = res != NULL && -42 != (*(int*)(value+sizeof(struct ${table.name}_action)));
    else:
        if hasattr(table, 'default_action'):
            #[    struct ${table.name}_action resStruct = { action_${table.default_action.expression.method.ref.name} };
            #[    struct ${table.name}_action* res = &resStruct;
            #[    bool hit = true;
        else:
            #[    struct ${table.name}_action* res = (struct ${table.name}_action*)0;
            #[    bool hit = false;

    # COUNTERS
    # TODO

    # ACTIONS
    #[     if(res == NULL) {
    #[       debug("    :: NO RESULT, NO DEFAULT ACTION.\n");
    #[     } else {
    #[       switch (res->action_id) {
    for action in table.actions:
        action_name = action.action_object.name
        if action_name == 'NoAction':
            continue
        #[         case action_${action_name}:
        #[           debug("    :: EXECUTING ACTION ${action_name}...\n");
        if action.action_object.parameters.parameters: # action.expression.arguments != []:
            #[           action_code_${action_name}(pd, tables, res->${action_name}_params);
        else:
            #[           action_code_${action_name}(pd, tables);
        #[           break;
    #[       }
    #[     }

    #[     struct apply_result_s apply_result = { hit, res != NULL ? res->action_id : -1 };
    #[     return apply_result;
    #[ }
    #[
    #[ struct ${table.name}_s {
    #[     struct apply_result_s (*apply)(packet_descriptor_t* pd, lookup_table_t** tables);
    #[ };
    #[ struct ${table.name}_s ${table.name} = {.apply = &${table.name}_apply};


################################################################################

#[ void reset_headers(${STDPARAMS}) {
for h in hlir16.header_instances:
    if not h.type.type_ref.is_metadata:
        #[ pd->headers[${h.id}].pointer = NULL;
    else:
        #[ memset(pd->headers[${h.id}].pointer, 0, header_info(${h.id}).bytewidth * sizeof(uint8_t));
#[ }

#[ void init_headers(${STDPARAMS}) {
for h in hlir16.header_instances:
    if not h.type.type_ref.is_metadata:
        #[ pd->headers[${h.id}] = (header_descriptor_t)
        #[ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = NULL,
        #[     .var_width_field_bitwidth = 0,
        #[     .name = "${h.name}",
        #[ };
    else:
        #[ pd->headers[${h.id}] = (header_descriptor_t)
        #[ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = malloc(header_info(${h.id}).bytewidth * sizeof(uint8_t)),
        #[     .var_width_field_bitwidth = 0
        #[ };
#[ }

################################################################################

#TODO are these keyless tabls supported in p4-16?

def keyless_single_action_table(table):
    if not table.get_attr('key'):
        return True
    return table.key_length_bytes == 0 and len(table.actions) == 2 and table.actions[1].action_object.name.startswith('NoAction')

for table in hlir16.tables:
    if keyless_single_action_table(table):
        #[ extern void ${table.name}_setdefault(struct ${table.name}_action);

#[ void init_keyless_tables() {
for table in hlir16.tables:
    if keyless_single_action_table(table):
        action = table.actions[0].action_object
        #[ struct ${table.name}_action ${table.name}_a;
        #[ ${table.name}_a.action_id = action_${action.name};
        #[ ${table.name}_setdefault(${table.name}_a);
#[ }

################################################################################

#[ void init_dataplane(${STDPARAMS}) {
#[     init_headers(${STDPARAMS_IN});
#[     reset_headers(${STDPARAMS_IN});
#[     init_keyless_tables();
#[     pd->dropped=0;
#[ }

#{ void update_packet(packet_descriptor_t* pd) {
#[     uint32_t value32, res32;
#[     (void)value32, (void)res32;
for hdr in hlir16.header_instances:
    #[ 
    #[ // updating header instance ${hdr.name}

    for fld in hdr.type.type_ref.fields:
        if not fld.preparsed and fld.type.size <= 32:
            #{ if(pd->fields.attr_field_instance_${hdr.name}_${fld.name} == MODIFIED) {
            #[     value32 = pd->fields.field_instance_${hdr.name}_${fld.name};
            #[     MODIFY_INT32_INT32_AUTO_PACKET(pd, header_instance_${hdr.name}, field_${hdr.type.type_ref.name}_${fld.name}, value32)
            #} }
#} }

################################################################################
# Pipeline

class types:
    def __init__(self, new_type_env):
        global type_env
        self.env_vars = set()
        for v in new_type_env:
            if v in type_env:
                addWarning('adding a type environment', 'variable {} is already bound to type {}'.format(v, type_env[v]))
            else:
                self.env_vars.add(v)
                type_env[v] = new_type_env[v]

    def __enter__(self):
        global type_env
        return type_env

    def __exit__(self, type, value, traceback):
        global type_env
        for v in self.env_vars:
            del type_env[v]

# TODO this is a temporary quick fix for "calculated_field"s
for m in hlir16.declarations['Method']:
    # TODO Hacking the hack to support offload annotation
    if m.name in ['verify_checksum', 'update_checksum', 'verify_checksum_offload', 'update_checksum_offload', 'mark_to_drop']:#These are already implemented in the DPDK HAL
        continue
    # TODO temporary fix for l3-routing-full, this will be computed later on
    with types({
        "T": "struct uint8_buffer_t",
        "O": "int",
        "HashAlgorithm": "int",
    }):
        t = m.type
        ret_type = format_type_16(t.returnType)
        args = ", ".join([format_expr_16(arg) for arg in t.parameters.parameters] + [STDPARAMS])

    #[ ${ret_type} ${m.name}(${args}) {
    #[     // TODO proper body
    #[ }

for pe in pipeline_elements:
    ctl = hlir16.declarations.get(pe.type.name, 'P4Control')
        
    if ctl is not None:
        #[ void control_${pe.type.name}(${STDPARAMS})
        #{ {
        #[     debug("entering control ${ctl.name}...\n");
        #[     uint32_t value32, res32;
        #[     (void)value32, (void)res32;
        #[     control_locals_${pe.type.name}_t control_locals_struct;
        #[     control_locals_${pe.type.name}_t* control_locals = &control_locals_struct;
        #[     pd->control_locals = (void*) control_locals;
        #[ ${format_statement_16_ctl(ctl.body, ctl)}
        #} }

#[ void process_packet(${STDPARAMS})
#{ {
for pe in pipeline_elements:
    ctl = hlir16.declarations.get(pe.type.name, 'P4Control')
    if ctl is not None:
        #[ control_${pe.type.name}(${STDPARAMS_IN});
        if pe.type.name == 'egress':
            #[ update_packet(pd); // we need to update the packet prior to calculating the new checksum
#} }

################################################################################

metadata_names = {hi.name for hi in hlir16.header_instances if hi.type.type_ref.is_metadata}
longest_hdr_name_len = max({len(h.name) for h in hlir16.header_instances if h.name not in metadata_names})

pkt_name_indent = " " * longest_hdr_name_len

#[ void store_headers_for_emit(${STDPARAMS})
#{ {
#[     debug("   :: Preparing %d header instances for storage...\n", pd->emit_hdrinst_count);

#[     uint8_t* storage = pd->header_tmp_storage;
#[     pd->emit_headers_length = 0;
#{     for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];

#{         if (hdr.pointer == NULL) {
#[             debug("    : Skipping header   (%${longest_hdr_name_len}s)  : (invalid header)\n", hdr.name);
#[             continue;
#}         }

#[         dbg_bytes(hdr.pointer, hdr.length, "    : Storing  %02d bytes (%${longest_hdr_name_len}s)  : ", hdr.length, hdr.name);

#[         memcpy(storage, hdr.pointer, hdr.length);
#[         storage += hdr.length;
#[         pd->emit_headers_length += hdr.length;
#}     }

#[     dbg_bytes(pd->header_tmp_storage, pd->emit_headers_length, "   :: Stored   %02d bytes     $pkt_name_indent: ", pd->emit_headers_length);
#} }

#[ void resize_packet_on_emit(${STDPARAMS})
#{ {
#{     if (unlikely(pd->emit_headers_length != pd->parsed_length)) {
#{         if (likely(pd->emit_headers_length > pd->parsed_length)) {
#[             int len_change = pd->emit_headers_length - pd->parsed_length;
#[             debug("   :: Adding   %02d bytes %${longest_hdr_name_len}s   : (header: from %d bytes to %d bytes)\n", len_change, "to packet", pd->parsed_length, pd->emit_headers_length);
#[             char* new_ptr = rte_pktmbuf_prepend(pd->wrapper, len_change);
#[             if (new_ptr == NULL) {
#[                 rte_exit(1, "Could not reserve necessary headroom (%d additional bytes)", len_change);
#[             }
#[             pd->data = (packet_data_t*)new_ptr;
#[         } else {
#[             int len_change = pd->parsed_length - pd->emit_headers_length;
#[             debug("   :: Removing %02d bytes %${longest_hdr_name_len}s  : (header: from %d bytes to %d bytes)\n", len_change, "from packet", pd->parsed_length, pd->emit_headers_length);
#[             char* new_ptr = rte_pktmbuf_adj(pd->wrapper, len_change);
#[             pd->data = (packet_data_t*)new_ptr;
#}         }
#[     } else {
#[         debug("   :: To emit  %02d bytes (no resize)\n", pd->emit_headers_length);
#}     }
#} }

#[ void copy_emit_contents(${STDPARAMS})
#{ {
#[     dbg_bytes(pd->header_tmp_storage, pd->emit_headers_length, "   :: Headers: %02d bytes %${longest_hdr_name_len}s : ", pd->emit_headers_length, "from storage");
#[     memcpy((struct rte_mbuf*)pd->data, pd->header_tmp_storage, pd->emit_headers_length);
#} }

#[ void emit_packet(${STDPARAMS})
#{ {
#[     if (unlikely(pd->is_emit_reordering)) {
#[         debug(" :::: Reordering emit\n");
#[         store_headers_for_emit(${STDPARAMS_IN});
#[         resize_packet_on_emit(${STDPARAMS_IN});
#[         copy_emit_contents(${STDPARAMS_IN});
#[     }
#} }

#[ void debug_print_parsed_packet(${STDPARAMS})
#{ {
#[ #ifdef P4DPDK_DEBUG
#[     debug("Parsed packet\n");
#[     for (int i = 0; i < HEADER_INSTANCE_COUNT; ++i) {
#[         if (!header_instance_is_metadata[i] && pd->headers[i].pointer != NULL) {
#[             dbg_bytes(pd->headers[i].pointer, pd->headers[i].length, " :::: Header %${longest_hdr_name_len}s (%02d bytes)    : ",  pd->headers[i].name, pd->headers[i].length);
#[         }
#[     }

#[     dbg_bytes(pd->data + pd->parsed_length, pd->payload_length, " :::: Payload  %02d bytes  %${longest_hdr_name_len}s   : ", pd->payload_length, " ");
#[ #endif
#} }

#[ void handle_packet(${STDPARAMS})
#{ {
#[     int value32;
#[     int res32;

#[     EXTRACT_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, value32)
#[     dbg_bytes(pd->data, rte_pktmbuf_pkt_len(pd->wrapper), "HANDLING PACKET (port %" PRIu32 ", %02d bytes)  : ", value32, rte_pktmbuf_pkt_len(pd->wrapper));

#[     reset_headers(${STDPARAMS_IN});
#[     pd->parsed_length = 0;
#[     parse_packet(${STDPARAMS_IN});
#[     pd->payload_length = rte_pktmbuf_pkt_len(pd->wrapper) - pd->parsed_length;
#[
#[     debug_print_parsed_packet(${STDPARAMS_IN});
#[
#[     emit_addr = pd->data;
#[     pd->emit_hdrinst_count = 0;
#[     pd->is_emit_reordering = false;
#[
#[     process_packet(${STDPARAMS_IN});
#[
#[     emit_packet(${STDPARAMS_IN});
#} }
