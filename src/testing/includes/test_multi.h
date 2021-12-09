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

// PAD(len,txt) extends txt to len bytes like this: pad(5, "abcd") ===> 000000abcd
#define tlen(txt)                   (((int)sizeof(txt)-1)/2)
#define lendiff(len,txtlen)         (len - txtlen)
#define xpad(len,txtlen,txt,pad)    (lendiff(len,txtlen) >= 32  ? x32(pad) txt : \
                                     lendiff(len,txtlen) >= 31  ? x31(pad) txt : \
                                     lendiff(len,txtlen) >= 30  ? x30(pad) txt : \
                                     lendiff(len,txtlen) >= 29  ? x29(pad) txt : \
                                     lendiff(len,txtlen) >= 28  ? x28(pad) txt : \
                                     lendiff(len,txtlen) >= 27  ? x27(pad) txt : \
                                     lendiff(len,txtlen) >= 26  ? x26(pad) txt : \
                                     lendiff(len,txtlen) >= 25  ? x25(pad) txt : \
                                     lendiff(len,txtlen) >= 24  ? x24(pad) txt : \
                                     lendiff(len,txtlen) >= 23 ? x23(pad) txt : \
                                     lendiff(len,txtlen) >= 22 ? x22(pad) txt : \
                                     lendiff(len,txtlen) >= 21 ? x21(pad) txt : \
                                     lendiff(len,txtlen) >= 20 ? x20(pad) txt : \
                                     lendiff(len,txtlen) >= 19 ? x19(pad) txt : \
                                     lendiff(len,txtlen) >= 18 ? x18(pad) txt : \
                                     lendiff(len,txtlen) >= 17 ? x17(pad) txt : \
                                     lendiff(len,txtlen) >= 16 ? x16(pad) txt : \
                                     lendiff(len,txtlen) >= 15 ? x15(pad) txt : \
                                     lendiff(len,txtlen) >= 14 ? x14(pad) txt : \
                                     lendiff(len,txtlen) >= 13 ? x13(pad) txt : \
                                     lendiff(len,txtlen) >= 12 ? x12(pad) txt : \
                                     lendiff(len,txtlen) >= 11 ? x11(pad) txt : \
                                     lendiff(len,txtlen) >= 10 ? x10(pad) txt : \
                                     lendiff(len,txtlen) >= 9 ? x9(pad) txt : \
                                     lendiff(len,txtlen) >= 8 ? x8(pad) txt : \
                                     lendiff(len,txtlen) >= 7 ? x7(pad) txt : \
                                     lendiff(len,txtlen) >= 6 ? x6(pad) txt : \
                                     lendiff(len,txtlen) >= 5 ? x5(pad) txt : \
                                     lendiff(len,txtlen) >= 4 ? x4(pad) txt : \
                                     lendiff(len,txtlen) >= 3 ? x3(pad) txt : \
                                     lendiff(len,txtlen) >= 2 ? x2(pad) txt : \
                                     lendiff(len,txtlen) >= 1 ? pad txt : \
                                     txt)
#define PAD(len,txt)                xpad(len,tlen(txt),txt,"00")
