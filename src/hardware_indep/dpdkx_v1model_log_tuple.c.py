# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary


# TODO move it to the DPDK specific area

#[ #include "common.h"
#[ #include "util_debug.h"

#[ void make_formatter_tuple(char* out_fmt, const char* in_fmt, ...) {
#[     va_list argp;
#[     va_start(argp, in_fmt);
#[ 
#[     char* param_ptr;
#[     while ((param_ptr = strstr(in_fmt, "{}")) != NULL) {
#[         const char* param_fmt = va_arg(argp, const char*);
#[ 
#[         int pre_param_len = param_ptr - in_fmt;
#[         int param_len = strlen(param_fmt);
#[ 
#[         memcpy(out_fmt, in_fmt, pre_param_len);
#[         strcpy(out_fmt + pre_param_len, param_fmt);
#[         out_fmt += pre_param_len + param_len;
#[         in_fmt += pre_param_len + 2; // skip the {} characters
#[     }
#[ 
#[     // the remaining format characters don't contain {}, copy them
#[     strcpy(out_fmt, in_fmt);
#[ 
#[     va_end(argp);
#[ }
#[
#[ void print_log_msg_tuple(const char* msg) {
#[     #ifdef T4P4S_DEBUG
#[         debug("    : " T4LIT(Logged,status) ": %s\n", msg);
#[     #else
#[         printf("    : " T4LIT(Logged,status) ": %s\n", msg);
#[     #endif
#[ }
#[


for struct in hlir.news.data:
    name = re.sub('_t$', '', struct.name)

    def get_fmt(fld):
        if fld.urtype.node_type == 'Type_Stack':
            return '"TODO_Stack1"'

        if fld.urtype.node_type == 'Type_Header':
            parts = '", "'.join(f'"{hdrfld.name}="{get_fmt(hdrfld)}' for hdrfld in fld.urtype.fields)
            return f'"["{parts}"]"'

        size = fld.urtype.size
        if size <= 32:
            return f'T4LIT(0x%0{size//4}x,bytes) "=" T4LIT(%d) "/" T4LIT({size}) "b"'

        bytes_fmt = '%02x' * (size//8)
        return f'T4LIT(0x{bytes_fmt},bytes) "/" T4LIT({size//8}) "B"'

    def get_contents(fld, hdrname=None):
        if fld.urtype.node_type == 'Type_Stack':
            return "TODO_Stack2"

        if (hdr := fld).urtype.node_type == 'Type_Header':
            return ', '.join(f'{hdrfld.name}={get_contents(hdrfld, hdr.name)}' for hdrfld in hdr.urtype.fields)

        size = fld.urtype.size
        container = f'parts->{hdrname}.{fld.name}' if hdrname else f'parts->{fld.name}'
        if size <= 32:
            return f'{container}, {container}'
        return ', '.join(f'((uint8_t*){container})[{idx}]' for idx in range(0, size//8))

    fmts = ', '.join(['fmt', '*msg'] + list(struct.fields.map(get_fmt)))
    contents = ', '.join(struct.fields.map(get_contents))

    #{ void log_msg__${name}(const char** msg, const ${name}_t* parts, SHORT_STDPARAMS) {
    #[     char fmt[1024];
    #[     char text[1024];
    #[     make_formatter_tuple($fmts);
    #[     sprintf(text, fmt, "$contents");
    #[     print_log_msg_tuple(text);
    #} }
