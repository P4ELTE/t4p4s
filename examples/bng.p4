#include <core.p4>
#include <v1model.p4>

/***********************  C O N S T A N T S  *****************************/
const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_ARP  = 0x0806;
const bit<8>  IPPROTO_ICMP   = 0x01;

/***********************  H E A D E R S  *********************************/

const bit<16> ARP_HTYPE_ETHERNET = 0x0001;
const bit<16> ARP_PTYPE_IPV4     = 0x0800;
const bit<8>  ARP_HLEN_ETHERNET  = 6;
const bit<8>  ARP_PLEN_IPV4      = 4;

header cpu_header_t {
    bit<64> preamble;
    bit<8>  device;
    bit<8>  reason;
    bit<8>  if_index;
}

header arp_t {
    bit<16> htype;
    bit<16> ptype;
    bit<8>  hlen;
    bit<8>  plen;
    bit<16> oper;
}

header ethernet_t
{
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    bit<32>      srcAddr;
    bit<32>      dstAddr;
}

/*header icmp_h {
    bit<8> type_;
    bit<8> code;
    bit<16> hdrChecksum;
}*/

header icmp_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4> dataOffset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header gre_t {
    bit<1> C;
    bit<1> R;
    bit<1> K;
    bit<1> S;
    bit<1> s;
    bit<3> recurse;
    bit<5> flags;
    bit<3> ver;
    bit<16> proto;
}

header udp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header sctp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> verifTag;
    bit<32> checksum;
}


header nvgre_t {
    bit<24> tni;
    bit<8> reserved;
}

/*header arp_rarp_h {
    bit<16> hwType;
    bit<16> protoType;
    bit<8> hwAddrLen;
    bit<8> protoAddrLen;
    bit<16> opcode;
}*/

/*header arp_rarp_ipv4_h {
    bit<48> srcHwAddr;
    bit<32> srcProtoAddr;
    bit<48> dstHwAddr;
    bit<32> dstProtoAddr;
}*/

header arp_ipv4_t {
    bit<48>  sha;
    bit<32>  spa;
    bit<48>  tha;
    bit<32>  tpa;
}



struct headers {
    ethernet_t   ethernet;
    ethernet_t   outer_ethernet;
    ethernet_t   ethernet_decap;
    arp_t        arp;
    ipv4_t       ipv4;
    ipv4_t       outer_ipv4;

    gre_t        gre;
    tcp_t        tcp;
    icmp_t       icmp;
    @name("inner_ipv4") 
    ipv4_t       inner_ipv4;
    @name("inner_ethernet") 
    ethernet_t   inner_ethernet;
    @name("inner_tcp") 
    tcp_t        inner_tcp;
    @name("inner_icmp") 
    icmp_t       inner_icmp;
}

 struct meta_ipv4_t {            
     bit<4>       version;  
     bit<4>       ihl;      
     bit<8>       diffserv; 
     bit<16>      totalLen; 
     bit<16>      identification;
     bit<3>       flags;    
     bit<13>      fragOffset;
     bit<8>       ttl;
     bit<8>       protocol;
     bit<16>      hdrChecksum;
     bit<32>      srcAddr;
     bit<32>      dstAddr;
 }
/***********************  M E T A D A T A  *******************************/
struct routing_metadata_t {
    bit<32> nhgroup;

    bit<32> dst_ipv4;
    bit<32> src_ipv4;
    bit<48>  mac_da;
    bit<48>  mac_sa;
    bit<9>   egress_port;
    bit<48>  my_mac;

    bit<32> nhop_ipv4;
    bit<1>  do_forward;
    bit<1>  rewrite_outer;
    bit<16> tcp_sp;
    bit<16> tcp_dp;

    bit<8>  if_index;    
    bit<32> if_ipv4_addr;
    bit<48> if_mac_addr;
    bit<1>  is_ext_if;
    
    bit<32> tunnel_id;
    bit<5>  ingress_tunnel_type;
    bit<1>  tcp_inner_en;
    bit<16> lkp_inner_l4_sport;
    bit<16> lkp_inner_l4_dport;

    bit<32>  dst_inner_ipv4;
    bit<32>  src_inner_ipv4;

    bit<32> meter_tag;

}

struct metadata {
    @name(".routing_metadata") 
    routing_metadata_t routing_metadata;
    @name(".meta_ipv4") 
    meta_ipv4_t meta_ipv4 ;
   
}



/***********************  P A R S E R  ***********************************/
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition parse_ethernet;
    }
    @name ("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_ARP  : parse_arp;
            default        : accept;
        }
    }
    @name ("parse_arp") state parse_arp {
        packet.extract(hdr.arp);
        transition accept;
    }

    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPPROTO_ICMP : parse_icmp;
            8w0x6 :         parse_tcp;
            8w47 :         parse_gre;
            default      : accept;
        }
    }
    @name("parse_icmp") state parse_icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }    
    @name("parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
    @name("parse_gre") state parse_gre {
        packet.extract<gre_t>(hdr.gre);
        transition select(hdr.gre.C, hdr.gre.R, hdr.gre.K, hdr.gre.S, hdr.gre.s, hdr.gre.recurse, hdr.gre.flags, hdr.gre.ver, hdr.gre.proto) {
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x800): parse_gre_ipv4;
            default: accept;
        }
    }
    @name(".parse_gre_ipv4") state parse_gre_ipv4 {
        transition parse_inner_ipv4;
    }

    @name(".parse_inner_ipv4") state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ihl, hdr.inner_ipv4.protocol) {
            (13w0x0, 4w0x5, 8w0x1): parse_inner_icmp;
            (13w0x0, 4w0x5, 8w0x6): parse_inner_tcp;
            /*(13w0x0, 4w0x5, 8w0x11): parse_inner_udp;*/
            default: accept;
        }
    }
    @name(".parse_inner_icmp") state parse_inner_icmp {
        packet.extract(hdr.inner_icmp);
        transition accept;
    }

    @name(".parse_inner_tcp") state parse_inner_tcp {
        packet.extract(hdr.inner_tcp);
        transition accept;
    }

    @name(".parse_inner_ethernet") state parse_inner_ethernet {
        packet.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.etherType) {
            16w0x800: parse_inner_ipv4;
            default: accept;
        }
    }
}
@name("mac_learn_digest") struct mac_learn_digest {
    bit<8> in_port;    /* 9 bits?, it doesnt compile with other value like 16, why? */
    bit<48> mac_sa;
}


/**************  I N G R E S S   P R O C E S S I N G   ******************/
control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    
    /***************************** Drop  **************************/
     @name(".drop")action drop() {
        /*mark_to_drop();*/

    }
    /***************************** set IF info and others  **************************/
    /*action set_if_info(bit<32> ipv4_addr, bit<48> mac_addr, bit<1> is_ext) { */
    @name(".set_if_info") action set_if_info(bit<1> is_ext) {
        
        
        meta.routing_metadata.mac_da = hdr.ethernet.dstAddr;
        meta.routing_metadata.mac_sa = hdr.ethernet.srcAddr;
        meta.routing_metadata.if_ipv4_addr = 0x7fef4800 ;
        meta.routing_metadata.if_mac_addr = 0x010101010100;
        meta.routing_metadata.is_ext_if = is_ext;

    }
    @name(".if_info") table if_info {
        key = { meta.routing_metadata.if_index: exact;}  
        /*key = { standard_metadata.ingress_port: exact;}*/
        actions = {   drop;  set_if_info; }
    default_action = drop();     
   
    }
    /***************************** process mac learn  *****************************/
   
    @name(".generate_learn_notify") action generate_learn_notify() {
        digest<mac_learn_digest>(32w1024, {meta.routing_metadata.if_index, hdr.ethernet.srcAddr });
    }
    @name(".smac") table smac {
        actions = {
            generate_learn_notify;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
    }
    
    /***************************** tunnel control decap *****************************/
    @name(".decap_gre_inner_ipv4") action decap_gre_inner_ipv4(bit <32> tunnel_id) {
               
           hdr.ipv4.setInvalid();
           hdr.gre.setInvalid();
           meta.routing_metadata.tunnel_id = tunnel_id;         
           meta.routing_metadata.dst_ipv4 = hdr.inner_ipv4.dstAddr;
	   standard_metadata.egress_port = 1; 
           meta.routing_metadata.is_ext_if = 0;
           
    }
    
    @name("decap_process_outer") table decap_process_outer {
        actions = {decap_gre_inner_ipv4; drop; }
        key ={ hdr.ethernet.srcAddr: exact;}
        
        size = 1024;
        default_action = drop();
    } 
    
    /***************************** Nat control *****************************************/
    @name(".nat_hit_int_to_ext") action nat_hit_int_to_ext(bit<32> srcAddr, bit<16> srcPort) {
        meta.routing_metadata.rewrite_outer = 1w1;
        hdr.inner_ipv4.srcAddr= srcAddr;
        hdr.inner_tcp.srcPort = srcPort;
    }

    @name(".nat_up") table nat_up {
        actions = { drop; nat_hit_int_to_ext; }
        key = { hdr.inner_ipv4.srcAddr: exact;}
        size = 1024;
        default_action = drop();     
    }

    @name(".nat_hit_ext_to_int") action nat_hit_ext_to_int(bit<32> dstAddr, bit<16> dstPort) {
        meta.routing_metadata.rewrite_outer = 1w0;
        meta.routing_metadata.dst_ipv4 = dstAddr; /* to lpm */
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp.dstPort = dstPort;
    }
    @name(".nat_dw") table nat_dw {
        actions = { drop; nat_hit_ext_to_int;  }
        key = {meta.routing_metadata.is_ext_if: exact; }
        size = 1024;
        default_action = drop();
    }
    
    /***************************** tunnel control encap *****************************/
    
    /*@name(".ipv4_gre_rewrite") action ipv4_gre_rewrite(bit<32> gre_srcAddr, bit<32> gre_dstAddr) { */
    @name(".ipv4_gre_rewrite") action ipv4_gre_rewrite(bit<32> gre_srcAddr) {
      hdr.ethernet.setInvalid();     
      hdr.gre.setValid();
      hdr.gre.proto = 16w0x800;
 
      meta.meta_ipv4.version        = hdr.ipv4.version        ; 
      meta.meta_ipv4.ihl            = hdr.ipv4.ihl            ;
      meta.meta_ipv4.diffserv       = hdr.ipv4.diffserv       ;
      meta.meta_ipv4.totalLen       = hdr.ipv4.totalLen       ;
      meta.meta_ipv4.identification = hdr.ipv4.identification ;
      meta.meta_ipv4.flags          = hdr.ipv4.flags          ;
      meta.meta_ipv4.fragOffset     = hdr.ipv4.fragOffset     ;
      meta.meta_ipv4.ttl            = hdr.ipv4.ttl            ;
      meta.meta_ipv4.protocol       = hdr.ipv4.protocol       ;
      meta.meta_ipv4.hdrChecksum    = hdr.ipv4.hdrChecksum    ;
      meta.meta_ipv4.srcAddr        = hdr.ipv4.srcAddr        ;
      meta.meta_ipv4.dstAddr        = hdr.ipv4.dstAddr        ;

      hdr.outer_ipv4.setValid();  
      hdr.outer_ipv4.srcAddr  = 0x04000001;  
      hdr.outer_ipv4.dstAddr  = gre_srcAddr;  
      hdr.outer_ipv4.protocol = 47; 
      hdr.outer_ipv4.version        =  meta.meta_ipv4.version        ;          
      hdr.outer_ipv4.ihl            =  meta.meta_ipv4.ihl            ;
      hdr.outer_ipv4.diffserv       =  meta.meta_ipv4.diffserv       ;
      hdr.outer_ipv4.totalLen       =  meta.meta_ipv4.totalLen       ;
      hdr.outer_ipv4.identification =  meta.meta_ipv4.identification ;
      hdr.outer_ipv4.flags          =  meta.meta_ipv4.flags          ;
      hdr.outer_ipv4.fragOffset     =  meta.meta_ipv4.fragOffset     ;
      hdr.outer_ipv4.ttl            =  meta.meta_ipv4.ttl            ;

      hdr.outer_ethernet.setValid(); 
      hdr.outer_ethernet.dstAddr = 0x000000000001; 
      hdr.outer_ethernet.srcAddr = 0x000000000002; 
      hdr.outer_ethernet.etherType = 16w0x800;
      standard_metadata.egress_port = 1;
      meta.routing_metadata.dst_ipv4= hdr.outer_ipv4.dstAddr;  
      meta.routing_metadata.rewrite_outer = 0;             
       
    }

      @name(".tunnel_encap_process_outer") table tunnel_encap_process_outer {
        actions = {ipv4_gre_rewrite; drop; }
        key = {  hdr.ipv4.dstAddr       : exact; }    
        size = 128;
    }
    
    /************** forwarding ipv4 ******************/
      
          
    @name(".set_nhop") action set_nhop(bit<9> port) {
        standard_metadata.egress_port =  port; 
    }
    
    @name(".ipv4_up") table ipv4_up {
        key = {meta.routing_metadata.dst_ipv4 : lpm;}  
        actions = { set_nhop; drop;  }
    }
    @name(".rewrite_src_mac") action rewrite_src_mac(bit<48> src_mac) {

        hdr.ethernet.setInvalid();
           hdr.ethernet_decap.setValid();
           hdr.ethernet_decap.dstAddr =  meta.routing_metadata.mac_da; 
           hdr.ethernet_decap.srcAddr =  src_mac;
           hdr.ethernet_decap.etherType = 16w0x800;
    } 

    @name(".sendout") table sendout {
        actions = {drop; rewrite_src_mac; }
        key = {  standard_metadata.egress_port: exact; }
        size = 512;
    }    
    
    @name(".rewrite_src_mac_dw") action rewrite_src_mac_dw(bit<48> src_mac) {
           hdr.outer_ethernet.dstAddr =  meta.routing_metadata.mac_da; 
           hdr.outer_ethernet.srcAddr =  src_mac;
           hdr.outer_ethernet.etherType = 16w0x800;
    } 

    @name(".sendout_dw") table sendout_dw {
        actions = {drop; rewrite_src_mac_dw; }
        key = {  standard_metadata.egress_port: exact; }
        size = 512;
    }    
 
    /************** APPLY ******************/
    apply {
        if_info.apply();
        smac.apply();  
        /* ------ decap-------  */ 
        if(hdr.ipv4.protocol== 8w47){
           decap_process_outer.apply();
           nat_up.apply();
        }

        if(meta.routing_metadata.is_ext_if == 1){
           nat_dw.apply();  
           tunnel_encap_process_outer.apply(); 
        }
           ipv4_up.apply(); 
 
        if (meta.routing_metadata.rewrite_outer == 1w1) {sendout.apply(); }
        if (meta.routing_metadata.rewrite_outer == 1w0) {sendout_dw.apply(); }

    }
    
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}
/************   C H E C K S U M    V E  I F I C A T I O N   *************/
control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
