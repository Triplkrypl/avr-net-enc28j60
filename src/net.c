#ifndef NET
#define NET

#include <string.h>
#include "arp.h"
#include "icmp.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

// todo rozkouskovat do jednotlivych protokolu
void NetHandleIncomingPacket(unsigned char *buffer, unsigned short length){
 unsigned char src_mac[MAC_ADDRESS_SIZE];
 memcpy(src_mac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
 if(arp_packet_is_arp(buffer, ARP_OPCODE_REQUEST_V)){
  arp_send_reply(buffer, src_mac);
  return;
 }
 if(!ip_packet_is_ip(buffer)){
  return;
 }
 unsigned char src_ip[4];
 memcpy(src_ip, buffer + IP_SRC_IP_P, 4);
 if(icmp_send_reply(buffer, length, src_mac, src_ip)){// moznost vypnuti icmp
  return;
 }
 unsigned short src_port;
 if(udp_is_udp( buffer, 5000, &src_port )){// upravit na nactaeni dest portu a predi do callbacku
  // pridani callbacku na prichozi packaket
  return;
 }
 if(tcp_is_tcp(buffer, 80, &src_port)){// refactor jako prease
  tcp_handle_incoming_packet(buffer, length, src_mac, src_ip, src_port, 80);
 }
}

void NetInit() {
 enc28j60_init();
 TcpInit();
}
#endif
