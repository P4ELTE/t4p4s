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

void fill_smac_table(uint8_t port, uint8_t mac[6]) {
  char buffer[2048];
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  /*	struct p4_action_parameter* ap;
  	struct p4_field_match_header* fmh;*/
  struct p4_field_match_exact * exact;

  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "smac");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "ethernet.srcAddr");
  memcpy(exact->bitmap, mac, 6);
  exact->length = 6 * 8 + 0;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "_nop");

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
  strcpy(te->table_name, "dmac");

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

void fill_ue_lpm_table(uint8_t ip[4], uint8_t prefix, uint8_t port, uint8_t mode) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_field_match_lpm * lpm;
  struct p4_field_match_exact * exact;

  printf("ue_selector_lpm\n");
  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "ue_selector_lpm");

  lpm = add_p4_field_match_lpm(te, 2048);
  strcpy(lpm->header.name, "ipv4.dstAddr");
  memcpy(lpm->bitmap, ip, 4);
  lpm->prefix_length = prefix;

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "ethernet.dstPort");
  memcpy(exact->bitmap, & port, 1);

  a = add_p4_action(h, 2048);
  if (mode == 1) {
    printf("add mode gtp_encapsulate\n");
    strcpy(a->description.name, "gtp_encapsulate");
  } else {
    printf("add mode gtp_decapsulate\n");
    strcpy(a->description.name, "gtp_decapsulate");
  }

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_lpm(lpm);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, 2048);
}

void fill_teid_rate_limiter_table(uint32_t teid, uint8_t mode) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_field_match_exact * exact;

  printf("teid_rate_limiter\n");
  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "teid_rate_limiter");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "gtp_metadata.teid");
  memcpy(exact->bitmap, & teid, 1);
  exact->length = 1 * 8 + 0;

  a = add_p4_action(h, 2048);
  if (mode == 1) {
    printf("add mode apply_meter\n");
    strcpy(a->description.name, "apply_meter");
  } else if (mode == 2) {
    printf("add mode _nop\n");
    strcpy(a->description.name, "_nop");
  } else {
    printf("add mode _drop\n");
    strcpy(a->description.name, "_drop");
  }

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);

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
  strcpy(te->table_name, "m_filter");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "gtp_metadata.color");
  memcpy(exact->bitmap, & color, 1);
  exact->length = 1 * 8 + 0;

  a = add_p4_action(h, 2048);
  if (mode == 2) {
    printf("add mode _nop\n");
    strcpy(a->description.name, "_nop");
  } else {
    printf("add mode _drop\n");
    strcpy(a->description.name, "_drop");
  }

  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_exact(exact);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, 2048);
}

void fill_ipv4_lpm_table(uint8_t ip[4], uint8_t prefix, uint32_t nhgrp) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_action_parameter * ap;
  struct p4_field_match_lpm * lpm;

  printf("ipv4_lpm\n");
  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "ipv4_lpm");

  lpm = add_p4_field_match_lpm(te, 2048);
  strcpy(lpm->header.name, "ipv4.dstAddr");
  memcpy(lpm->bitmap, ip, 4);
  lpm->prefix_length = prefix;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "set_nhop");

  printf("add nhgrp\n");
  ap = add_p4_action_parameter(h, a, 2048);
  strcpy(ap->name, "nhgroup");
  memcpy(ap->bitmap, & nhgrp, 4);
  ap->length = 4 * 8 + 0;

  printf("NH-1\n");
  netconv_p4_header(h);
  netconv_p4_add_table_entry(te);
  netconv_p4_field_match_lpm(lpm);
  netconv_p4_action(a);
  netconv_p4_action_parameter(ap);

  send_p4_msg(c, buffer, 2048);
}

void fill_nexthops_table(uint32_t nhgroup, uint8_t port, uint8_t smac[6], uint8_t dmac[6]) {
  char buffer[2048]; /* TODO: ugly */
  struct p4_header * h;
  struct p4_add_table_entry * te;
  struct p4_action * a;
  struct p4_action_parameter * ap, * ap2, * ap3;
  struct p4_field_match_exact * exact;

  printf("nexthops\n");
  printf("Group: %d Port: %d Smac: %d:%d:%d:%d:%d:%d Dmac: %d:%d:%d:%d:%d:%d\n", nhgroup, port, smac[0], smac[1], smac[2], smac[3], smac[4], smac[5], dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

  h = create_p4_header(buffer, 0, 2048);
  te = create_p4_add_table_entry(buffer, 0, 2048);
  strcpy(te->table_name, "nexthops");

  exact = add_p4_field_match_exact(te, 2048);
  strcpy(exact->header.name, "routing_metadata.nhgroup");
  memcpy(exact->bitmap, & nhgroup, 4);
  exact->length = 4 * 8 + 0;

  a = add_p4_action(h, 2048);
  strcpy(a->description.name, "forward");

  ap = add_p4_action_parameter(h, a, 2048);
  strcpy(ap->name, "dmac");
  memcpy(ap->bitmap, dmac, 6);
  ap->length = 6 * 8 + 0;
  ap2 = add_p4_action_parameter(h, a, 2048);
  strcpy(ap2->name, "smac");
  memcpy(ap2->bitmap, smac, 6);
  ap2->length = 6 * 8 + 0;

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
  netconv_p4_action_parameter(ap2);
  netconv_p4_action_parameter(ap3);

  send_p4_msg(c, buffer, 2048);
}

void set_default_action_smac() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table smac\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "smac");

  a = & (sda->action);
  strcpy(a->description.name, "mac_learn");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_dmac() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table dmac\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "dmac");

  a = & (sda->action);
  strcpy(a->description.name, "bcast");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_ue_lpm_table() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table ue_lpm table\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "ue_selector_lpm");

  a = & (sda->action);
  strcpy(a->description.name, "_drop");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_teid_rate_limiter_table() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for tteid_rate_limiter table\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "teid_rate_limiter");

  a = & (sda->action);
  strcpy(a->description.name, "_drop");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_m_filter_table() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table m_filter table\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "m_filter");

  a = & (sda->action);
  strcpy(a->description.name, "_drop");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_nexthops() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table nexthops\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "nexthops");

  a = & (sda->action);
  strcpy(a->description.name, "_drop");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_ipv4_lpm() {
  char buffer[2048];
  struct p4_header * h;
  struct p4_set_default_action * sda;
  struct p4_action * a;

  printf("Generate set_default_action message for table ipv4_lpm\n");

  h = create_p4_header(buffer, 0, sizeof(buffer));

  sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
  strcpy(sda->table_name, "ipv4_lpm");

  a = & (sda->action);
  strcpy(a->description.name, "_drop");

  netconv_p4_header(h);
  netconv_p4_set_default_action(sda);
  netconv_p4_action(a);

  send_p4_msg(c, buffer, sizeof(buffer));
}

void dhf(void * b) {
  printf("Unknown digest received\n");
}

void init_simple() {
  uint8_t ip[4] = {
    10,
    0,
    99,
    99
  };
  uint8_t mac[6] = {
    0xd2,
    0x69,
    0x0f,
    0xa8,
    0x39,
    0x9c
  };
  uint8_t smac[6] = {
    0xd2,
    0x69,
    0x0f,
    0x00,
    0x00,
    0x9c
  };
  uint8_t port = 15;
  uint32_t nhgrp = 0;

  fill_ipv4_lpm_table(ip, 16, nhgrp);
  fill_nexthops_table(nhgrp, port, smac, mac);
}

int read_config_from_file(char * filename) {
  FILE * f;
  char line[100];

  uint8_t smac[6];
  uint8_t dmac[6];
  uint8_t ip[4];
  uint8_t port;
  uint8_t mode;
  uint8_t teid;
  uint8_t color;
  uint8_t prefix;
  uint32_t nhgrp;
  char dummy;

  f = fopen(filename, "r");
  if (f == NULL) return -1;

  int line_index = 0;
  while (fgets(line, sizeof(line), f)) {
    line[strlen(line) - 1] = '\0';
    line_index++;
    printf("Sor: %d.", line_index);
    if (line[0] == 'S') { //SMAC
      if (8 == sscanf(line, "%c %x:%x:%x:%x:%x:%x %d%c", & dummy, & smac[0], & smac[1], & smac[2], & smac[3], & smac[4], & smac[5], & port)) {
        fill_smac_table(port, dmac);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'D') { //DMAC
      if (8 == sscanf(line, "%c %x:%x:%x:%x:%x:%x %d%c", & dummy, & dmac[0], & dmac[1], & dmac[2], & dmac[3], & dmac[4], & dmac[5], & port)) {
        fill_dmac_table(port, dmac);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'U') { //UE_SELECTOR
      if (8 == sscanf(line, "%c %d.%d.%d.%d %d %d %d%c", & dummy, & ip[0], & ip[1], & ip[2], & ip[3], & prefix, & port, & mode)) //mode 1 encapsulate, 0 decapsulate
      {
        fill_ue_lpm_table(ip, prefix, port, mode);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'T') { //teid_rate_limiter
      if (3 == sscanf(line, "%c %d %d%c", & dummy, & teid, & mode)) //mode 0 apply_meter, 1 _nop, 2 _drop
      {
        fill_teid_rate_limiter_table(teid, mode);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'M') { //m_filter
      if (3 == sscanf(line, "%c %d %d%c", & dummy, & color, & mode)) //mode 1 _nop, 2 _drop
      {
        fill_m_filter_table(color, mode);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'E') {
      if (7 == sscanf(line, "%c %d.%d.%d.%d %d %d%c", & dummy, & ip[0], & ip[1], & ip[2], & ip[3], & prefix, & nhgrp)) {
        fill_ipv4_lpm_table(ip, prefix, nhgrp);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else if (line[0] == 'N') {
      char dummy2;
      if (15 == sscanf(line, "%c %d %d %x:%x:%x:%x:%x:%x %x:%x:%x:%x:%x:%x%c", & dummy2, & nhgrp, & port, & smac[0], & smac[1], & smac[2], & smac[3], & smac[4], & smac[5], & dmac[0], & dmac[1], & dmac[2], & dmac[3], & dmac[4], & dmac[5])) {
        printf(line);
        printf("\n");
        printf("%c %d %d %x:%x:%x:%x:%x:%x %x:%x:%x:%x:%x:%x%c", dummy, nhgrp, port, smac[0], smac[1], smac[2], smac[3], smac[4], smac[5], dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);
        fill_nexthops_table(nhgrp, port, smac, dmac);
      } else {
        printf("Wrong format error in line\n");
        fclose(f);
        return -1;
      }
    } else {
      printf("Wrong format error in line\n");
      fclose(f);
      return -1;
    }
  }
  fclose(f);
  return 0;
}

char * fn;

void init_complex() {
  set_default_action_smac();
  set_default_action_dmac();
  set_default_action_ue_lpm_table();
  set_default_action_teid_rate_limiter_table();
  set_default_action_m_filter_table();
  set_default_action_nexthops();
  set_default_action_ipv4_lpm();

  if (read_config_from_file(fn) < 0) {
    printf("File cannnot be opened...\n");
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

  printf("Launching controller's main loop...\n");
  execute_controller(c);

  printf("Destroy controller\n");
  destroy_controller(c);

  return 0;
}
