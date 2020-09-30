// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_IPS 60000

controller c;

extern void notify_controller_initialized();
extern void set_table_default_action(char* table_nickname, char* table_name, char* default_action_name);
extern void fill_teid_rate_limiter_table(uint32_t teid, uint8_t mode);

void fill_smac_table(uint8_t port, uint8_t mac[6]) {
  char buffer[2048];
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  /*  struct p4_action_parameter* ap;
    struct p4_field_match_header* fmh;*/
  struct p4_field_match_exact * exact;

  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "smac_0");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "ethernet.srcAddr");
  memcpy(exact->bitmap, mac, 6);
  exact->length = 6 * 8 + 0;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "NoAction_0");

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, 2048);
}

void fill_dmac_table(uint8_t port, uint8_t mac[6]) {
  char buffer[2048];
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_action_parameter * ap;
  struct p4_field_match_exact * exact;

  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "dmac_0");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "ethernet.dstAddr");
  memcpy(exact->bitmap, mac, 6);
  exact->length = 6 * 8 + 0;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "forward");

  ap = add_p4_action_parameter(h, a, 2048);
  strcpy(ap->name, "port");
  memcpy(ap->bitmap, & port, 1);
  ap->length = 1 * 8 + 0;

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);
  netconv_p4_action_parameter(ap);

  send_p4_msg(c, buffer, 2048);
}

void fill_m_filter_table(uint8_t color, uint8_t mode) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_field_match_exact * exact;

  printf("m_filter\n");
  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "m_filter_0");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "gtp_metadata.color");
  memcpy(exact->bitmap, & color, 1);
  exact->length = 1 * 8 + 0;

  a = add_p4_action(h, 2048);
  if (mode == 2) {
    printf("add mode _nop\n");
    strcpy(a->description.name, "NoAction_5");
  } else {
    printf("add mode _drop\n");
    strcpy(a->description.name, "drop_6");
  }

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, 2048);
}

void fill_ipv4_lpm_table(uint8_t ip[4], uint8_t prefix, uint8_t nhgrp) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_action_parameter * ap;
  struct p4_field_match_lpm * lpm;

  printf("ipv4_lpm_0 %d.%d.%d.%d / %d\n", ip[0], ip[1], ip[2], ip[3], prefix);
  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "ipv4_lpm_0");

  lpm = add_p4_field_match_lpm(te, 2048);
  strcpy(lpm->header.name, "ipv4.dstAddr");
  memcpy(lpm->bitmap, ip, 4);
  lpm->prefix_length = prefix;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "set_nhgrp");

  printf("add nhgrp\n");
  ap = add_p4_action_parameter(h, a, 2048);
  strcpy(ap->name, "nhgroup");
  memcpy(ap->bitmap, & nhgrp, 1);
  ap->length = 1 * 8 + 0;

//  printf("NH-1\n");
  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_lpm(lpm);
  netconv_p4_action(a);
  netconv_p4_action_parameter(ap);

  send_p4_msg(c, buffer, 2048);
}

void fill_ipv4_forward_table(uint8_t nhgroup, uint8_t port, uint8_t smac[6], uint8_t dmac[6]) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_action_parameter * ap;
  // struct p4_action_parameter * ap2;
  struct p4_action_parameter * ap3;
  struct p4_field_match_exact * exact;

  printf("ipv4_forward\n");
  printf("Group: %d Port: %d Smac: %d:%d:%d:%d:%d:%d Dmac: %d:%d:%d:%d:%d:%d\n", nhgroup, port, smac[0], smac[1], smac[2], smac[3], smac[4], smac[5], dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "ipv4_forward_0");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "routing_metadata.nhgrp");
  memcpy(exact->bitmap, & nhgroup, 1);
  exact->length = 1 * 8 + 0;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "pkt_send");

  ap = add_p4_action_parameter(h, a, 2048);
  strcpy(ap->name, "nhmac");
  memcpy(ap->bitmap, dmac, 6);
  ap->length = 6 * 8 + 0;
/*  ap2 = add_p4_action_parameter(h, a, 2048);
  strcpy(ap2->name, "smac");
  memcpy(ap2->bitmap, smac, 6);
  ap2->length = 6 * 8 + 0;*/

  ap3 = add_p4_action_parameter(h, a, 2048);
  strcpy(ap3->name, "port");
  ap3->bitmap[0] = port;
  ap3->bitmap[1] = 0;
  ap3->length = 2 * 8 + 0;

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);
  netconv_p4_action_parameter(ap);
/*  netconv_p4_action_parameter(ap2);*/
  netconv_p4_action_parameter(ap3);

  send_p4_msg(c, buffer, 2048);
}

void dhf(void* b) {
  printf("Unknown digest received\n");
}

void init_simple() {
  uint8_t ip[4] = {
    10,
    0,
    99,
    99
  };
  // uint8_t mac[6] = {
  //   0xd2,
  //   0x69,
  //   0x0f,
  //   0xa8,
  //   0x39,
  //   0x9c
  // };
  // uint8_t smac[6] = {
  //   0xd2,
  //   0x69,
  //   0x0f,
  //   0x00,
  //   0x00,
  //   0x9c
  // };
  // uint8_t port = 15;
  uint32_t nhgrp = 0;

  fill_ipv4_lpm_table(ip, 16, nhgrp);
//  fill_nexthops_table(nhgrp, port, smac, mac);
}

int read_config_from_file(char * filename) {
  FILE * f;
  char line[100];

  uint8_t smac[6];
  uint8_t dmac[6];
  uint8_t ip[4];
  uint8_t ipbst[4];
  uint8_t port;
  uint16_t udpport;
  uint8_t mode;
  uint32_t teid;
  uint8_t color;
  uint8_t prefix;
  uint16_t temp;
  uint8_t nhgrp;
  char format_code[256];

  printf("Opening config file %s\n", filename);
  f = fopen(filename, "r");
  if (f == NULL) return -1;

  int error = 0;
  int line_index = 0;
  while (fgets(line, sizeof(line), f)) {
    line[strlen(line) - 1] = '\0';
    line_index++;
    printf("Line: %d. ", line_index);

    sscanf(line, "%s ", format_code);
    printf("Found code %s\n", format_code);

    if (!strcmp("SMAC", format_code) || !strcmp("M", format_code)) { //SMAC
      error = (7 != sscanf(line, "%s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx", format_code, & smac[0], & smac[1], & smac[2], & smac[3], & smac[4], & smac[5]));
      if (error)   break;

      // fill_smac_table(port, smac);
    } else if (!strcmp("DMAC", format_code)) { //DMAC
      error = (8 != sscanf(line, "%s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hhd", format_code, & dmac[0], & dmac[1], & dmac[2], & dmac[3], & dmac[4], & dmac[5], & port));
      if (error)   break;

      fill_dmac_table(port, dmac);
    } else if (!strcmp("UE-SELECTOR", format_code) || !strcmp("U", format_code)) { //UE_SELECTOR
      char mode_string[256];

      error = (13 != sscanf(line, "%s %hhd.%hhd.%hhd.%hhd %hu %hd %s %d %hhd.%hhd.%hhd.%hhd", format_code, & ip[0], & ip[1], & ip[2], & ip[3], &temp, & udpport, mode_string, &teid, &ipbst[0], &ipbst[1],&ipbst[2],&ipbst[3]));
      if (error)   break;

      // modes: 1 encapsulate, 0 decapsulate
      if (!strcmp("DECAPSULATE", mode_string) || !strcmp("0", mode_string))  mode = 0;
      if (!strcmp("ENCAPSULATE", mode_string) || !strcmp("1", mode_string))  mode = 1;

      prefix = 32;
      printf("%s\n", line);
      printf("%s %d.%d.%d.%d %u %d %d %d %d.%d.%d.%d\n", format_code, ip[0], ip[1], ip[2], ip[3], prefix, udpport, mode, teid, ipbst[0], ipbst[1],ipbst[2],ipbst[3]);
      printf("PREFIX:::%d %d\n", prefix, temp);

      // struct p4_header* hdr = new_hdr();
      // add_table_lpm("ue_selector_0", "ipv4.dstAddr", prefix, ip);
      // if (mode == 1) {
      //   add_action2(hdr, "gtp_encapsulate", "teid", &teid, 4, "ip", &ipbst, 4);
      // } else {
      //   add_action0(hdr, "gtp_decapsulate");
      // }
      // send_msg();

    } else if (!strcmp("TEID-RATE-LIMITER", format_code) || !strcmp("T", format_code)) { //teid_rate_limiter
      char teid_mode[256];
      error = (3 != sscanf(line, "%s %d %s", format_code, & teid, teid_mode));
      if (error)   break;

      // modes: 0 apply_meter, 1 _nop, 2 _drop
      if (!strcmp("APPLY-METER", teid_mode) || !strcmp("0", teid_mode))  mode = 0;
      if (!strcmp("NOP", teid_mode)         || !strcmp("1", teid_mode))  mode = 1;
      if (!strcmp("DROP", teid_mode)        || !strcmp("2", teid_mode))  mode = 2;

      fill_teid_rate_limiter_table(teid, mode);
    } else if (!strcmp("M-FILTER", format_code) || !strcmp("F", format_code)) { //m_filter
      char m_filter_mode[256];
      error = (3 != sscanf(line, "%s %hhd %s", format_code, & color, m_filter_mode));
      if (error)   break;

      // modes: 1 _nop, 2 _drop
      if (!strcmp("NOP", m_filter_mode)  || !strcmp("1", m_filter_mode))         mode = 1;
      if (!strcmp("DROP", m_filter_mode) || !strcmp("2", m_filter_mode))         mode = 2;
      fill_m_filter_table(color, mode);
    } else if (!strcmp("NEXTHOP", format_code) || !strcmp("E", format_code)) {
      error = (7 != sscanf(line, "%s %hhd.%hhd.%hhd.%hhd %hhd %hhd", format_code, & ip[0], & ip[1], & ip[2], & ip[3], & prefix, & nhgrp));
      if (error)   break;

      fill_ipv4_lpm_table(ip, prefix, nhgrp);
    } else if (!strcmp("SMAC-DMAC", format_code) || !strcmp("N", format_code)) {
      error = (15 != sscanf(line, "%s %hhd %hhd %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hhx:%hhx:%hhx:%hhx:%hhx:%hhx", format_code, & nhgrp, & port, & smac[0], & smac[1], & smac[2], & smac[3], & smac[4], & smac[5], & dmac[0], & dmac[1], & dmac[2], & dmac[3], & dmac[4], & dmac[5]));
      if (error)   break;

      printf("%s\n", line);
      printf("%s %d %d %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hhx:%hhx:%hhx:%hhx:%hhx:%hhx", format_code, nhgrp, port, smac[0], smac[1], smac[2], smac[3], smac[4], smac[5], dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);
      fill_ipv4_forward_table(nhgrp, port, smac, dmac);
    } else {
      printf("Wrong format error on line %d, unknown code %s: %s\n", line_index, format_code, line);
      fclose(f);
      return -2;
    }
  }

  if (error != 0) {
    printf("Wrong format error on line %d: %s\n", line_index, line);
    fclose(f);
    return -2;
  }

  fclose(f);
  return 0;
}

char * fn;

void init_complex() {
  // set_table_default_action("smac", "smac", "mac_learn");
  // set_table_default_action("dmac", "dmac", "bcast");
  set_table_default_action("ue_lpm", "ue_selector_0", "drop");
  // set_table_default_action("tteid_rate_limiter", "teid_rate_limiter", "_drop_3");
  // set_table_default_action("m_filter", "m_filter", "_drop_4");
  set_table_default_action("nexthops", "ipv4_forward_0", "drop_8");
  set_table_default_action("ipv4_lpm", "ipv4_lpm_0", "drop_7");

  int retval = read_config_from_file(fn);
  if (retval == -1) {
    printf("File %s cannot be opened...\n", fn);
  } else if (retval == -2) {
    printf("File %s contains errors, stopped processing\n", fn);
  } else {
    printf("File %s processed successfully\n", fn);
  } 
}

int main(int argc, char * argv[]) {
  printf("Create and configure controller...\n");

  if (argc > 1) {
    if (argc != 2) {
      printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
      return -1;
    }
    printf("Command line argument is present...\nLoading configuration data...\n");
    fn = argv[1];
    c = create_controller_with_init(11111, 3, dhf, init_complex);
  } else {
    c = create_controller_with_init(11111, 3, dhf, init_simple);
  }

  notify_controller_initialized();

  printf("Launching controller's main loop...\n");
  execute_controller(c);


  printf("Destroy controller\n");
  destroy_controller(c);

  return 0;
}
