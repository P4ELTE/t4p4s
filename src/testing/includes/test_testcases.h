// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "test_testsuite.h"
#include "gen_model.h"

// ------------------------------------------------------
// Timeouts

#define NO_CTL_REPLY 0
#define CTL_REPLIES 200

#define WAIT_FOR_CTL  FSLEEP(500)

// ------------------------------------------------------
// Header field changes while processing packet

#define INOUT(from, to)  "<" from "|" to ">"
#define OUT(part)        INOUT("", part)
#define IN(part)         INOUT(part, "")

// ------------------------------------------------------
// Testcase steps

#define FDATA(...)    { __VA_ARGS__, "" }

#define FSLEEP(time)  {FAKE_PKT, 0, 0, FDATA(""), time, 0, FDATA("")}
#define FEND          {FAKE_END, 0, 0, FDATA(""),    0, 0, FDATA("")}


// for internal use
#define SIMPLESEND(inport, outport, ctl, pkt, ...)  {FAKE_PKT, 0, inport, FDATA(pkt, ##__VA_ARGS__), ctl, outport, FDATA(pkt, ##__VA_ARGS__)}
#define REQSEND(inport, outport, ctl, req, pkt, ...)  {FAKE_PKT, 0, inport, FDATA(pkt, ##__VA_ARGS__), ctl, outport, FDATA(pkt, ##__VA_ARGS__), req}

#define BCAST -1
#define DROP  -2
#define ANY   -3
#define SAME  -4
#define RND1  -5
#define RND2  -6
#define RND3  -7
#define RND4  -8

// this packet is processed on the "fast path"
#define FAST(inport, out, pkt, ...)  SIMPLESEND(inport, out == BCAST ? T4P4S_BROADCAST_PORT : out, NO_CTL_REPLY, pkt, ##__VA_ARGS__)
// this packet is processed on the "slow path": the control plane is contacted while processing the packet
#define SLOW(inport, out, pkt, ...)  SIMPLESEND(inport, out == BCAST ? T4P4S_BROADCAST_PORT : out == DROP ? EGRESS_DROP_VALUE : out, CTL_REPLIES, pkt, ##__VA_ARGS__)

// this packet is processed on the "fast path"
#define FASTREQ(inport, out, req, pkt, ...)  REQSEND(inport, out == BCAST ? T4P4S_BROADCAST_PORT : out, NO_CTL_REPLY, req, pkt, ##__VA_ARGS__)
// this packet is processed on the "slow path": the control plane is contacted while processing the packet
#define SLOWREQ(inport, out, req, pkt, ...)  REQSEND(inport, out == BCAST ? T4P4S_BROADCAST_PORT : out == DROP ? EGRESS_DROP_VALUE : out, CTL_REPLIES, req, pkt, ##__VA_ARGS__)

// ------------------------------------------------------
// Testcase structuring

#define LCORE(...)           { __VA_ARGS__ , FEND }
#define LCORES(...)          { __VA_ARGS__ }
#define SINGLE_LCORE(...)    { LCORE(__VA_ARGS__), { FEND }}

// ------------------------------------------------------
// Conditions

#define REQ(reqs)       { reqs }
