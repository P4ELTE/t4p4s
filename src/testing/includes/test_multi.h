// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#pragma once

// Replicates the given part N times as a single header (or part of a single header).
#define x2(part)    part part
#define x3(part)    part part part
#define x4(part)    part part part part
#define x5(part)    part part part part part
#define x6(part)    part part part part part part
#define x7(part)    part part part part part part part
#define x8(part)    part part part part part part part part
#define x9(part)    part part part part part part part part part
#define x10(part)   part part part part part part part part part part
#define x11(part)   part part part part part part part part part part part
#define x12(part)   part part part part part part part part part part part part
#define x13(part)   part part part part part part part part part part part part part
#define x14(part)   part part part part part part part part part part part part part part
#define x15(part)   part part part part part part part part part part part part part part part
#define x16(part)   part part part part part part part part part part part part part part part part
#define x17(part)   part part part part part part part part part part part part part part part part part
#define x18(part)   part part part part part part part part part part part part part part part part part part
#define x19(part)   part part part part part part part part part part part part part part part part part part part
#define x20(part)   part part part part part part part part part part part part part part part part part part part part
#define x21(part)   part part part part part part part part part part part part part part part part part part part part part
#define x22(part)   part part part part part part part part part part part part part part part part part part part part part part
#define x23(part)   part part part part part part part part part part part part part part part part part part part part part part part
#define x24(part)   part part part part part part part part part part part part part part part part part part part part part part part part
#define x25(part)   part part part part part part part part part part part part part part part part part part part part part part part part part
#define x26(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x27(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x28(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x29(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x30(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x31(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part
#define x32(part)   part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part part

// Replicates the given header N times into N separate headers.
#define x2h(hdr)    hdr, hdr
#define x3h(hdr)    hdr, hdr, hdr
#define x4h(hdr)    hdr, hdr, hdr, hdr
#define x5h(hdr)    hdr, hdr, hdr, hdr, hdr
#define x6h(hdr)    hdr, hdr, hdr, hdr, hdr, hdr
#define x7h(hdr)    hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x8h(hdr)    hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x9h(hdr)    hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x10h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x11h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x12h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x13h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x14h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x15h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x16h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x17h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x18h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x19h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x20h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x21h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x22h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x23h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x24h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x25h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x26h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x27h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x28h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x29h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x30h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x31h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
#define x32h(hdr)   hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr, hdr
