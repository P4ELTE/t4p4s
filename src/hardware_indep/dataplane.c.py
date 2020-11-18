# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types

#[ #include <stdlib.h>
#[ #include <string.h>
#[ #include <stdbool.h>
#[ #include <netinet/in.h>

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include "backend.h"
#[ #include "util_debug.h"
#[ #include "tables.h"
#[ #include "gen_include.h"

#[ uint8_t* emit_addr;
#[ uint32_t ingress_pkt_len;

#[ extern ctrl_plane_backend bg;
#[ extern char* action_canonical_names[];
#[ extern char* action_names[];

#[ extern void parse_packet(STDPARAMS);
#[ extern void increase_counter(int counterid, int index);
#[ extern void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);

# note: 0 is for the special case where there are no tables
max_key_length = max([t.key_length_bytes for t in hlir.tables] + [0])
#[ uint8_t reverse_buffer[${max_key_length}];


#{ #ifdef T4P4S_STATS
#[     extern t4p4s_stats_t t4p4s_stats;
#} #endif

################################################################################

packet_name = hlir.news.main.type.baseType.type_ref.name
pipeline_elements = hlir.news.main.arguments

#{ typedef struct apply_result_s {
#[     bool hit;
#[     // actions_t action_run;
#} } apply_result_t;

for ctl in hlir.controls:
    #[ void control_${ctl.name}(STDPARAMS);
    for t in ctl.controlLocals['P4Table']:
        #[ apply_result_t ${t.name}_apply(STDPARAMS);

################################################################################
# Table key calculation

for table in hlir.tables:
    #{ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key) {
    sortedfields = sorted(table.key.keyElements, key=lambda k: k.match_order)
    #TODO variable length fields
    #TODO field masks
    for f in sortedfields:
        if 'header' in f:
            hi_name = "all_metadatas" if f.header.urtype.is_metadata else f.header.name

            #{ #ifdef T4P4S_DEBUG
            #{     if (unlikely(pd->headers[HDR(${hi_name})].pointer == NULL)) {
            #[         debug(" " T4LIT(!!!!,error) " " T4LIT(Lookup on invalid header,error) " " T4LIT(${hi_name},header) "." T4LIT(${f.field_name},field) "\n");
            #}     }
            #} #endif
            if f.size <= 32:
                #[ EXTRACT_INT32_BITS_PACKET(pd, HDR(${hi_name}), FLD(${f.header.name},${f.field_name}), *(uint32_t*)key)
                #[ key += sizeof(uint32_t);
            elif f.size > 32 and f.size % 8 == 0:
                byte_width = (f.size+7)//8
                #[ EXTRACT_BYTEBUF_PACKET(pd, HDR(${hi_name}), FLD(${f.header.name},${f.field_name}), key)
                #[ key += ${byte_width};
            else:
                addWarning("table key computation", f"Skipping unsupported field {f.id} ({f.size} bits): it is over 32 bits long and not byte aligned")
        else:
            # f is a control local
            if f.size <= 32 or f.size % 8 == 0:
                byte_width = (f.size+7)//8
                fld_name = f.expression.path.name
                #[ memcpy(key, &((control_locals_${table.control.name}_t*) pd->control_locals)->${fld_name}, ${byte_width});
                #[ key += ${byte_width};
            else:
                addWarning("table key computation", f"Skipping unsupported key component {f.expression.path.name} ({f.size} bits): it is over 32 bits long and not byte aligned")


    if table.matchType.name == "lpm":
        #[ key -= ${table.key_length_bytes};
        #[ for(int c = ${table.key_length_bytes-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(int c = 0; c < ${table.key_length_bytes}; c++) *(key+c) = *(reverse_buffer+c);
    #} }

################################################################################
# Table application

def unique_stable(items):
    """Returns only the first occurrence of the items in a list.
    Equivalent to unique_everseen from Python 3."""
    from collections import OrderedDict
    return list(OrderedDict.fromkeys(items))


for type in unique_stable([comp['type'] for table in hlir.tables for smem in table.direct_meters + table.direct_counters for comp in smem.components]):
    #[ void apply_direct_smem_$type(register_uint32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    #[    debug("     : applying apply_direct_smem_$type(register_uint32_t (*smem)[1], uint32_t value, char* table_name, char* smem_type_name, char* smem_name)");
    #[ }


#[ #define STD_DIGEST_RECEIVER_ID 1024

# TODO make it unique by digest name
for mcall in hlir.all_nodes.by_type('MethodCallStatement').map('methodCall').filter(lambda n: 'path' in n.method and n.method.path.name=='digest'):
    digest = mcall.typeArguments[0]
    funname = f'{mcall.method.path.name}__{digest.path.name}'

    #{ ${format_type(mcall.urtype)} $funname(ctrl_plane_backend bg, ${digest.path.name}_t digest, SHORT_STDPARAMS) {
    #[     debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(%d,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(digest,extern) "\n", STD_DIGEST_RECEIVER_ID);

    #[     ctrl_plane_digest cpd = create_digest(bg, "digest");

    for fld in digest.urtype.fields:
        if fld.urtype.size > 32:
            #[     dbg_bytes(&(digest.${fld.name}), (${fld.urtype.size}+7)/8, "       : $[field]{fld.name}/" T4LIT(${fld.urtype.size}) " = ");
            #[     add_digest_field(cpd, &(digest.${fld.name}), ${fld.urtype.size});
        else:
            #[     debug("       : " T4LIT(ingress_port,field) "/" T4LIT(${fld.urtype.size}) " = " T4LIT(%x) "\n", digest.${fld.name});
            #[     add_digest_field(cpd, &(digest.${fld.name}), ${fld.urtype.size});

    #[     send_digest(bg, cpd, STD_DIGEST_RECEIVER_ID);
    #[     sleep_millis(DIGEST_SLEEP_MILLIS);
    #} }


#{ #ifdef T4P4S_DEBUG
for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    table_info = table.canonical_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")

    #{ void ${table.name}_apply_show_hit_info(const uint8_t* key[${table.key_length_bytes}], bool hit, table_entry_${table.name}_t* entry, STDPARAMS) {
    for dbg_action in table.actions:
        dbg_action_name = dbg_action.expression.method.path.name
        #{ if (entry != 0 && !strcmp("${dbg_action_name}", action_names[entry->action.action_id])) {
        params = dbg_action.expression.method.type.parameters.parameters

        def make_value(value):
            is_hex = value.base == 16
            split_places = 4 if is_hex else 3

            prefix = '0x' if is_hex else ''
            val = f'{value.value:x}' if is_hex else f'{value.value}'
            val = '_'.join(val[::-1][i:i+split_places] for i in range(0, len(val), split_places))[::-1]
            return f'{prefix}{val}'

        param_fmts = (f'" T4LIT(%d) "=" T4LIT(%0{(sz+3)//4}x) "' if sz <= 32 else f'(" T4LIT({(sz+7)//8}b) ")' for param in params for sz in [param.urtype.size])
        params_str = ", ".join((f'" T4LIT({param.name},field) "/" T4LIT({param.urtype.size}b) "={fmt}' for param, fmt in zip(params, param_fmts)))
        if params_str != "":
            params_str = f'({params_str})'

        #[     dbg_bytes(key, table_config[TABLE_${table.name}].entry.key_size,
        #[               " %s Lookup on $$[table]{table_info}/" T4LIT(${table.matchType.name}) "/" T4LIT(%dB) ": $$[action]{}{%s}${params_str}%s <- %s ",
        #[               hit ? T4LIT(++++,success) : T4LIT(XXXX,status),
        #[               ${table.key_length_bytes},
        #[               action_canonical_names[entry->action.action_id],
        for param in params:
            if param.urtype.size <= 32:
                #[               *(entry->action.${dbg_action.action_object.name}_params.${param.name}), // decimal
                #[               *(entry->action.${dbg_action.action_object.name}_params.${param.name}), // hex
        #[               hit ? "" : " (default)",
        #[               hit ? T4LIT(hit,success) : T4LIT(miss,status)
        #[               );
        #} }
    #} }
    #[
#} #endif

for table in hlir.tables:
    table_info = table.canonical_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")

    #[ apply_result_t ${table.name}_apply(STDPARAMS)
    #{ {
    if 'key' in table:
        #[     uint8_t* key[${table.key_length_bytes}];
        #[     table_${table.name}_key(pd, (uint8_t*)key);

        if table.key_length_bits == 0:
            #[     table_entry_${table.name}_t* entry = (table_entry_${table.name}_t*)tables[TABLE_${table.name}]->default_val;
            #[     bool hit = false;

            if table.is_hidden or len(table.actions) == 1:
                #[     debug(" ~~~~ Action $$[action]{}{%s} (lookup on $$[table]{table_info})\n",
                #[               action_canonical_names[entry->action.action_id]
                #[               );
            else:
                #[     debug(" " T4LIT(XXXX,status) " Lookup on $$[table]{table_info}: $$[action]{}{%s} (default)\n",
                #[               action_canonical_names[entry->action.action_id]
                #[               );
        else:
            #[     table_entry_${table.name}_t* entry = (table_entry_${table.name}_t*)${table.matchType.name}_lookup(tables[TABLE_${table.name}], (uint8_t*)key);
            #[     bool hit = entry != NULL && entry->is_entry_valid != INVALID_TABLE_ENTRY;
            #{     if (unlikely(!hit)) {
            #[         entry = (table_entry_${table.name}_t*)tables[TABLE_${table.name}]->default_val;
            #}     }

            #{ #ifdef T4P4S_DEBUG
            #[     ${table.name}_apply_show_hit_info(key, hit, entry, STDPARAMS_IN);
            #} #endif

            #{ #ifdef T4P4S_STATS
            #[     t4p4s_stats.table_hit__${table.name} = hit || t4p4s_stats.table_hit__${table.name};
            #[     t4p4s_stats.table_miss__${table.name} = !hit || t4p4s_stats.table_miss__${table.name};
            #} #endif

            if len(table.direct_meters + table.direct_counters) > 0:
                #{     if (likely(hit)) {
                #[         // applying direct counters and meters
                for smem in table.direct_meters + table.direct_counters:
                    for comp in smem.components:
                        value = "pd->parsed_length" if comp['for'] == 'bytes' else "1"
                        type = comp['type']
                        name  = comp['name']
                        #[         extern void apply_${smem.smem_type}(${smem.smem_type}_t*, int, const char*, const char*, const char*);
                        #[         apply_${smem.smem_type}(&(global_smem.${name}_${table.name}), $value, "${table.name}", "${smem.smem_type}", "$name");
                #}    }
    else:
        if 'default_action' in table:
            #[    table_entry_${table.name}_t* entry = (table_entry_${table.name}_t*)tables[TABLE_${table.name}][rte_lcore_id()].default_val;
            #[    debug(" :::: Lookup on " T4LIT(${table_info},table) ", default action is " T4LIT(%s,action) "\n", action_names[entry->action.action_id]);
            #[    bool hit = true;
        else:
            #[    debug(" :::: Lookup on " T4LIT(${table_info},table) ", " T4LIT(no default action,action) "\n");
            #[    table_entry_${table.name}_t* entry = (struct ${table.name}_action*)0;
            #[    bool hit = false;

    #{     #ifdef T4P4S_STATS
    #[         t4p4s_stats.table_apply__${table.name} = true;

    for stat_action in table.actions:
        stat_action_name = stat_action.expression.method.path.name
        #{         if (entry != 0 && !strcmp("${stat_action_name}", action_names[entry->action.action_id])) {
        #[             t4p4s_stats.table_action_used__${table.name}_${stat_action_name} = true;
        #}         }
    #}     #endif


    # ACTIONS
    #[     if (likely(entry != 0)) {
    #{       switch (entry->action.action_id) {
    for action in table.actions:
        action_name = action.action_object.name
        #{         case action_${action_name}:
        #[           action_code_${action_name}(entry->action.${action_name}_params, SHORT_STDPARAMS_IN);
        #}           return (apply_result_t) { hit };
    #[       }
    #}     }

    #[     return (apply_result_t) { hit }; // unreachable
    #} }
    #[


################################################################################

#{ void reset_headers(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    #[ pd->headers[HDR(${hdr.name})].pointer = NULL;

#[     // reset metadatas
#[     memset(pd->headers[HDR(all_metadatas)].pointer, 0, hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t));
#} }

#{ void init_headers(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    #[ pd->headers[HDR(${hdr.name})] = (header_descriptor_t)
    #{ {
    #[     .type = HDR(${hdr.name}),
    #[     .length = hdr_infos[HDR(${hdr.name})].byte_width,
    #[     .pointer = NULL,
    #[     .var_width_field_bitwidth = 0,
    #[ #ifdef T4P4S_DEBUG
    #[     .name = "${hdr.name}",
    #[ #endif
    #} };

#[     // init metadatas
#[     pd->headers[HDR(all_metadatas)] = (header_descriptor_t)
#{     {
#[         .type = HDR(all_metadatas),
#[         .length = hdr_infos[HDR(all_metadatas)].byte_width,
#[         .pointer = rte_malloc("all_metadatas_t", hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t), 0),
#[         .var_width_field_bitwidth = 0
#}     };
#} }

################################################################################

def is_keyless_single_action_table(table):
    return table.key_length_bytes == 0 and len(table.actions) == 2 and table.actions[1].action_object.name.startswith('NoAction')

################################################################################

#{ void init_dataplane(SHORT_STDPARAMS) {
#[     init_headers(SHORT_STDPARAMS_IN);
#[     reset_headers(SHORT_STDPARAMS_IN);

#[     uint32_t res32;
#[     MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_INIT_VALUE);
#} }

#{ void update_packet(packet_descriptor_t* pd) {
#[     uint32_t value32, res32;
#[     (void)value32, (void)res32;
for hdr in hlir.header_instances:
    #[
    #[ // updating header ${hdr.name}
    for fld in hdr.urtype.fields:
        if fld.preparsed or fld.urtype.size > 32:
            continue
        #{ if(pd->fields.FLD_ATTR(${hdr.name},${fld.name}) == MODIFIED) {
        #[     value32 = pd->fields.FLD(${hdr.name},${fld.name});
        #[     MODIFY_INT32_INT32_AUTO_PACKET(pd, HDR(all_metadatas), FLD(${hdr.name},${fld.name}), value32);
        #[     // set_field((fldT[]){{pd, HDR(${hdr.name}), FLD(${hdr.name},${fld.name})}}, 0, value32, ${fld.urtype.size});
        #} }
#} }

################################################################################
# Pipeline


for ctl in hlir.controls:
    if len(ctl.body.components) == 0:
        #[ // skipping method generation for empty control ${ctl.name}
        continue

    #[ void control_${ctl.name}(STDPARAMS)
    #{ {
    #[     debug("Control $$[control]{ctl.name}...\n");
    #[     uint32_t value32, res32;
    #[     (void)value32, (void)res32;
    #[     control_locals_${ctl.name}_t local_vars_struct;
    #[     control_locals_${ctl.name}_t* local_vars = &local_vars_struct;
    #[     pd->control_locals = (void*) local_vars;
    #= format_statement(ctl.body, ctl)
    #} }

#[ void process_packet(STDPARAMS)
#{ {
it=0
for ctl in hlir.controls:
    if len(ctl.body.components) == 0:
        #[ // skipping empty control ${ctl.name}
    else:
        #[ control_${ctl.name}(STDPARAMS_IN);

    if hlir.news.model == 'V1Switch' and it==1:
        #[ transfer_to_egress(pd);
    it = it+1
    if ctl.name == 'egress':
        #[ // TODO temporarily disabled
        #[ // update_packet(pd); // we need to update the packet prior to calculating the new checksum
#} }

################################################################################

longest_hdr_name_len = max({len(h.name) for h in hlir.header_instances if not h.urtype.is_metadata})

pkt_name_indent = " " * longest_hdr_name_len

#[ void store_headers_for_emit(STDPARAMS)
#{ {
#{     #ifdef T4P4S_DEBUG
#[         int skips = 0;
#{         for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[             header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];
#[             if (unlikely(hdr.pointer == NULL))    ++skips;
#}         }
#[         int emits = pd->emit_hdrinst_count - skips;
#[         debug("   :: Preparing $${}{%d} header%s for storage, skipping " T4LIT(%d) " header%s...\n",
#[               emits, emits != 1 ? "s" : "", skips, skips != 1 ? "s" : "");
#}     #endif

#[     pd->emit_headers_length = 0;
#{     for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];

#[
#{         #if T4P4S_EMIT != 1
#{             if (unlikely(hdr.pointer == NULL)) {
#{                 #ifdef T4P4S_DEBUG
#{                     if (hdr.was_enabled_at_initial_parse) {
#[                         debug("        : -" T4LIT(#%02d ,status) "$$[status][%]{longest_hdr_name_len}{s}$$[status]{}{/%02dB} (invalidated)\n", pd->header_reorder[i] + 1, hdr.name, hdr.length);
#}                     }
#}                 #endif
#[                 continue;
#}             }
#}         #endif

#{         if (likely(hdr.was_enabled_at_initial_parse)) {
#[             dbg_bytes(hdr.pointer, hdr.length, "        :  " T4LIT(#%02d) " $$[header][%]{longest_hdr_name_len}{s}/$${}{%02dB} = %s", pd->header_reorder[i] + 1, hdr.name, hdr.length, hdr.pointer == NULL ? T4LIT((invalid),warning) " " : "");
#[             memcpy(pd->header_tmp_storage + hdr_infos[hdr.type].byte_offset, hdr.pointer, hdr.length);
#[         } else {
#[             dbg_bytes(hdr.pointer, hdr.length, "        : +" T4LIT(#%02d) " $$[header][%]{longest_hdr_name_len}{s}/$${}{%02dB} = ", pd->header_reorder[i] + 1, hdr.name, hdr.length);
#}         }
#[
#[         pd->emit_headers_length += hdr.length;
#}     }
#} }

#[ void resize_packet_on_emit(STDPARAMS)
#{ {
#[     int old_length = packet_length(pd);
#[     int new_length = pd->emit_headers_length + pd->payload_length;
#{     if (likely(new_length == old_length)) {
#[         debug(" " T4LIT(::::,status) " Skipping packet resizing: no change in total packet header length\n");
#[         return;
#}     }
#[
#[     int len_change = new_length - old_length;
#{     if (likely(len_change > 0)) {
#[         debug("   " T4LIT(::,status) " Adding   $${}{%02d} bytes %${longest_hdr_name_len}{s}, header length: $${}{%dB} to $${}{%dB}\n", len_change, "to packet", old_length - pd->payload_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_prepend(pd->wrapper, len_change);
#[         if (unlikely(new_ptr == 0)) {
#[             rte_exit(1, "Could not reserve necessary headroom ($${}{%d} additional bytes)", len_change);
#[         }
#[         pd->data = (packet_data_t*)new_ptr;
#[     } else {
#[         debug("   " T4LIT(::,status) " Removing $${}{%02d} bytes %${longest_hdr_name_len}{s}, header length: $${}{%dB} to $${}{%dB}\n", -len_change, "from packet", old_length - pd->payload_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_adj(pd->wrapper, -len_change);
#[         pd->data = (packet_data_t*)new_ptr;
#}     }
#[     pd->wrapper->pkt_len = new_length;
#} }

#[ // if (some of) the emitted headers are one after another, this function copies them in one go
#[ void copy_emit_contents(STDPARAMS)
#{ {
#[     // debug("   :: Putting together packet\n");
#[     uint8_t* dst_start = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
#[     uint8_t* dst = dst_start;
#{     for (int idx = 0; idx < pd->emit_hdrinst_count; ) {
#[         #ifdef T4P4S_DEBUG
#[             char header_names_txt[1024];
#[             char* header_names_ptr = header_names_txt;
#[         #endif
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[idx]];
#[         if (unlikely(hdr.pointer == NULL))    { ++idx; continue; }
#[         uint8_t* copy_start     = hdr.pointer;
#[         int copy_start_idx      = idx;
#[         int copy_length         = hdr.length;
#[         int count               = 1;
#[         #ifdef T4P4S_DEBUG
#[             header_names_ptr += sprintf(header_names_ptr, T4LIT(%s,header) "/" T4LIT(%dB), hdr.name, hdr.length);
#[         #endif
#[         ++idx;
#{         while (idx < pd->emit_hdrinst_count && pd->headers[pd->header_reorder[idx]].pointer == hdr.pointer + hdr.length) {
#[             ++count;
#[             hdr = pd->headers[pd->header_reorder[idx]];
#[             ++idx;
#[             if (unlikely(hdr.pointer == NULL))    continue;
#[             copy_length += hdr.length;
#[             #ifdef T4P4S_DEBUG
#[                 header_names_ptr += sprintf(header_names_ptr, " " T4LIT(%s,header) "/" T4LIT(%dB), hdr.name, hdr.length);
#[             #endif
#}         }
#[         // dbg_bytes(copy_start, copy_length, "    : Copying " T4LIT(%d) " %s to byte " T4LIT(#%2ld) " of egress header %s: ", count, count == 1 ? "header" : "adjacent headers", dst - dst_start, header_names_txt);
#[         memcpy(dst, copy_start, copy_length);
#[         dst += copy_length;
#}     }
#} }

#{ bool is_packet_dropped(STDPARAMS) {
#[      return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD) == EGRESS_DROP_VALUE;
#} }


#[ void emit_packet(STDPARAMS)
#{ {
#{     if (unlikely(pd->is_emit_reordering)) {
#{         if (unlikely(is_packet_dropped(STDPARAMS_IN))) {
#[             debug(" " T4LIT(::::,status) " Skipping pre-emit processing: packet is " T4LIT(dropped,status) "\n");
#[             return;
#}         }
#[         debug(" :::: Pre-emit reordering\n");
#[         store_headers_for_emit(STDPARAMS_IN);
#[         resize_packet_on_emit(STDPARAMS_IN);
#[         copy_emit_contents(STDPARAMS_IN);
#[     } else {
#[         debug(" " T4LIT(::::,status) " Skipping pre-emit processing: no change in packet header structure\n");
#}     }
#} }

#[ void handle_packet(uint32_t portid, STDPARAMS)
#{ {
#[     int value32;
#[     int res32;
#[
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     set_handle_packet_metadata(pd, portid);
#[
#[     dbg_bytes(pd->data, packet_length(pd), "Handling packet (port " T4LIT(%d,port) ", $${}{%02dB}): ", extract_ingress_port(pd), packet_length(pd));
#[
#[     pd->parsed_length = 0;
#[     pd->is_emit_reordering = false;
#[     parse_packet(STDPARAMS_IN);
#[
#[     emit_addr = pd->data;
#[     pd->emit_hdrinst_count = 0;
#[
#[     process_packet(STDPARAMS_IN);
#[
#[     emit_packet(STDPARAMS_IN);
#} }
