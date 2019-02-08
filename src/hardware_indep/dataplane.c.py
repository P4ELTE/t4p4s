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

from utils.codegen import format_declaration, format_statement_ctl, format_expr, format_type, type_env, SHORT_STDPARAMS, SHORT_STDPARAMS_IN, STDPARAMS, STDPARAMS_IN
from utils.misc import addError, addWarning
from hlir16.hlir16_attrs import get_main

#[ #include <stdlib.h>
#[ #include <string.h>
#[ #include <stdbool.h>
#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include "backend.h"
#[ #include "util.h"
#[ #include "util_packet.h"
#[ #include "tables.h"

#[ uint8_t* emit_addr;
#[ uint32_t ingress_pkt_len;

#[ extern ctrl_plane_backend bg;
#[ extern char* action_names[];

#[ extern void parse_packet(STDPARAMS);
#[ extern void increase_counter (int counterid, int index);

# note: 0 is for the special case where there are no tables
max_key_length = max([t.key_length_bytes for t in hlir16.tables if hasattr(t, 'key')] + [0])
#[ uint8_t reverse_buffer[${max_key_length}];

################################################################################

main = get_main(hlir16)
packet_name = main.type.baseType.type_ref.name
pipeline_elements = main.arguments

#{ struct apply_result_s {
#[     bool hit;
#[     enum actions action_run;
#} };

for pe in pipeline_elements:
    c = hlir16.objects.get(pe.expression.type.name, 'P4Control')
    if c is None:
        continue

    #[ void control_${pe.expression.type.name}(STDPARAMS);
    for t in c.controlLocals['P4Table']:
        #[ struct apply_result_s ${t.name}_apply(STDPARAMS);

################################################################################

# TODO move this to HAL
def match_type_order(t):
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
    sortedfields = sorted(table.key.keyElements, key=lambda k: match_type_order(k.match_type))
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

for smem in table.meters + table.counters:
    for comp in smem.components:
        type = comp['type']
        name  = comp['name']
        #[ void apply_direct_smem_$type(rte_atomic32_t (*smem)[1], uint32_t value, char* table_name, char* smem_type_name, char* smem_name);


for table in hlir16.tables:
    lookupfun = {'LPM':'lpm_lookup', 'EXACT':'exact_lookup', 'TERNARY':'ternary_lookup'}
    #[ struct apply_result_s ${table.name}_apply(STDPARAMS)
    #{ {
    if hasattr(table, 'key'):
        #[     uint8_t* key[${table.key_length_bytes}];
        #[     table_${table.name}_key(pd, (uint8_t*)key);

        #[     dbg_bytes(key, table_config[TABLE_${table.name}].entry.key_size,
        #[               " :::: Lookup on table $$[table]{table.name} for %s",
        #[               ${table.key_length_bytes} == 0 ? "$$[bytes]{}{(empty key)}" : "");

        #[     table_entry_${table.name}_t* entry = (table_entry_${table.name}_t*)${lookupfun[table.match_type]}(tables[TABLE_${table.name}], (uint8_t*)key);
        #[     bool hit = entry != NULL && entry->is_entry_valid == INVALID_TABLE_ENTRY;

        #[     debug("   :: Lookup $$[success]{}{%s}: $${}{%s}%s\n",
        #[               hit ? "hit" : "miss",
        #[               entry == 0 ? "(no action)" : action_names[entry->action.action_id],
        #[               hit ? "" : " (default)");

        #{     if (likely(hit)) {
        for smem in table.meters + table.counters:
            for comp in smem.components:
                value = "pd->parsed_length" if comp['for'] == 'bytes' else "1"
                type = comp['type']
                name  = comp['name']
                #[ apply_direct_smem_$type(&(entry->state.$name), $value, "${table.name}", "${smem.smem_type}", "$name");
        #}    }
    else:
        action = table.default_action.expression.method.ref.name if hasattr(table, 'default_action') else None

        if action:
            #[    table_entry_${table.name}_t resStruct = {
            #[        .action = { action_${table.default_action.expression.method.ref.name} },
            #[    };
            #[    table_entry_${table.name}_t* entry = &resStruct;
            #[    bool hit = true;
            #[    bool is_default = false;
        else:
            #[    table_entry_${table.name}_t* entry = (struct ${table.name}_action*)0;
            #[    bool hit = false;
            #[    bool is_default = false;


    # ACTIONS
    #[     if (likely(entry != 0)) {
    #{       switch (entry->action.action_id) {
    for action in table.actions:
        action_name = action.action_object.name
        if action_name == 'NoAction':
            continue
        #{         case action_${action_name}:
        #[           debug("   :: Executing action $$[action]{action_name}%s...\n", hit ? "" : " (default)");
        #[           action_code_${action_name}(SHORT_STDPARAMS_IN, entry->action.${action_name}_params);
        #}           break;
    #[       }
    #[     } else {
    #[       debug("   :: NO RESULT, NO DEFAULT ACTION.\n");
    #}     }

    #[     struct apply_result_s apply_result = { hit, hit ? entry->action.action_id : -1 };
    #[     return apply_result;
    #} }


################################################################################

#{ void reset_headers(SHORT_STDPARAMS) {
for h in hlir16.header_instances:
    if not h.type.type_ref.is_metadata:
        #[ pd->headers[${h.id}].pointer = NULL;
    else:
        #[ memset(pd->headers[${h.id}].pointer, 0, header_info(${h.id}).bytewidth * sizeof(uint8_t));
#} }

#{ void init_headers(SHORT_STDPARAMS) {
for h in hlir16.header_instances:
    if not h.type.type_ref.is_metadata:
        #[ pd->headers[${h.id}] = (header_descriptor_t)
        #{ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = NULL,
        #[     .var_width_field_bitwidth = 0,
        #[     .name = "${h.name}",
        #} };
    else:
        #[ pd->headers[${h.id}] = (header_descriptor_t)
        #{ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = malloc(header_info(${h.id}).bytewidth * sizeof(uint8_t)),
        #[     .var_width_field_bitwidth = 0
        #} };
#} }

################################################################################

def is_keyless_single_action_table(table):
    return table.key_length_bytes == 0 and len(table.actions) == 2 and table.actions[1].action_object.name.startswith('NoAction')

for table in hlir16.tables:
    if is_keyless_single_action_table(table):
        #[ extern void ${table.name}_setdefault(struct ${table.name}_action);

#{ void init_keyless_tables() {
for table in hlir16.tables:
    if is_keyless_single_action_table(table):
        action = table.actions[0].action_object
        #[ struct ${table.name}_action ${table.name}_a;
        #[ ${table.name}_a.action_id = action_${action.name};
        #[ ${table.name}_setdefault(${table.name}_a);
#} }

################################################################################

#{ void init_dataplane(SHORT_STDPARAMS) {
#[     init_headers(SHORT_STDPARAMS_IN);
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     init_keyless_tables();
#[     pd->dropped=0;
#} }

#{ void update_packet(packet_descriptor_t* pd) {
#[     uint32_t value32, res32;
#[     (void)value32, (void)res32;
for hdr in hlir16.header_instances:
    #[ 
    #[ // updating header instance ${hdr.name}

    for fld in hdr.type.type_ref.valid_fields:
        if not fld.preparsed and fld.type.size <= 32:
            #{ if(pd->fields.attr_field_instance_${hdr.name}_${fld.name} == MODIFIED) {
            #[     value32 = pd->fields.field_instance_${hdr.name}_${fld.name};
            #[     MODIFY_INT32_INT32_AUTO_PACKET(pd, header_instance_${hdr.name}, field_instance_${hdr.name}_${fld.name}, value32);
            #[     // set_field((fldT[]){{pd, header_instance_${hdr.name}, field_${hdr.type.type_ref.name}_${fld.name}}}, 0, value32, ${fld.type.size});
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

# objects for externs
for m in hlir16.objects['Method']:
    # TODO temporary fix for l3-routing-full, this will be computed later on
    with types({
        "T": "struct uint8_buffer_s",
        "O": "unsigned",
        "HashAlgorithm": "int",
    }):
        t = m.type
        ret_type = format_type(t.returnType)
        args = ", ".join([format_expr(arg) for arg in t.parameters.parameters] + [STDPARAMS])

        #[ extern ${ret_type} ${m.name}(${args});

for pe in pipeline_elements:
    ctl = hlir16.objects.get(pe.expression.type.name, 'P4Control')
        
    if ctl is None:
        continue

    #[ void control_${pe.expression.type.name}(STDPARAMS)
    #{ {
    #[     debug("Entering control $$[control]{ctl.name}...\n");
    #[     uint32_t value32, res32;
    #[     (void)value32, (void)res32;
    #[     control_locals_${pe.expression.type.name}_t control_locals_struct;
    #[     control_locals_${pe.expression.type.name}_t* control_locals = &control_locals_struct;
    #[     pd->control_locals = (void*) control_locals;
    #= format_statement_ctl(ctl.body, ctl)
    #} }

#[ void process_packet(STDPARAMS)
#{ {
for pe in pipeline_elements:
    ctl = hlir16.objects.get(pe.expression.type.name, 'P4Control')
    if ctl is not None:
        #[ control_${pe.expression.type.name}(${STDPARAMS_IN});
        if pe.expression.type.name == 'egress':
            #[ update_packet(pd); // we need to update the packet prior to calculating the new checksum
#} }

################################################################################

metadata_names = {hi.name for hi in hlir16.header_instances if hi.type.type_ref.is_metadata}
longest_hdr_name_len = max({len(h.name) for h in hlir16.header_instances if h.name not in metadata_names})

pkt_name_indent = " " * longest_hdr_name_len

#[ void store_headers_for_emit(STDPARAMS)
#{ {
#[     debug("   :: Preparing $${}{%d} header instances for storage...\n", pd->emit_hdrinst_count);

#[     uint8_t* storage = pd->header_tmp_storage;
#[     pd->emit_headers_length = 0;
#{     for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];

#{         if (hdr.pointer == NULL) {
#[             debug("    : Skipping header   ($$[header][%]{longest_hdr_name_len}{s})  : (invalid header)\n", hdr.name);
#[             continue;
#}         }

#[         dbg_bytes(hdr.pointer, hdr.length, "    : Storing  $${}{%02d} bytes ($$[header][%]{longest_hdr_name_len}{s})  : ", hdr.length, hdr.name);

#[         memcpy(storage, hdr.pointer, hdr.length);
#[         storage += hdr.length;
#[         pd->emit_headers_length += hdr.length;
#}     }

#[     dbg_bytes(pd->header_tmp_storage, pd->emit_headers_length, "   :: Stored   $${}{%02d} bytes     $pkt_name_indent: ", pd->emit_headers_length);
#} }

#[ void resize_packet_on_emit(STDPARAMS)
#{ {
#{     if (likely(pd->emit_headers_length == pd->parsed_length)) {
#[         debug("   :: Emitting $${}{%02d} bytes (no resize)\n", pd->emit_headers_length);
#[         return;
#}     }
#[
#{     if (likely(pd->emit_headers_length > pd->parsed_length)) {
#[         int len_change = pd->emit_headers_length - pd->parsed_length;
#[         debug("   :: Adding   $${}{%02d} bytes %${longest_hdr_name_len}{s}   : (header: from $${}{%d} bytes to $${}{%d} bytes)\n", len_change, "to packet", pd->parsed_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_prepend(pd->wrapper, len_change);
#[         if (unlikely(new_ptr == 0)) {
#[             rte_exit(1, "Could not reserve necessary headroom ($${}{%d} additional bytes)", len_change);
#[         }
#[         pd->data = (packet_data_t*)new_ptr;
#[     } else {
#[         int len_change = pd->parsed_length - pd->emit_headers_length;
#[         debug("   :: Removing $${}{%02d} bytes %${longest_hdr_name_len}{s}  : (header: from $${}{%d} bytes to $${}{%d} bytes)\n", len_change, "from packet", pd->parsed_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_adj(pd->wrapper, len_change);
#[         pd->data = (packet_data_t*)new_ptr;
#}     }
#[     pd->wrapper->pkt_len = pd->emit_headers_length + pd->payload_length;
#} }

#[ void copy_emit_contents(STDPARAMS)
#{ {
#[     dbg_bytes(pd->header_tmp_storage, pd->emit_headers_length, "   :: Headers: $${}{%02d} bytes %${longest_hdr_name_len}{s} : ", pd->emit_headers_length, "from storage");
#[     memcpy(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), pd->header_tmp_storage, pd->emit_headers_length);
#} }

#[ void emit_packet(STDPARAMS)
#{ {
#[     if (unlikely(pd->is_emit_reordering)) {
#[         debug(" :::: Reordering emit\n");
#[         store_headers_for_emit(${STDPARAMS_IN});
#[         resize_packet_on_emit(${STDPARAMS_IN});
#[         copy_emit_contents(${STDPARAMS_IN});
#[     }
#} }

#[ static void set_metadata_inport(packet_descriptor_t* pd, uint32_t inport)
#[ {
#[     int res32; // needed for the macro
#[     MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, inport);
#[ }

#[ void handle_packet(STDPARAMS, uint32_t portid)
#{ {
#[     int value32;
#[     int res32;
#[
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     set_metadata_inport(pd, portid);
#[
#[     dbg_bytes(pd->data, rte_pktmbuf_pkt_len(pd->wrapper), "Handling packet (port %" PRIu32 ", $${}{%02d} bytes)  : ", EXTRACT_INGRESSPORT(pd), rte_pktmbuf_pkt_len(pd->wrapper));
#[
#[     pd->parsed_length = 0;
#[     parse_packet(${STDPARAMS_IN});
#[     pd->payload_length = rte_pktmbuf_pkt_len(pd->wrapper) - pd->parsed_length;
#[
#[     emit_addr = pd->data;
#[     pd->emit_hdrinst_count = 0;
#[     pd->is_emit_reordering = false;
#[
#[     process_packet(${STDPARAMS_IN});
#[
#[     emit_packet(${STDPARAMS_IN});
#} }
