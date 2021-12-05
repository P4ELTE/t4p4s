// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_mbuf.h>
typedef struct rte_mbuf packet;

typedef uint8_t packet_data_t;

#include <rte_spinlock.h>
typedef rte_spinlock_t lock_t;

#define INVALID_ACTION -1

#define FLD_ATTR(hdr,fld) attr_field_instance_##hdr##_##fld
#define FLD(hdr,fld) field_instance_##hdr##_##fld
#define HDR(hdr) header_##hdr
#define STK(stk) stack_##stk

// TODO include the control's/parser's name in there
#define LOCALNAME(ctl,name)   name

#define EXTERNTYPE(name)      externtype_##name##_t
#define EXTERNNAME(name)      externname_##name
#define SMEMTYPE(name)        smemtype_##name##_t
#define SMEMNAME(name)        externname_##name##_t
#define REGTYPE(signed,size)  register##_##signed##size##_t

#define EXTERN(part1,part2)                    part1##_##part2
#define SMEM2(part1,part2)                     part1##_##part2
#define SMEM3(part1,part2,part3)               part1##_##part2##_##part3
#define SMEM4(part1,part2,part3,part4)         part1##_##part2##_##part3##_##part4
#define SMEM5(part1,part2,part3,part4,part5)   part1##_##part2##_##part3##_##part4##_##part5

#define STR_SMEM2(part1,part2)                     #part1 "_" #part2
#define STR_SMEM3(part1,part2,part3)               #part1 "_" #part2 "_" #part3
#define STR_SMEM4(part1,part2,part3,part4)         #part1 "_" #part2 "_" #part3 "_" #part4
#define STR_SMEM5(part1,part2,part3,part4,part5)   #part1 "_" #part2 "_" #part3 "_" #part4 "_" #part5

#define EXTERNCALL0(externname,name)                                 extern_##externname##_##name
#define EXTERNCALL1(externname,name,type1)                           extern_##externname##_##name##_##type1
#define EXTERNCALL2(externname,name,type1,type2)                     extern_##externname##_##name##_##type1##_##type2
#define EXTERNCALL3(externname,name,type1,type2,type3)               extern_##externname##_##name##_##type1##_##type2##_##type3
#define EXTERNCALL4(externname,name,type1,type2,type3,type4)         extern_##externname##_##name##_##type1##_##type2##_##type3##_##type4
#define EXTERNCALL5(externname,name,type1,type2,type3,type4,type5)   extern_##externname##_##name##_##type1##_##type2##_##type3##_##type4##_##type5

#define SHORT_EXTERNCALL0(externname)                                extern_##externname
#define SHORT_EXTERNCALL1(externname,type1)                          extern_##externname##_##type1
#define SHORT_EXTERNCALL2(externname,type1,type2)                    extern_##externname##_##type1##_##type2
#define SHORT_EXTERNCALL3(externname,type1,type2,type3)              extern_##externname##_##type1##_##type2##_##type3
#define SHORT_EXTERNCALL4(externname,type1,type2,type3,type4)        extern_##externname##_##type1##_##type2##_##type3##_##type4
#define SHORT_EXTERNCALL5(externname,type1,type2,type3,type4,type5)  extern_##externname##_##type1##_##type2##_##type3##_##type4##_##type5

#define EXTERNIMPL0(externname)                                externimpl_##externname
#define EXTERNIMPL1(externname,type1)                          externimpl_##externname##_##type1
#define EXTERNIMPL2(externname,type1,type2)                    externimpl_##externname##_##type1##_##type2
#define EXTERNIMPL3(externname,type1,type2,type3)              externimpl_##externname##_##type1##_##type2##_##type3
#define EXTERNIMPL4(externname,type1,type2,type3,type4)        externimpl_##externname##_##type1##_##type2##_##type3##_##type4
#define EXTERNIMPL5(externname,type1,type2,type3,type4,type5)  externimpl_##externname##_##type1##_##type2##_##type3##_##type4##_##type5
