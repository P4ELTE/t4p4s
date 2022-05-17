# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addWarning
from utils.codegen import to_c_bool

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
all_table_infos = sorted(zip(hlir.tables, table_names), key=lambda k: len(k[0].actions))
table_infos = list(ti for idx, ti in enumerate(all_table_infos) if idx % part_count == multi_idx)

all_hdrs = sorted(hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata), key=lambda hdr: len(hdr.urtype.fields))
hdrs = list(hdr for idx, hdr in enumerate(all_hdrs) if idx % part_count == multi_idx)

if hdrs == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    #[ #include "parser_stages.h"
    #[ #include "hdr_fld.h"
    #[ #include "hdr_fld_sprintf.h"

    for hdr in hdrs:
        #{ void print_parsed_hdr_${hdr.name}(packet_descriptor_t* pd, header_descriptor_t* hdr, header_instance_e hdrinst) {
        #{     #ifdef T4P4S_DEBUG
        #[         char fields_txt[4096];
        #[         if (hdr->size > 0)    sprintf_hdr(fields_txt, pd, hdr);
        #[         debug("   :: Parsed header" T4LIT(#%d) " " T4LIT(%s,header) "/" T4LIT(%d) "B%s%s\n",
        #[               hdr_infos[hdrinst].idx + 1, hdr_infos[hdrinst].name, hdr->size,
        #[               hdr->size == 0 ? "" : ": ",
        #[               hdr->size == 0 ? "" : fields_txt);
        #}     #endif
        #} }

    for hdr in hdrs:
        #{ int parser_extract_${hdr.name}(int vwlen, STDPARAMS) {
        #[     parser_state_t* local_vars = pstate;

        hdrtype = hdr.urtype
        is_vw = hdrtype.is_vw
        base_size = hdr.urtype.size

        is_stack = 'stack' in hdr and hdr.stack is not None

        if is_stack:
            #[     stk_next(STK(${hdr.stack.name}), pd);
            #[     header_instance_e hdrinst = stk_current(STK(${hdr.stack.name}), pd);
        else:
            #[     header_instance_e hdrinst = HDR(${hdr.name});
        #[     header_descriptor_t* hdr = &(pd->headers[hdrinst]);

        #[     hdr->was_enabled_at_initial_parse = ${to_c_bool(not hdr.is_skipped)};
        #[     hdr->size = (${base_size} + vwlen) / 8;
        if is_vw:
            #[     hdr->vw_size = vwlen;

        #{     if (unlikely(pd->parsed_size + hdr->size > pd->wrapper->pkt_len)) {
        #[         cannot_parse_hdr("${"variable width " if is_vw else ""}", "${hdr.name}", ${base_size}, vwlen, STDPARAMS_IN);
        #[         return PARSED_AFTER_END_OF_PACKET;
        #}     }

        if hdr.is_skipped:
            #[     hdr->pointer = NULL;
            skip_size = hdrtype.size
            skip_pad_txt = ''
            if hdrtype.size % 8 != 0:
                padded_skip_size = ((skip_size+7) // 8) * 8
                addWarning('Skipping bits', f'Only byte aligned skipping is supported, {skip_size}b is padded to {padded_skip_size//8}B')
                skip_size = padded_skip_size
                skip_pad_txt = '"->" T4LIT()'
            #{     if (unlikely(pd->parsed_size + ${skip_size//8} > pd->wrapper->pkt_len)) {
            #[         debug("   " T4LIT(!!,error) " Tried to skip " T4LIT(<${hdrtype.name}>,header) "/" T4LIT(%d) "B but it is over packet size\n", ${skip_size} / 8);
            #[         return PARSED_AFTER_END_OF_PACKET;
            #[     } else {
            #[         debug("   :: Skipping " T4LIT(<${hdrtype.name}>,header) "/" T4LIT(%d) "B\n", ${skip_size} / 8);
            #}     }

            #[     pd->parsed_size += ${skip_size//8};
        else:
            #[     hdr->pointer = pd->extract_ptr;

            for fld in hdrtype.fields:
                if fld.preparsed and fld.size <= 32:
                    #[     pd->fields.FLD(hdr,$name) = GET32(src_pkt(pd), FLD(${hdr.name}, $name));
                    #[     pd->fields.ATTRFLD(hdr,$name) = NOT_MODIFIED;

            #[     print_parsed_hdr_${hdr.name}(pd, hdr, hdrinst);

            #[     pd->parsed_size += hdr->size;
    
        #[     pd->extract_ptr += hdr->size;

        if hdr.is_skipped:
            #[     return 0; // ${hdr.name} is skipped
        else:
            #[     return hdr->size;
        #} }
        #[
