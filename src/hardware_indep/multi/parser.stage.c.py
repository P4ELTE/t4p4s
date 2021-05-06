# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
all_table_infos = sorted(zip(hlir.tables, table_names), key=lambda k: len(k[0].actions))
table_infos = list(ti for idx, ti in enumerate(all_table_infos) if idx % part_count == multi_idx)

all_hdrs = sorted(hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata), key=lambda hdr: len(hdr.urtype.fields))
hdrs = list(hdr for idx, hdr in enumerate(all_hdrs) if idx % part_count == multi_idx)

if hdrs == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    for hdr in hdrs:
        #[ #include "parser_stages.h"

        #{ int parser_extract_${hdr.name}(uint32_t vwlen, STDPARAMS) {
        #[     uint32_t value32; (void)value32;
        #[     uint32_t res32; (void)res32;
        #[     parser_state_t* local_vars = pstate;

        hdrtype = hdr.urtype

        is_vw = hdrtype.is_vw

        #[     uint32_t hdrlen = (${hdr.urtype.size} + vwlen) / 8;
        #{     if (unlikely(pd->parsed_length + hdrlen > pd->wrapper->pkt_len)) {
        #[         cannot_parse_hdr("${"variable width " if is_vw else ""}", "${hdr.name}", hdrlen, STDPARAMS_IN);
        #[         return -1; // parsed after end of packet
        #}     }

        if 'stack' in hdr and hdr.stack is not None:
            #[     stk_next(STK(${hdr.stack.name}), pd);
            #[     header_instance_t hdrinst = stk_current(STK(${hdr.stack.name}), pd);
        else:
            #[     header_instance_t hdrinst = HDR(${hdr.name});
        #[     header_descriptor_t* hdr = &(pd->headers[hdrinst]);

        #[     hdr->pointer = pd->extract_ptr;
        #[     hdr->was_enabled_at_initial_parse = true;
        #[     hdr->length = hdrlen;
        #[     hdr->var_width_field_bitwidth = vwlen;

        for fld in hdrtype.fields:
            if fld.preparsed and fld.size <= 32:
                #[     EXTRACT_INT32_AUTO_PACKET(pd, hdr, FLD(hdr,${fld.name}), value32)
                #[     pd->fields.FLD(hdr,${fld.name}) = value32;
                #[     pd->fields.ATTRFLD(hdr,${fld.name}) = NOT_MODIFIED;

        #[     dbg_bytes(hdr->pointer, hdr->length,
        #[               "   :: Parsed ${"variable width " if is_vw else ""}header" T4LIT(#%d) " " T4LIT(%s,header) "/$${}{%dB}: ", hdr_infos[hdrinst].idx, hdr_infos[hdrinst].name, hdr->length);

        #[     pd->parsed_length += hdrlen;
        #[     pd->extract_ptr += hdrlen;

        #[     return hdrlen;
        #} }
        #[
